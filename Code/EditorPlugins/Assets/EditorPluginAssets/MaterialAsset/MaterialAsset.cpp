#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/ShaderTypeRegistry.h>
#include <EditorPluginAssets/VisualShader/VsCodeGenerator.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

namespace
{
  plResult AddDefines(plPreprocessor& inout_pp, const plDocumentObject* pObject, const plAbstractProperty* pProp)
  {
    plStringBuilder sDefine;

    const char* szName = pProp->GetPropertyName();
    if (pProp->GetSpecificType()->GetVariantType() == plVariantType::Bool)
    {
      sDefine.Set(szName, " ", pObject->GetTypeAccessor().GetValue(szName).Get<bool>() ? "TRUE" : "FALSE");
      return inout_pp.AddCustomDefine(sDefine);
    }
    else if (pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags))
    {
      plInt64 iValue = pObject->GetTypeAccessor().GetValue(szName).ConvertTo<plInt64>();

      plHybridArray<plReflectionUtils::EnumKeyValuePair, 16> enumValues;
      plReflectionUtils::GetEnumKeysAndValues(pProp->GetSpecificType(), enumValues, plReflectionUtils::EnumConversionMode::ValueNameOnly);
      for (auto& enumValue : enumValues)
      {
        sDefine.Format("{} {}", enumValue.m_sKey, enumValue.m_iValue);
        PLASMA_SUCCEED_OR_RETURN(inout_pp.AddCustomDefine(sDefine));

        if (enumValue.m_iValue == iValue)
        {
          sDefine.Set(szName, " ", enumValue.m_sKey);
          PLASMA_SUCCEED_OR_RETURN(inout_pp.AddCustomDefine(sDefine));
        }
      }

      return PLASMA_SUCCESS;
    }

    PLASMA_REPORT_FAILURE("Invalid shader permutation property type '{0}'", pProp->GetSpecificType()->GetTypeName());
    return PLASMA_FAILURE;
  }

  plResult ParseMaterialConfig(plStringView sRelativeFileName, const plDocumentObject* pShaderPropertyObject, plVariantDictionary& out_materialConfig)
  {
    plStringBuilder sFileContent;
    {
      plFileReader File;
      if (File.Open(sRelativeFileName).Failed())
        return PLASMA_FAILURE;

      sFileContent.ReadAll(File);
    }

    plShaderHelper::plTextSectionizer sections;
    plShaderHelper::GetShaderSections(sFileContent, sections);

    plUInt32 uiFirstLine = 0;
    plStringView sMaterialConfig = sections.GetSectionContent(plShaderHelper::plShaderSections::MATERIALCONFIG, uiFirstLine);

    plPreprocessor pp;
    pp.SetPassThroughPragma(false);
    pp.SetPassThroughLine(false);

    // set material permutation var
    {
      PLASMA_SUCCEED_OR_RETURN(pp.AddCustomDefine("TRUE 1"));
      PLASMA_SUCCEED_OR_RETURN(pp.AddCustomDefine("FALSE 0"));

      plHybridArray<const plAbstractProperty*, 32> properties;
      pShaderPropertyObject->GetType()->GetAllProperties(properties);

      plStringBuilder sDefine;
      plStringBuilder sValue;
      for (auto& pProp : properties)
      {
        const plCategoryAttribute* pCategory = pProp->GetAttributeByType<plCategoryAttribute>();
        if (pCategory == nullptr || plStringUtils::IsEqual(pCategory->GetCategory(), "Permutation") == false)
          continue;

        PLASMA_SUCCEED_OR_RETURN(AddDefines(pp, pShaderPropertyObject, pProp));
      }
    }

    pp.SetFileOpenFunction([&](plStringView sAbsoluteFile, plDynamicArray<plUInt8>& out_fileContent, plTimestamp& out_fileModification) {
        if (sAbsoluteFile == "MaterialConfig")
        {
          out_fileContent.PushBackRange(plMakeArrayPtr((const plUInt8*)sMaterialConfig.GetStartPointer(), sMaterialConfig.GetElementCount()));
          return PLASMA_SUCCESS;
        }

        plFileReader r;
        if (r.Open(sAbsoluteFile).Failed())
        {
          plLog::Error("Could not find include file '{0}'", sAbsoluteFile);
          return PLASMA_FAILURE;
        }

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)
        plFileStats stats;
        if (plFileSystem::GetFileStats(sAbsoluteFile, stats).Succeeded())
        {
          out_fileModification = stats.m_LastModificationTime;
        }
#endif

        plUInt8 Temp[4096];
        while (plUInt64 uiRead = r.ReadBytes(Temp, 4096))
        {
          out_fileContent.PushBackRange(plArrayPtr<plUInt8>(Temp, (plUInt32)uiRead));
        }

        return PLASMA_SUCCESS; });

    bool bFoundUndefinedVars = false;
    pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVars](const plPreprocessor::ProcessingEvent& e) {
        if (e.m_Type == plPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVars = true;

          plLog::Error("Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}. Only material permutation variables are allowed in material config sections.", e.m_pToken->m_DataView, e.m_pToken->m_File, e.m_pToken->m_uiLine);
        } });

    plStringBuilder sOutput;
    if (pp.Process("MaterialConfig", sOutput, false).Failed() || bFoundUndefinedVars)
    {
      plLog::Error("Preprocessing the material config section failed");
      return PLASMA_FAILURE;
    }

    plHybridArray<plStringView, 32> allAssignments;
    sOutput.Split(false, allAssignments, "\n", ";", "\r");

    plStringBuilder temp;
    plHybridArray<plStringView, 4> components;
    for (const plStringView& assignment : allAssignments)
    {
      temp = assignment;
      temp.Trim(" \t\r\n;");
      if (temp.IsEmpty())
        continue;

      temp.Split(false, components, " ", "\t", "=", "\r");

      if (components.GetCount() != 2)
      {
        plLog::Error("Malformed shader state assignment: '{0}'", temp);
        continue;
      }

      out_materialConfig[components[0]] = components[1];
    }

    return PLASMA_SUCCESS;
  }
} // namespace

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plMaterialAssetPreview, 1)
  PLASMA_ENUM_CONSTANT(plMaterialAssetPreview::Ball),
  PLASMA_ENUM_CONSTANT(plMaterialAssetPreview::Sphere),
  PLASMA_ENUM_CONSTANT(plMaterialAssetPreview::Box),
  PLASMA_ENUM_CONSTANT(plMaterialAssetPreview::Plane),
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plMaterialShaderMode, 1)
PLASMA_ENUM_CONSTANTS(plMaterialShaderMode::BaseMaterial, plMaterialShaderMode::File, plMaterialShaderMode::Custom)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMaterialAssetProperties, 4, plRTTIDefaultAllocator<plMaterialAssetProperties>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_ACCESSOR_PROPERTY("ShaderMode", plMaterialShaderMode, GetShaderMode, SetShaderMode),
    PLASMA_ACCESSOR_PROPERTY("BaseMaterial", GetBaseMaterial, SetBaseMaterial)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material", plDependencyFlags::Transform | plDependencyFlags::Thumbnail | plDependencyFlags::Package)),
    PLASMA_ACCESSOR_PROPERTY("Surface", GetSurface, SetSurface)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface", plDependencyFlags::Package)),
    PLASMA_ACCESSOR_PROPERTY("Shader", GetShader, SetShader)->AddAttributes(new plFileBrowserAttribute("Select Shader", "*.plShader", "CustomAction_CreateShaderFromTemplate")),
    // This property holds the phantom shader properties type so it is only used in the object graph but not actually in the instance of this object.
    PLASMA_ACCESSOR_PROPERTY("ShaderProperties", GetShaderProperties, SetShaderProperties)->AddFlags(plPropertyFlags::PointerOwner)->AddAttributes(new plContainerAttribute(false, false, false)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMaterialAssetDocument, 7, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plUuid plMaterialAssetDocument::s_LitBaseMaterial;
plUuid plMaterialAssetDocument::s_LitAlphaTextBaseMaterial;
plUuid plMaterialAssetDocument::s_NeutralNormalMap;

void plMaterialAssetProperties::SetBaseMaterial(const char* szBaseMaterial)
{
  if (m_sBaseMaterial == szBaseMaterial)
    return;

  m_sBaseMaterial = szBaseMaterial;

  // If no doc is present, we are de-serializing the document so do nothing yet.
  if (!m_pDocument)
    return;
  if (m_pDocument->GetCommandHistory()->IsInUndoRedo())
    return;
  m_pDocument->SetBaseMaterial(m_sBaseMaterial);
}

const char* plMaterialAssetProperties::GetBaseMaterial() const
{
  return m_sBaseMaterial;
}

void plMaterialAssetProperties::SetShader(const char* szShader)
{
  if (m_sShader != szShader)
  {
    m_sShader = szShader;
    UpdateShader();
  }
}

const char* plMaterialAssetProperties::GetShader() const
{
  return m_sShader;
}

void plMaterialAssetProperties::SetShaderProperties(plReflectedClass* pProperties)
{
  // This property represents the phantom shader type, so it is never actually used.
}

plReflectedClass* plMaterialAssetProperties::GetShaderProperties() const
{
  // This property represents the phantom shader type, so it is never actually used.
  return nullptr;
}

void plMaterialAssetProperties::SetShaderMode(plEnum<plMaterialShaderMode> mode)
{
  if (m_ShaderMode == mode)
    return;

  m_ShaderMode = mode;

  // If no doc is present, we are de-serializing the document so do nothing yet.
  if (!m_pDocument)
    return;
  plCommandHistory* pHistory = m_pDocument->GetCommandHistory();
  plObjectAccessorBase* pAccessor = m_pDocument->GetObjectAccessor();
  // Do not make new commands if we got here in a response to an undo / redo action.
  if (pHistory->IsInUndoRedo())
    return;

  plStringBuilder tmp;

  switch (m_ShaderMode)
  {
    case plMaterialShaderMode::BaseMaterial:
    {
      pAccessor->SetValue(m_pDocument->GetPropertyObject(), "BaseMaterial", "").AssertSuccess();
      pAccessor->SetValue(m_pDocument->GetPropertyObject(), "Shader", "").AssertSuccess();
    }
    break;
    case plMaterialShaderMode::File:
    {
      pAccessor->SetValue(m_pDocument->GetPropertyObject(), "BaseMaterial", "").AssertSuccess();
      pAccessor->SetValue(m_pDocument->GetPropertyObject(), "Shader", "").AssertSuccess();
    }
    break;
    case plMaterialShaderMode::Custom:
    {
      pAccessor->SetValue(m_pDocument->GetPropertyObject(), "BaseMaterial", "").AssertSuccess();
      pAccessor->SetValue(m_pDocument->GetPropertyObject(), "Shader", plConversionUtils::ToString(m_pDocument->GetGuid(), tmp).GetData()).AssertSuccess();
    }
    break;
  }
}

void plMaterialAssetProperties::SetDocument(plMaterialAssetDocument* pDocument)
{
  m_pDocument = pDocument;
  if (!m_sBaseMaterial.IsEmpty())
  {
    m_pDocument->SetBaseMaterial(m_sBaseMaterial);
  }
  UpdateShader(true);
}


void plMaterialAssetProperties::UpdateShader(bool bForce)
{
  // If no doc is present, we are de-serializing the document so do nothing yet.
  if (!m_pDocument)
    return;

  plCommandHistory* pHistory = m_pDocument->GetCommandHistory();
  // Do not make new commands if we got here in a response to an undo / redo action.
  if (pHistory->IsInUndoRedo())
    return;

  PLASMA_ASSERT_DEBUG(pHistory->IsInTransaction(), "Missing undo scope on stack.");

  plDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();

  // TODO: If m_sShader is empty, we need to get the shader of our base material and use that one instead
  // for the code below. The type name is the clean path to the shader at the moment.
  plStringBuilder sShaderPath = ResolveRelativeShaderPath();
  sShaderPath.MakeCleanPath();

  if (sShaderPath.IsEmpty())
  {
    // No shader, delete any existing properties object.
    if (pPropObject)
    {
      DeleteProperties();
    }
  }
  else
  {
    if (pPropObject)
    {
      // We already have a shader properties object, test whether
      // it has a different type than the newly set shader. The type name
      // is the clean path to the shader at the moment.
      const plRTTI* pType = pPropObject->GetTypeAccessor().GetType();
      if (sShaderPath != pType->GetTypeName() || bForce) // TODO: Is force even necessary anymore?
      {
        // Shader has changed, delete old and create new one.
        DeleteProperties();
        CreateProperties(sShaderPath);
      }
      else
      {
        // Same shader but it could have changed so try to update it anyway.
        plShaderTypeRegistry::GetSingleton()->GetShaderType(sShaderPath);
      }
    }

    if (!pPropObject)
    {
      // No shader properties exist yet, so create a new one.
      CreateProperties(sShaderPath);
    }
  }
}

void plMaterialAssetProperties::DeleteProperties()
{
  SaveOldValues();
  plCommandHistory* pHistory = m_pDocument->GetCommandHistory();
  plDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();
  plRemoveObjectCommand cmd;
  cmd.m_Object = pPropObject->GetGuid();
  auto res = pHistory->AddCommand(cmd);
  PLASMA_ASSERT_DEV(res.m_Result.Succeeded(), "Removal of old properties should never fail.");
}

void plMaterialAssetProperties::CreateProperties(const char* szShaderPath)
{
  plCommandHistory* pHistory = m_pDocument->GetCommandHistory();

  const plRTTI* pType = plShaderTypeRegistry::GetSingleton()->GetShaderType(szShaderPath);
  if (!pType && m_ShaderMode == plMaterialShaderMode::Custom)
  {
    // Force generate if custom shader is missing
    plAssetFileHeader AssetHeader;
    AssetHeader.SetFileHashAndVersion(0, m_pDocument->GetAssetTypeVersion());
    m_pDocument->RecreateVisualShaderFile(AssetHeader).LogFailure();
    pType = plShaderTypeRegistry::GetSingleton()->GetShaderType(szShaderPath);
  }

  if (pType)
  {
    plAddObjectCommand cmd;
    cmd.m_pType = pType;
    cmd.m_sParentProperty = "ShaderProperties";
    cmd.m_Parent = m_pDocument->GetPropertyObject()->GetGuid();
    cmd.m_NewObjectGuid = cmd.m_Parent;
    cmd.m_NewObjectGuid.CombineWithSeed(plUuid::MakeStableUuidFromString("ShaderProperties"));

    auto res = pHistory->AddCommand(cmd);
    PLASMA_ASSERT_DEV(res.m_Result.Succeeded(), "Addition of new properties should never fail.");
    LoadOldValues();
  }
}

void plMaterialAssetProperties::SaveOldValues()
{
  plDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();
  if (pPropObject)
  {
    const plIReflectedTypeAccessor& accessor = pPropObject->GetTypeAccessor();
    const plRTTI* pType = accessor.GetType();
    plHybridArray<const plAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetCategory() == plPropertyCategory::Member)
      {
        m_CachedProperties[pProp->GetPropertyName()] = accessor.GetValue(pProp->GetPropertyName());
      }
    }
  }
}

void plMaterialAssetProperties::LoadOldValues()
{
  plDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();
  plCommandHistory* pHistory = m_pDocument->GetCommandHistory();
  if (pPropObject)
  {
    const plIReflectedTypeAccessor& accessor = pPropObject->GetTypeAccessor();
    const plRTTI* pType = accessor.GetType();
    plHybridArray<const plAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetCategory() == plPropertyCategory::Member)
      {
        plString sPropName = pProp->GetPropertyName();
        auto it = m_CachedProperties.Find(sPropName);
        if (it.IsValid())
        {
          if (it.Value() != accessor.GetValue(sPropName.GetData()))
          {
            plSetObjectPropertyCommand cmd;
            cmd.m_Object = pPropObject->GetGuid();
            cmd.m_sProperty = sPropName;
            cmd.m_NewValue = it.Value();

            // Do not check for success, if a cached value failed to apply, simply ignore it.
            pHistory->AddCommand(cmd).AssertSuccess();
          }
        }
      }
    }
  }
}

plString plMaterialAssetProperties::GetAutoGenShaderPathAbs() const
{
  plAssetDocumentManager* pManager = plDynamicCast<plAssetDocumentManager*>(m_pDocument->GetDocumentManager());
  plString sAbsOutputPath = pManager->GetAbsoluteOutputFileName(m_pDocument->GetAssetDocumentTypeDescriptor(), m_pDocument->GetDocumentPath(), plMaterialAssetDocumentManager::s_szShaderOutputTag);
  return sAbsOutputPath;
}

void plMaterialAssetProperties::PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plMaterialAssetProperties>())
  {
    plInt64 shaderMode = e.m_pObject->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<plInt64>();

    auto& props = *e.m_pPropertyStates;

    if (shaderMode == plMaterialShaderMode::File)
      props["Shader"].m_Visibility = plPropertyUiState::Default;
    else
      props["Shader"].m_Visibility = plPropertyUiState::Invisible;

    if (shaderMode == plMaterialShaderMode::BaseMaterial)
      props["BaseMaterial"].m_Visibility = plPropertyUiState::Default;
    else
      props["BaseMaterial"].m_Visibility = plPropertyUiState::Invisible;
  }
}

plString plMaterialAssetProperties::ResolveRelativeShaderPath() const
{
  bool isGuid = plConversionUtils::IsStringUuid(m_sShader);

  if (isGuid)
  {
    plUuid guid = plConversionUtils::ConvertStringToUuid(m_sShader);
    auto pAsset = plAssetCurator::GetSingleton()->GetSubAsset(guid);
    if (pAsset)
    {
      PLASMA_ASSERT_DEV(pAsset->m_pAssetInfo->GetManager() == m_pDocument->GetDocumentManager(), "Referenced shader via guid by this material is not of type material asset (plMaterialShaderMode::Custom).");

      plStringBuilder sProjectDir = plAssetCurator::GetSingleton()->FindDataDirectoryForAsset(pAsset->m_pAssetInfo->m_Path);
      plStringBuilder sResult = pAsset->m_pAssetInfo->GetManager()->GetRelativeOutputFileName(m_pDocument->GetAssetDocumentTypeDescriptor(), sProjectDir, pAsset->m_pAssetInfo->m_Path, plMaterialAssetDocumentManager::s_szShaderOutputTag);

      sResult.Prepend("AssetCache/");
      return sResult;
    }
    else
    {
      plLog::Error("Could not resolve guid '{0}' for the material shader.", m_sShader);
      return "";
    }
  }
  else
  {
    return m_sShader;
  }

  return m_sShader;
}

//////////////////////////////////////////////////////////////////////////

plMaterialAssetDocument::plMaterialAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plMaterialAssetProperties>(PLASMA_DEFAULT_NEW(plMaterialObjectManager), sDocumentPath, plAssetDocEngineConnection::Simple, true)
{
  plQtEditorApp::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plMaterialAssetDocument::EditorEventHandler, this));
}

plMaterialAssetDocument::~plMaterialAssetDocument()
{
  plQtEditorApp::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plMaterialAssetDocument::EditorEventHandler, this));
}

void plMaterialAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  {
    plCommandHistory* pHistory = GetCommandHistory();
    pHistory->StartTransaction("Update Material Shader");
    GetProperties()->SetDocument(this);
    pHistory->FinishTransaction();
  }

  bool bSetModified = false;

  // The above command may patch the doc with the newest shader properties so we need to clear the undo history here.
  GetCommandHistory()->ClearUndoHistory();
  SetModified(bSetModified);
}

plDocumentObject* plMaterialAssetDocument::GetShaderPropertyObject()
{
  plDocumentObject* pObject = GetObjectManager()->GetRootObject()->GetChildren()[0];
  plIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  plUuid propObjectGuid = accessor.GetValue("ShaderProperties").ConvertTo<plUuid>();
  plDocumentObject* pPropObject = nullptr;
  if (propObjectGuid.IsValid())
  {
    pPropObject = GetObjectManager()->GetObject(propObjectGuid);
  }
  return pPropObject;
}

const plDocumentObject* plMaterialAssetDocument::GetShaderPropertyObject() const
{
  return const_cast<plMaterialAssetDocument*>(this)->GetShaderPropertyObject();
}

void plMaterialAssetDocument::SetBaseMaterial(const char* szBaseMaterial)
{
  plDocumentObject* pObject = GetPropertyObject();
  auto pAssetInfo = plAssetCurator::GetSingleton()->FindSubAsset(szBaseMaterial);
  if (pAssetInfo == nullptr)
  {
    plDeque<const plDocumentObject*> sel;
    sel.PushBack(pObject);
    UnlinkPrefabs(sel);
  }
  else
  {
    const plStringBuilder& sNewBase = plPrefabCache::GetSingleton()->GetCachedPrefabDocument(pAssetInfo->m_Data.m_Guid);
    const plAbstractObjectGraph* pBaseGraph = plPrefabCache::GetSingleton()->GetCachedPrefabGraph(pAssetInfo->m_Data.m_Guid);

    plUuid seed = GetSeedFromBaseMaterial(pBaseGraph);
    if (sNewBase.IsEmpty() || !pBaseGraph || !seed.IsValid())
    {
      plLog::Error("The selected base material '{0}' is not a valid material file!", szBaseMaterial);
      return;
    }

    {
      auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObject->GetGuid());

      if (pMeta->m_CreateFromPrefab != pAssetInfo->m_Data.m_Guid)
      {
        pMeta->m_sBasePrefab = sNewBase;
        pMeta->m_CreateFromPrefab = pAssetInfo->m_Data.m_Guid;
        pMeta->m_PrefabSeedGuid = seed;
      }
      m_DocumentObjectMetaData->EndModifyMetaData(plDocumentObjectMetaData::PrefabFlag);
    }
    UpdatePrefabs();
  }
}

plUuid plMaterialAssetDocument::GetSeedFromBaseMaterial(const plAbstractObjectGraph* pBaseGraph)
{
  if (!pBaseGraph)
    return plUuid();

  plUuid instanceGuid = GetPropertyObject()->GetGuid();
  plUuid baseGuid = plMaterialAssetDocument::GetMaterialNodeGuid(*pBaseGraph);
  if (baseGuid.IsValid())
  {
    // Create seed that converts base guid into instance guid
    instanceGuid.RevertCombinationWithSeed(baseGuid);
    return instanceGuid;
  }

  return plUuid();
}

plUuid plMaterialAssetDocument::GetMaterialNodeGuid(const plAbstractObjectGraph& graph)
{
  for (auto it = graph.GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetType() == plGetStaticRTTI<plMaterialAssetProperties>()->GetTypeName())
    {
      return it.Value()->GetGuid();
    }
  }
  return plUuid();
}

void plMaterialAssetDocument::UpdatePrefabObject(plDocumentObject* pObject, const plUuid& PrefabAsset, const plUuid& PrefabSeed, plStringView sBasePrefab)
{
  // Base
  plAbstractObjectGraph baseGraph;
  plPrefabUtils::LoadGraph(baseGraph, sBasePrefab);
  baseGraph.PruneGraph(GetMaterialNodeGuid(baseGraph));

  // NewBase
  const plStringBuilder& sLeft = plPrefabCache::GetSingleton()->GetCachedPrefabDocument(PrefabAsset);
  const plAbstractObjectGraph* pLeftGraph = plPrefabCache::GetSingleton()->GetCachedPrefabGraph(PrefabAsset);
  plAbstractObjectGraph leftGraph;
  if (pLeftGraph)
  {
    pLeftGraph->Clone(leftGraph);
  }
  else
  {
    plStringBuilder sGuid;
    plConversionUtils::ToString(PrefabAsset, sGuid);
    plLog::Error("Can't update prefab, new base graph does not exist: {0}", sGuid);
    return;
  }
  leftGraph.PruneGraph(GetMaterialNodeGuid(leftGraph));

  // Instance
  plAbstractObjectGraph rightGraph;
  {
    plDocumentObjectConverterWriter writer(&rightGraph, pObject->GetDocumentObjectManager());
    writer.AddObjectToGraph(pObject);
    rightGraph.ReMapNodeGuids(PrefabSeed, true);
  }

  // Merge diffs relative to base
  plDeque<plAbstractGraphDiffOperation> mergedDiff;
  plPrefabUtils::Merge(baseGraph, leftGraph, rightGraph, mergedDiff);

  // Skip 'ShaderMode' as it should not be inherited, and 'ShaderProperties' is being set by the 'Shader' property
  plDeque<plAbstractGraphDiffOperation> cleanedDiff;
  for (const plAbstractGraphDiffOperation& op : mergedDiff)
  {
    if (op.m_Operation == plAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      if (op.m_sProperty == "ShaderMode" || op.m_sProperty == "ShaderProperties")
        continue;

      cleanedDiff.PushBack(op);
    }
  }

  // Apply diff to base, making it the new instance
  baseGraph.ApplyDiff(cleanedDiff);

  // Do not allow 'Shader' to be overridden, always use the prefab template version.
  if (plAbstractObjectNode* pNode = leftGraph.GetNode(GetMaterialNodeGuid(leftGraph)))
  {
    if (auto pProp = pNode->FindProperty("Shader"))
    {
      if (plAbstractObjectNode* pNodeBase = baseGraph.GetNode(GetMaterialNodeGuid(baseGraph)))
      {
        pNodeBase->ChangeProperty("Shader", pProp->m_Value);
      }
    }
  }

  // Create a new diff that changes our current instance to the new instance
  plDeque<plAbstractGraphDiffOperation> newInstanceToCurrentInstance;
  baseGraph.CreateDiffWithBaseGraph(rightGraph, newInstanceToCurrentInstance);
  if (false)
  {
    plFileWriter file;
    file.Open("C:\\temp\\Material - diff.txt").IgnoreResult();

    plStringBuilder sDiff;
    sDiff.Append("######## New Instance To Instance #######\n");
    plPrefabUtils::WriteDiff(newInstanceToCurrentInstance, sDiff);
    file.WriteBytes(sDiff.GetData(), sDiff.GetElementCount()).IgnoreResult();
  }
  // Apply diff to current instance
  // Shader needs to be set first
  for (plUInt32 i = 0; i < newInstanceToCurrentInstance.GetCount(); ++i)
  {
    if (newInstanceToCurrentInstance[i].m_sProperty == "Shader")
    {
      plAbstractGraphDiffOperation op = newInstanceToCurrentInstance[i];
      newInstanceToCurrentInstance.RemoveAtAndCopy(i);
      newInstanceToCurrentInstance.Insert(op, 0);
      break;
    }
  }
  for (const plAbstractGraphDiffOperation& op : newInstanceToCurrentInstance)
  {
    if (op.m_Operation == plAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      // Never change this material's mode, as it should not be inherited from prefab base
      if (op.m_sProperty == "ShaderMode")
        continue;

      // these properties may not exist and we do not want to change them either
      if (op.m_sProperty == "MetaBasePrefab" || op.m_sProperty == "MetaPrefabSeed" || op.m_sProperty == "MetaFromPrefab")
        continue;

      plSetObjectPropertyCommand cmd;
      cmd.m_Object = op.m_Node;
      cmd.m_Object.CombineWithSeed(PrefabSeed);
      cmd.m_NewValue = op.m_Value;
      cmd.m_sProperty = op.m_sProperty;

      auto pObj = GetObjectAccessor()->GetObject(cmd.m_Object);
      if (!pObj)
        continue;

      auto pProp = pObj->GetType()->FindPropertyByName(op.m_sProperty);
      if (!pProp)
        continue;

      if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
        continue;

      GetCommandHistory()->AddCommand(cmd).AssertSuccess();
    }
  }

  // Update prefab meta data
  {
    auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObject->GetGuid());
    pMeta->m_CreateFromPrefab = PrefabAsset; // Should not change
    pMeta->m_PrefabSeedGuid = PrefabSeed;    // Should not change
    pMeta->m_sBasePrefab = sLeft;

    m_DocumentObjectMetaData->EndModifyMetaData(plDocumentObjectMetaData::PrefabFlag);
  }
}

class plVisualShaderErrorLog : public plLogInterface
{
public:
  plStringBuilder m_sResult;
  plResult m_Status;

  plVisualShaderErrorLog()
    : m_Status(PLASMA_SUCCESS)
  {
  }

  virtual void HandleLogMessage(const plLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case plLogMsgType::ErrorMsg:
        m_Status = PLASMA_FAILURE;
        m_sResult.Append("Error: ", le.m_sText, "\n");
        break;

      case plLogMsgType::SeriousWarningMsg:
      case plLogMsgType::WarningMsg:
        m_sResult.Append("Warning: ", le.m_sText, "\n");
        break;

      default:
        return;
    }
  }
};

plTransformStatus plMaterialAssetDocument::InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  if (sOutputTag.IsEqual(plMaterialAssetDocumentManager::s_szShaderOutputTag))
  {
    plStatus ret = RecreateVisualShaderFile(AssetHeader);

    if (transformFlags.IsSet(plTransformFlags::ForceTransform))
    {
      plMaterialVisualShaderEvent e;

      if (GetProperties()->m_ShaderMode == plMaterialShaderMode::Custom)
      {
        e.m_Type = plMaterialVisualShaderEvent::TransformFailed;
        e.m_sTransformError = ret.m_sMessage;

        if (ret.Succeeded())
        {
          e.m_Type = plMaterialVisualShaderEvent::TransformSucceeded;
          plStringBuilder sAutoGenShader = GetProperties()->GetAutoGenShaderPathAbs();

          QStringList arguments;
          plStringBuilder temp;

          arguments << "-project";
          arguments << QString::fromUtf8(plToolsProject::GetSingleton()->GetProjectDirectory().GetData());

          arguments << "-shader";
          arguments << QString::fromUtf8(sAutoGenShader.GetData());

          arguments << "-platform";
          arguments << "DX11_SM50"; /// \todo Rendering platform is currently hardcoded

          // determine the permutation variables that should get fixed values
          {
            // m_sCheckPermutations are just all fixed perm vars from every node in the VS
            plStringBuilder temp = m_sCheckPermutations;
            plDeque<plStringView> perms;
            temp.Split(false, perms, "\n");

            // remove duplicates
            plSet<plString> uniquePerms;
            for (const plStringView& perm : perms)
            {
              uniquePerms.Insert(perm);
            }

            // pass permutation variable definitions to the compiler: "SOME_VAR=SOME_VAL"
            arguments << "-perm";
            for (auto it = uniquePerms.GetIterator(); it.IsValid(); ++it)
            {
              arguments << it.Key().GetData();
            }
          }

          plVisualShaderErrorLog log;

          ret = plQtEditorApp::GetSingleton()->ExecuteTool("ShaderCompiler", arguments, 60, &log);
          if (ret.Failed())
          {
            e.m_Type = plMaterialVisualShaderEvent::TransformFailed;
            e.m_sTransformError = ret.m_sMessage;
          }
          else
          {
            e.m_Type = log.m_Status.Succeeded() ? plMaterialVisualShaderEvent::TransformSucceeded : plMaterialVisualShaderEvent::TransformFailed;
            e.m_sTransformError = log.m_sResult;
            plLog::Info("Compiled Visual Shader.");
          }
        }
      }
      else
      {
        e.m_Type = plMaterialVisualShaderEvent::VisualShaderNotUsed;
      }

      if (e.m_Type == plMaterialVisualShaderEvent::TransformFailed)
      {
        TagVisualShaderFileInvalid(pAssetProfile, e.m_sTransformError);
      }

      m_VisualShaderEvents.Broadcast(e);
    }

    return ret;
  }
  else
  {
    return SUPER::InternalTransformAsset(szTargetFile, sOutputTag, pAssetProfile, AssetHeader, transformFlags);
  }
}

plTransformStatus plMaterialAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  PLASMA_ASSERT_DEV(sOutputTag.IsEmpty(), "Additional output '{0}' not implemented!", sOutputTag);

  return WriteMaterialAsset(stream, pAssetProfile, true);
}

plTransformStatus plMaterialAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  return plAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
}

void plMaterialAssetDocument::InternalGetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const
{
  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void plMaterialAssetDocument::AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void plMaterialAssetDocument::RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void plMaterialAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (GetProperties()->m_ShaderMode != plMaterialShaderMode::BaseMaterial)
  {
    // remove base material dependency, if it isn't used
    pInfo->m_TransformDependencies.Remove(GetProperties()->GetBaseMaterial());
    pInfo->m_ThumbnailDependencies.Remove(GetProperties()->GetBaseMaterial());
  }

  if (GetProperties()->m_ShaderMode != plMaterialShaderMode::File)
  {
    const bool bInUseByBaseMaterial = GetProperties()->m_ShaderMode == plMaterialShaderMode::BaseMaterial && plStringUtils::IsEqual(GetProperties()->GetShader(), GetProperties()->GetBaseMaterial());

    // remove shader file dependency, if it isn't used and differs from the base material
    if (!bInUseByBaseMaterial)
    {
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetShader());
      pInfo->m_ThumbnailDependencies.Remove(GetProperties()->GetShader());
    }
  }

  if (GetProperties()->m_ShaderMode == plMaterialShaderMode::Custom)
  {
    // We write our own guid into the shader field so BaseMaterial materials can find the shader file.
    // This would cause us to have a dependency to ourselves so we need to remove it.
    plStringBuilder tmp;
    pInfo->m_TransformDependencies.Remove(plConversionUtils::ToString(GetGuid(), tmp));
    pInfo->m_ThumbnailDependencies.Remove(plConversionUtils::ToString(GetGuid(), tmp));

    plVisualShaderCodeGenerator codeGen;

    plSet<plString> cfgFiles;
    codeGen.DetermineConfigFileDependencies(static_cast<const plDocumentNodeManager*>(GetObjectManager()), cfgFiles);

    for (const auto& sCfgFile : cfgFiles)
    {
      pInfo->m_TransformDependencies.Insert(sCfgFile);
    }

    pInfo->m_Outputs.Insert(plMaterialAssetDocumentManager::s_szShaderOutputTag);

    /// \todo The Visual Shader node configuration files would need to be a dependency of the auto-generated shader.
  }
}

plStatus plMaterialAssetDocument::WriteMaterialAsset(plStreamWriter& inout_stream0, const plPlatformProfile* pAssetProfile, bool bEmbedLowResData) const
{
  const plMaterialAssetProperties* pProp = GetProperties();

  plStringBuilder sValue;

  // now generate the .plMaterialBin file
  {
    const plUInt8 uiVersion = 7;

    inout_stream0 << uiVersion;

    plUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    uiCompressionMode = 1;
    plCompressedStreamWriterZstd stream(&inout_stream0, 0, plCompressedStreamWriterZstd::Compression::Average);
#else
    plStreamWriter& stream = stream0;
#endif

    inout_stream0 << uiCompressionMode;

    stream << pProp->m_sBaseMaterial;
    stream << pProp->m_sSurface;

    plString sRelativeShaderPath = pProp->ResolveRelativeShaderPath();
    stream << sRelativeShaderPath;

    plHybridArray<const plAbstractProperty*, 16> Textures2D;
    plHybridArray<const plAbstractProperty*, 16> TexturesCube;
    plHybridArray<const plAbstractProperty*, 16> Permutations;
    plHybridArray<const plAbstractProperty*, 16> Constants;

    const plDocumentObject* pObject = GetShaderPropertyObject();
    if (pObject != nullptr)
    {
      bool hasBaseMaterial = plPrefabUtils::GetPrefabRoot(pObject, *m_DocumentObjectMetaData).IsValid();
      auto pType = pObject->GetTypeAccessor().GetType();
      plHybridArray<const plAbstractProperty*, 32> properties;
      pType->GetAllProperties(properties);

      plHybridArray<plPropertySelection, 1> selection;
      selection.PushBack({pObject, plVariant()});
      plDefaultObjectState defaultState(GetObjectAccessor(), selection.GetArrayPtr());

      for (auto pProp : properties)
      {
        if (hasBaseMaterial && defaultState.IsDefaultValue(pProp))
          continue;

        const plCategoryAttribute* pCategory = pProp->GetAttributeByType<plCategoryAttribute>();

        PLASMA_ASSERT_DEBUG(pCategory, "Category cannot be null for a shader property");
        if (pCategory == nullptr)
          continue;

        if (plStringUtils::IsEqual(pCategory->GetCategory(), "Texture 2D"))
        {
          Textures2D.PushBack(pProp);
        }
        else if (plStringUtils::IsEqual(pCategory->GetCategory(), "Texture Cube"))
        {
          TexturesCube.PushBack(pProp);
        }
        else if (plStringUtils::IsEqual(pCategory->GetCategory(), "Permutation"))
        {
          Permutations.PushBack(pProp);
        }
        else if (plStringUtils::IsEqual(pCategory->GetCategory(), "Constant"))
        {
          Constants.PushBack(pProp);
        }
        else
        {
          PLASMA_REPORT_FAILURE("Invalid shader property type '{0}'", pCategory->GetCategory());
        }
      }
    }

    // write out the permutation variables
    {
      const plUInt16 uiPermVars = Permutations.GetCount();
      stream << uiPermVars;

      for (auto pProp : Permutations)
      {
        const char* szName = pProp->GetPropertyName();
        if (pProp->GetSpecificType()->GetVariantType() == plVariantType::Bool)
        {
          sValue = pObject->GetTypeAccessor().GetValue(szName).Get<bool>() ? "TRUE" : "FALSE";
        }
        else if (pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags))
        {
          plReflectionUtils::EnumerationToString(pProp->GetSpecificType(), pObject->GetTypeAccessor().GetValue(szName).ConvertTo<plInt64>(), sValue, plReflectionUtils::EnumConversionMode::ValueNameOnly);
        }
        else
        {
          PLASMA_REPORT_FAILURE("Invalid shader permutation property type '{0}'", pProp->GetSpecificType()->GetTypeName());
        }

        stream << szName;
        stream << sValue;
      }
    }

    // write out the 2D textures
    {
      const plUInt16 uiTextures = Textures2D.GetCount();
      stream << uiTextures;

      for (auto pProp : Textures2D)
      {
        const char* szName = pProp->GetPropertyName();
        sValue = pObject->GetTypeAccessor().GetValue(szName).ConvertTo<plString>();

        stream << szName;
        stream << sValue;
      }
    }

    // write out the Cube textures
    {
      const plUInt16 uiTextures = TexturesCube.GetCount();
      stream << uiTextures;

      for (auto pProp : TexturesCube)
      {
        const char* szName = pProp->GetPropertyName();
        sValue = pObject->GetTypeAccessor().GetValue(szName).ConvertTo<plString>();

        stream << szName;
        stream << sValue;
      }
    }

    // write out the constants
    {
      const plUInt16 uiConstants = Constants.GetCount();
      stream << uiConstants;

      for (auto pProp : Constants)
      {
        const char* szName = pProp->GetPropertyName();
        plVariant value = pObject->GetTypeAccessor().GetValue(szName);

        stream << szName;
        stream << value;
      }
    }

    // render data category
    {
      plVariantDictionary materialConfig;
      if (pObject != nullptr)
      {
        PLASMA_SUCCEED_OR_RETURN(ParseMaterialConfig(sRelativeShaderPath, pObject, materialConfig));
      }

      plVariant renderDataCategory;
      materialConfig.TryGetValue("RenderDataCategory", renderDataCategory);

      stream << renderDataCategory.ConvertTo<plString>();
    }

    // find and embed low res texture data
    {
      if (bEmbedLowResData)
      {
        plStringBuilder sFilename, sResourceName;
        plDynamicArray<plUInt32> content;

        // embed 2D texture data
        for (auto prop : Textures2D)
        {
          const char* szName = prop->GetPropertyName();
          sValue = pObject->GetTypeAccessor().GetValue(szName).ConvertTo<plString>();

          if (sValue.IsEmpty())
            continue;

          sResourceName = sValue;

          auto asset = plAssetCurator::GetSingleton()->FindSubAsset(sValue);
          if (!asset.isValid())
            continue;

          sValue = asset->m_pAssetInfo->GetManager()->GetAbsoluteOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, asset->m_pAssetInfo->m_Path, "", pAssetProfile);

          sFilename = sValue.GetFileName();
          sFilename.Append("-lowres");

          sValue.ChangeFileName(sFilename);

          plFileReader file;
          if (file.Open(sValue).Failed())
            continue;

          content.SetCountUninitialized(file.GetFileSize());

          file.ReadBytes(content.GetData(), content.GetCount());

          stream << sResourceName;
          stream << content.GetCount();
          PLASMA_SUCCEED_OR_RETURN(stream.WriteBytes(content.GetData(), content.GetCount()));
        }
      }

      // marker: end of embedded data
      stream << "";
    }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    PLASMA_SUCCEED_OR_RETURN(stream.FinishCompressedStream());

    plLog::Dev("Compressed material data from {0} KB to {1} KB ({2}%%)", plArgF((float)stream.GetUncompressedSize() / 1024.0f, 1), plArgF((float)stream.GetCompressedSize() / 1024.0f, 1), plArgF(100.0f * stream.GetCompressedSize() / stream.GetUncompressedSize(), 1));
#endif
  }

  return plStatus(PLASMA_SUCCESS);
}

void plMaterialAssetDocument::TagVisualShaderFileInvalid(const plPlatformProfile* pAssetProfile, const char* szError)
{
  if (GetProperties()->m_ShaderMode != plMaterialShaderMode::Custom)
    return;

  plAssetDocumentManager* pManager = plDynamicCast<plAssetDocumentManager*>(GetDocumentManager());
  plString sAutoGenShader = pManager->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), plMaterialAssetDocumentManager::s_szShaderOutputTag);

  plStringBuilder all;

  // read shader source
  {
    plFileReader file;
    if (file.Open(sAutoGenShader).Failed())
      return;

    all.ReadAll(file);
  }

  all.PrependFormat("/*\n{0}\n*/\n", szError);

  // write adjusted shader source
  {
    plFileWriter fileOut;
    if (fileOut.Open(sAutoGenShader).Failed())
      return;

    fileOut.WriteBytes(all.GetData(), all.GetElementCount()).IgnoreResult();
  }
}

plStatus plMaterialAssetDocument::RecreateVisualShaderFile(const plAssetFileHeader& assetHeader)
{
  if (GetProperties()->m_ShaderMode != plMaterialShaderMode::Custom)
  {
    return plStatus(PLASMA_SUCCESS);
  }

  plAssetDocumentManager* pManager = plDynamicCast<plAssetDocumentManager*>(GetDocumentManager());
  plString sAutoGenShader = pManager->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), plMaterialAssetDocumentManager::s_szShaderOutputTag);

  plVisualShaderCodeGenerator codeGen;

  PLASMA_SUCCEED_OR_RETURN(codeGen.GenerateVisualShader(static_cast<const plDocumentNodeManager*>(GetObjectManager()), m_sCheckPermutations));

  plFileWriter file;
  if (file.Open(sAutoGenShader).Succeeded())
  {
    plStringBuilder shader = codeGen.GetFinalShaderCode();
    shader.PrependFormat("//{0}|{1}\n", assetHeader.GetFileHash(), assetHeader.GetFileVersion());

    PLASMA_SUCCEED_OR_RETURN(file.WriteBytes(shader.GetData(), shader.GetElementCount()));
    file.Close();

    InvalidateCachedShader();

    return plStatus(PLASMA_SUCCESS);
  }
  else
    return plStatus(plFmt("Failed to write auto-generated shader to '{0}'", sAutoGenShader));
}

void plMaterialAssetDocument::InvalidateCachedShader()
{
  plAssetDocumentManager* pManager = plDynamicCast<plAssetDocumentManager*>(GetDocumentManager());
  plString sShader;

  if (GetProperties()->m_ShaderMode == plMaterialShaderMode::Custom)
  {
    sShader = pManager->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), plMaterialAssetDocumentManager::s_szShaderOutputTag);
  }
  else
  {
    sShader = GetProperties()->GetShader();
  }

  // This should update the shader parameter section in all affected materials
  plShaderTypeRegistry::GetSingleton()->GetShaderType(sShader);
}

void plMaterialAssetDocument::EditorEventHandler(const plEditorAppEvent& e)
{
  if (e.m_Type == plEditorAppEvent::Type::ReloadResources)
  {
    InvalidateCachedShader();
  }
}

static void MarkReachableNodes(plMap<const plDocumentObject*, bool>& ref_allNodes, const plDocumentObject* pRoot, plDocumentNodeManager* pNodeManager)
{
  if (ref_allNodes[pRoot])
    return;

  ref_allNodes[pRoot] = true;

  auto allInputs = pNodeManager->GetInputPins(pRoot);

  // we start at the final output, so use the inputs on a node and then walk backwards
  for (auto& pTargetPin : allInputs)
  {
    auto connections = pNodeManager->GetConnections(*pTargetPin);

    // all incoming connections at the input pin, there should only be one though
    for (const plConnection* const pConnection : connections)
    {
      // output pin on other node connecting to this node
      const plPin& sourcePin = pConnection->GetSourcePin();

      // recurse from here
      MarkReachableNodes(ref_allNodes, sourcePin.GetParent(), pNodeManager);
    }
  }
}

void plMaterialAssetDocument::RemoveDisconnectedNodes()
{
  plDocumentNodeManager* pNodeManager = static_cast<plDocumentNodeManager*>(GetObjectManager());

  const plDocumentObject* pRoot = pNodeManager->GetRootObject();
  const plRTTI* pNodeBaseRtti = plVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

  const plHybridArray<plDocumentObject*, 8>& children = pRoot->GetChildren();
  plMap<const plDocumentObject*, bool> AllNodes;

  for (plUInt32 i = 0; i < children.GetCount(); ++i)
  {
    if (children[i]->GetType()->IsDerivedFrom(pNodeBaseRtti))
    {
      AllNodes[children[i]] = false;
    }
  }

  for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
  {
    // skip nodes that have already been marked
    if (it.Value())
      continue;

    auto pDesc = plVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(it.Key()->GetType());

    if (pDesc->m_NodeType == plVisualShaderNodeType::Main)
    {
      MarkReachableNodes(AllNodes, it.Key(), pNodeManager);
    }
  }

  // now purge all nodes that haven't been reached
  {
    auto pHistory = GetCommandHistory();
    pHistory->StartTransaction("Purge unreachable nodes");

    for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
    {
      // skip nodes that have been marked
      if (it.Value())
        continue;

      plRemoveNodeCommand rem;
      rem.m_Object = it.Key()->GetGuid();

      pHistory->AddCommand(rem).AssertSuccess();
    }

    pHistory->FinishTransaction();
  }
}

plUuid plMaterialAssetDocument::GetLitBaseMaterial()
{
  if (!s_LitBaseMaterial.IsValid())
  {
    static const char* szLitMaterialAssetPath = plMaterialResource::GetDefaultMaterialFileName(plMaterialResource::DefaultMaterialType::Lit);
    auto assetInfo = plAssetCurator::GetSingleton()->FindSubAsset(szLitMaterialAssetPath);
    if (assetInfo)
      s_LitBaseMaterial = assetInfo->m_Data.m_Guid;
    else
      plLog::Error("Can't find default lit material {0}", szLitMaterialAssetPath);
  }
  return s_LitBaseMaterial;
}

plUuid plMaterialAssetDocument::GetLitAlphaTestBaseMaterial()
{
  if (!s_LitAlphaTextBaseMaterial.IsValid())
  {
    static const char* szLitAlphaTestMaterialAssetPath = plMaterialResource::GetDefaultMaterialFileName(plMaterialResource::DefaultMaterialType::LitAlphaTest);
    auto assetInfo = plAssetCurator::GetSingleton()->FindSubAsset(szLitAlphaTestMaterialAssetPath);
    if (assetInfo)
      s_LitAlphaTextBaseMaterial = assetInfo->m_Data.m_Guid;
    else
      plLog::Error("Can't find default lit alpha test material {0}", szLitAlphaTestMaterialAssetPath);
  }
  return s_LitAlphaTextBaseMaterial;
}

plUuid plMaterialAssetDocument::GetNeutralNormalMap()
{
  if (!s_NeutralNormalMap.IsValid())
  {
    static const char* szNeutralNormalMapAssetPath = "Base/Textures/NeutralNormal.plTextureAsset";
    auto assetInfo = plAssetCurator::GetSingleton()->FindSubAsset(szNeutralNormalMapAssetPath);
    if (assetInfo)
      s_NeutralNormalMap = assetInfo->m_Data.m_Guid;
    else
      plLog::Error("Can't find neutral normal map texture {0}", szNeutralNormalMapAssetPath);
  }
  return s_NeutralNormalMap;
}

void plMaterialAssetDocument::GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/plEditor.NodeGraph");
}

bool plMaterialAssetDocument::CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_sMimeType) const
{
  out_sMimeType = "application/plEditor.NodeGraph";

  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool plMaterialAssetDocument::Paste(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, plStringView sMimeType)
{
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, plQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plMaterialAssetPropertiesPatch_1_2 : public plGraphPatch
{
public:
  plMaterialAssetPropertiesPatch_1_2()
    : plGraphPatch("plMaterialAssetProperties", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Shader Mode", "ShaderMode");
    pNode->RenameProperty("Base Material", "BaseMaterial");
  }
};

plMaterialAssetPropertiesPatch_1_2 g_plMaterialAssetPropertiesPatch_1_2;


class plMaterialAssetPropertiesPatch_2_3 : public plGraphPatch
{
public:
  plMaterialAssetPropertiesPatch_2_3()
    : plGraphPatch("plMaterialAssetProperties", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pBaseMatProp = pNode->FindProperty("BaseMaterial");
    auto* pShaderModeProp = pNode->FindProperty("ShaderMode");
    if (pBaseMatProp && pBaseMatProp->m_Value.IsA<plString>())
    {
      if (!pBaseMatProp->m_Value.Get<plString>().IsEmpty())
      {
        // BaseMaterial is set
        pNode->ChangeProperty("ShaderMode", (plInt32)plMaterialShaderMode::BaseMaterial);
      }
      else
      {
        pNode->ChangeProperty("ShaderMode", (plInt32)plMaterialShaderMode::File);
      }
    }
  }
};

plMaterialAssetPropertiesPatch_2_3 g_plMaterialAssetPropertiesPatch_2_3;
