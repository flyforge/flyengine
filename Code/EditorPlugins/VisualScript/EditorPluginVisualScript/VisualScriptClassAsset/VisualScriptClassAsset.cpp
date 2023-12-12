#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginVisualScript/VisualScriptClassAsset/VisualScriptClassAsset.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptCompiler.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plVisualScriptClassAssetProperties, 1, plRTTIDefaultAllocator<plVisualScriptClassAssetProperties>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("BaseClass", m_sBaseClass)->AddAttributes(new plDefaultValueAttribute(plStringView("Component")), new plDynamicStringEnumAttribute("ScriptBaseClasses")),
    PLASMA_ARRAY_MEMBER_PROPERTY("Variables", m_Variables),
    PLASMA_MEMBER_PROPERTY("DumpAST", m_bDumpAST),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plVisualScriptClassAssetDocument, 3, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plVisualScriptClassAssetDocument::plVisualScriptClassAssetDocument(const char* szDocumentPath)
  : plSimpleAssetDocument<plVisualScriptClassAssetProperties>(PLASMA_DEFAULT_NEW(plVisualScriptNodeManager), szDocumentPath, plAssetDocEngineConnection::None)
{
  m_pObjectAccessor = PLASMA_DEFAULT_NEW(plNodeCommandAccessor, GetCommandHistory());
}

plTransformStatus plVisualScriptClassAssetDocument::InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  auto pManager = static_cast<plVisualScriptNodeManager*>(GetObjectManager());

  const auto& children = pManager->GetRootObject()->GetChildren();
  plHashedString sBaseClass = pManager->GetScriptBaseClass();

  plStringBuilder sBaseClassName = sBaseClass.GetView();
  if (plRTTI::FindTypeByName(sBaseClassName) == nullptr)
  {
    sBaseClassName.Prepend("pl");
    if (plRTTI::FindTypeByName(sBaseClassName) == nullptr)
    {
      return plStatus(plFmt("Invalid base class '{}'", sBaseClassName));
    }
  }

  plStringView sScriptClassName = plPathUtils::GetFileName(GetDocumentPath());

  plVisualScriptCompiler compiler;
  compiler.InitModule(sBaseClassName, sScriptClassName);

  plHybridArray<const plVisualScriptPin*, 16> pins;
  for (const plDocumentObject* pObject : children)
  {
    if (pManager->IsNode(pObject) == false)
      continue;

    auto pNodeDesc = plVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc == nullptr)
      return plStatus(PLASMA_FAILURE);

    if (pManager->IsFilteredByBaseClass(pObject->GetType(), *pNodeDesc, sBaseClass, true))
      continue;

    if (plVisualScriptNodeDescription::Type::IsEntry(pNodeDesc->m_Type))
    {
      pManager->GetOutputExecutionPins(pObject, pins);
      if (pins.IsEmpty())
        continue;

      if (pManager->GetConnections(*pins[0]).IsEmpty())
        continue;

      plStringView sFunctionName = plVisualScriptNodeManager::GetNiceFunctionName(pObject);
      PLASMA_SUCCEED_OR_RETURN(compiler.AddFunction(sFunctionName, pObject));
    }
  }

  plStringBuilder sDumpPath;
  if (GetProperties()->m_bDumpAST)
  {
    sDumpPath.Format(":appdata/{}_AST.dgml", sScriptClassName);
  }
  PLASMA_SUCCEED_OR_RETURN(compiler.Compile(sDumpPath));

  auto& compiledModule = compiler.GetCompiledModule();
  PLASMA_SUCCEED_OR_RETURN(compiledModule.Serialize(stream));

  return plStatus(PLASMA_SUCCESS);
}

void plVisualScriptClassAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  plExposedParameters* pExposedParams = PLASMA_DEFAULT_NEW(plExposedParameters);

  for (const auto& v : GetProperties()->m_Variables)
  {
    if (v.m_bExpose == false)
      continue;

    plExposedParameter* param = PLASMA_DEFAULT_NEW(plExposedParameter);
    param->m_sName = v.m_sName.GetString();
    param->m_DefaultValue = v.m_DefaultValue;

    pExposedParams->m_Parameters.PushBack(param);
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

void plVisualScriptClassAssetDocument::InternalGetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const
{
  auto pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void plVisualScriptClassAssetDocument::AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const auto pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void plVisualScriptClassAssetDocument::RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  auto pManager = static_cast<plDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void plVisualScriptClassAssetDocument::GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/plEditor.VisualScriptClassGraph");
}

bool plVisualScriptClassAssetDocument::CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_MimeType) const
{
  out_MimeType = "application/plEditor.VisualScriptClassGraph";

  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool plVisualScriptClassAssetDocument::Paste(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, plQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
