#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/ShaderTypeRegistry.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

PLASMA_IMPLEMENT_SINGLETON(plShaderTypeRegistry);

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, ShaderTypeRegistry)

BEGIN_SUBSYSTEM_DEPENDENCIES
  "ReflectedTypeManager"
END_SUBSYSTEM_DEPENDENCIES

ON_CORESYSTEMS_STARTUP
{
  PLASMA_DEFAULT_NEW(plShaderTypeRegistry);
}

ON_CORESYSTEMS_SHUTDOWN
{
  plShaderTypeRegistry* pDummy = plShaderTypeRegistry::GetSingleton();
  PLASMA_DEFAULT_DELETE(pDummy);
}

ON_HIGHLEVELSYSTEMS_STARTUP
{
}

ON_HIGHLEVELSYSTEMS_SHUTDOWN
{
}

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  struct PermutationVarConfig
  {
    plVariant m_DefaultValue;
    const plRTTI* m_pType;
  };

  static plHashTable<plString, PermutationVarConfig> s_PermutationVarConfigs;
  static plHashTable<plString, const plRTTI*> s_EnumTypes;

  const plRTTI* GetPermutationType(const plShaderParser::ParameterDefinition& def)
  {
    PLASMA_ASSERT_DEV(def.m_sType.IsEqual("Permutation"), "");

    PermutationVarConfig* pConfig = nullptr;
    if (s_PermutationVarConfigs.TryGetValue(def.m_sName, pConfig))
    {
      return pConfig->m_pType;
    }

    plStringBuilder sTemp;
    sTemp.Format("Shaders/PermutationVars/{0}.plPermVar", def.m_sName);

    plString sPath = sTemp;
    plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);

    plFileReader file;
    if (file.Open(sPath).Failed())
    {
      return nullptr;
    }

    sTemp.ReadAll(file);

    plVariant defaultValue;
    plShaderParser::EnumDefinition enumDefinition;

    plShaderParser::ParsePermutationVarConfig(sTemp, defaultValue, enumDefinition);
    if (defaultValue.IsValid())
    {
      pConfig = &(s_PermutationVarConfigs[def.m_sName]);
      pConfig->m_DefaultValue = defaultValue;

      if (defaultValue.IsA<bool>())
      {
        pConfig->m_pType = plGetStaticRTTI<bool>();
      }
      else
      {
        plReflectedTypeDescriptor descEnum;
        descEnum.m_sTypeName = def.m_sName;
        descEnum.m_sPluginName = "ShaderTypes";
        descEnum.m_sParentTypeName = plGetStaticRTTI<plEnumBase>()->GetTypeName();
        descEnum.m_Flags = plTypeFlags::IsEnum | plTypeFlags::Phantom;
        descEnum.m_uiTypeVersion = 1;

        plArrayPtr<plPropertyAttribute* const> noAttributes;

        plStringBuilder sEnumName;
        sEnumName.Format("{0}::Default", def.m_sName);

        descEnum.m_Properties.PushBack(plReflectedPropertyDescriptor(sEnumName, defaultValue.Get<plUInt32>(), noAttributes));

        for (const auto& ev : enumDefinition.m_Values)
        {
          plStringBuilder sEnumName;
          sEnumName.Format("{0}::{1}", def.m_sName, ev.m_sValueName);

          descEnum.m_Properties.PushBack(plReflectedPropertyDescriptor(sEnumName, ev.m_iValueValue, noAttributes));
        }

        pConfig->m_pType = plPhantomRttiManager::RegisterType(descEnum);
      }

      return pConfig->m_pType;
    }

    return nullptr;
  }

  const plRTTI* GetEnumType(const plShaderParser::EnumDefinition& def)
  {
    const plRTTI* pType = nullptr;
    if (s_EnumTypes.TryGetValue(def.m_sName, pType))
    {
      return pType;
    }

    plReflectedTypeDescriptor descEnum;
    descEnum.m_sTypeName = def.m_sName;
    descEnum.m_sPluginName = "ShaderTypes";
    descEnum.m_sParentTypeName = plGetStaticRTTI<plEnumBase>()->GetTypeName();
    descEnum.m_Flags = plTypeFlags::IsEnum | plTypeFlags::Phantom;
    descEnum.m_uiTypeVersion = 1;

    plArrayPtr<plPropertyAttribute* const> noAttributes;

    plStringBuilder sEnumName;
    sEnumName.Format("{0}::Default", def.m_sName);

    descEnum.m_Properties.PushBack(plReflectedPropertyDescriptor(sEnumName, def.m_uiDefaultValue, noAttributes));

    for (const auto& ev : def.m_Values)
    {
      plStringBuilder sEnumName;
      sEnumName.Format("{0}::{1}", def.m_sName, ev.m_sValueName);

      descEnum.m_Properties.PushBack(plReflectedPropertyDescriptor(sEnumName, ev.m_iValueValue, noAttributes));
    }

    pType = plPhantomRttiManager::RegisterType(descEnum);

    s_EnumTypes.Insert(def.m_sName, pType);

    return pType;
  }

  const plRTTI* GetType(const plShaderParser::ParameterDefinition& def)
  {
    if (def.m_pType != nullptr)
    {
      return def.m_pType;
    }

    if (def.m_sType.IsEqual("Permutation"))
    {
      return GetPermutationType(def);
    }

    const plRTTI* pType = nullptr;
    s_EnumTypes.TryGetValue(def.m_sType, pType);

    return pType;
  }

  void AddAttributes(plShaderParser::ParameterDefinition& def, const plRTTI* pType, plHybridArray<const plPropertyAttribute*, 2>& attributes)
  {
    if (def.m_sType.StartsWith_NoCase("texture"))
    {
      if (def.m_sType.IsEqual("Texture2D"))
      {
        attributes.PushBack(PLASMA_DEFAULT_NEW(plCategoryAttribute, "Texture 2D"));
        attributes.PushBack(PLASMA_DEFAULT_NEW(plAssetBrowserAttribute, "CompatibleAsset_Texture_2D"));
      }
      else if (def.m_sType.IsEqual("Texture3D"))
      {
        attributes.PushBack(PLASMA_DEFAULT_NEW(plCategoryAttribute, "Texture 3D"));
        attributes.PushBack(PLASMA_DEFAULT_NEW(plAssetBrowserAttribute, "CompatibleAsset_Texture_3D"));
      }
      else if (def.m_sType.IsEqual("TextureCube"))
      {
        attributes.PushBack(PLASMA_DEFAULT_NEW(plCategoryAttribute, "Texture Cube"));
        attributes.PushBack(PLASMA_DEFAULT_NEW(plAssetBrowserAttribute, "CompatibleAsset_Texture_Cube"));
      }
    }
    else if (def.m_sType.StartsWith_NoCase("permutation"))
    {
      attributes.PushBack(PLASMA_DEFAULT_NEW(plCategoryAttribute, "Permutation"));
    }
    else
    {
      attributes.PushBack(PLASMA_DEFAULT_NEW(plCategoryAttribute, "Constant"));
    }

    for (auto& attributeDef : def.m_Attributes)
    {
      if (attributeDef.m_sType.IsEqual("Default") && attributeDef.m_Values.GetCount() >= 1)
      {
        if (pType == plGetStaticRTTI<plColor>())
        {
          // always expose the alpha channel for color properties
          attributes.PushBack(PLASMA_DEFAULT_NEW(plExposeColorAlphaAttribute));

          // patch default type, VSE writes float4 instead of color
          if (attributeDef.m_Values[0].GetType() == plVariantType::Vector4)
          {
            plVec4 v = attributeDef.m_Values[0].Get<plVec4>();
            attributeDef.m_Values[0] = plColor(v.x, v.y, v.z, v.w);
          }
        }

        attributes.PushBack(PLASMA_DEFAULT_NEW(plDefaultValueAttribute, attributeDef.m_Values[0]));
      }
      else if (attributeDef.m_sType.IsEqual("Clamp") && attributeDef.m_Values.GetCount() >= 2)
      {
        attributes.PushBack(PLASMA_DEFAULT_NEW(plClampValueAttribute, attributeDef.m_Values[0], attributeDef.m_Values[1]));
      }
      else if (attributeDef.m_sType.IsEqual("Group"))
      {
        if (attributeDef.m_Values.GetCount() >= 1 && attributeDef.m_Values[0].CanConvertTo<plString>())
        {
          attributes.PushBack(PLASMA_DEFAULT_NEW(plGroupAttribute, attributeDef.m_Values[0].ConvertTo<plString>()));
        }
        else
        {
          attributes.PushBack(PLASMA_DEFAULT_NEW(plGroupAttribute));
        }
      }
    }
  }
} // namespace

plShaderTypeRegistry::plShaderTypeRegistry()
  : m_SingletonRegistrar(this)
{
  plShaderTypeRegistry::GetSingleton();
  plReflectedTypeDescriptor desc;
  desc.m_sTypeName = "plShaderTypeBase";
  desc.m_sPluginName = "ShaderTypes";
  desc.m_sParentTypeName = plGetStaticRTTI<plReflectedClass>()->GetTypeName();
  desc.m_Flags = plTypeFlags::Phantom | plTypeFlags::Abstract | plTypeFlags::Class;
  desc.m_uiTypeVersion = 2;

  m_pBaseType = plPhantomRttiManager::RegisterType(desc);

  plPhantomRttiManager::s_Events.AddEventHandler(plMakeDelegate(&plShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
}


plShaderTypeRegistry::~plShaderTypeRegistry()
{
  plPhantomRttiManager::s_Events.RemoveEventHandler(plMakeDelegate(&plShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
}

const plRTTI* plShaderTypeRegistry::GetShaderType(plStringView sShaderPath0)
{
  if (sShaderPath0.IsEmpty())
    return nullptr;

  plStringBuilder sShaderPath = sShaderPath0;
  sShaderPath.MakeCleanPath();

  if (sShaderPath.IsAbsolutePath())
  {
    if (!plQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sShaderPath))
    {
      plLog::Error("Could not make shader path '{0}' relative!", sShaderPath);
    }
  }

  auto it = m_ShaderTypes.Find(sShaderPath);
  if (it.IsValid())
  {
    plFileStats Stats;
    if (plOSFile::GetFileStats(it.Value().m_sAbsShaderPath, Stats).Succeeded() &&
        !Stats.m_LastModificationTime.Compare(it.Value().m_fileModifiedTime, plTimestamp::CompareMode::FileTimeEqual))
    {
      UpdateShaderType(it.Value());
    }
  }
  else
  {
    plStringBuilder sAbsPath = sShaderPath0;
    {
      if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
      {
        plLog::Warning("Can't make path absolute: '{0}'", sShaderPath0);
        return nullptr;
      }
      sAbsPath.MakeCleanPath();
    }

    it = m_ShaderTypes.Insert(sShaderPath, ShaderData());
    it.Value().m_sShaderPath = sShaderPath;
    it.Value().m_sAbsShaderPath = sAbsPath;
    UpdateShaderType(it.Value());
  }

  return it.Value().m_pType;
}

void plShaderTypeRegistry::UpdateShaderType(ShaderData& data)
{
  PLASMA_LOG_BLOCK("Updating Shader Parameters", data.m_sShaderPath.GetData());

  plHybridArray<plShaderParser::ParameterDefinition, 16> parameters;
  plHybridArray<plShaderParser::EnumDefinition, 4> enumDefinitions;

  {
    plFileStats Stats;
    bool bStat = plOSFile::GetFileStats(data.m_sAbsShaderPath, Stats).Succeeded();

    plFileReader file;
    if (!bStat || file.Open(data.m_sAbsShaderPath).Failed())
    {
      plLog::Error("Can't update shader '{0}' type information, the file can't be opened.", data.m_sShaderPath);
      return;
    }

    plShaderParser::ParseMaterialParameterSection(file, parameters, enumDefinitions);
    data.m_fileModifiedTime = Stats.m_LastModificationTime;
  }

  plReflectedTypeDescriptor desc;
  desc.m_sTypeName = data.m_sShaderPath;
  desc.m_sPluginName = "ShaderTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = plTypeFlags::Phantom | plTypeFlags::Class;
  desc.m_uiTypeVersion = 2;

  for (auto& enumDef : enumDefinitions)
  {
    GetEnumType(enumDef);
  }

  for (auto& parameter : parameters)
  {
    const plRTTI* pType = GetType(parameter);
    if (pType == nullptr)
    {
      continue;
    }

    plBitflags<plPropertyFlags> flags = plPropertyFlags::Phantom;
    if (pType->IsDerivedFrom<plEnumBase>())
      flags |= plPropertyFlags::IsEnum;
    if (pType->IsDerivedFrom<plBitflagsBase>())
      flags |= plPropertyFlags::Bitflags;
    if (plReflectionUtils::IsBasicType(pType))
      flags |= plPropertyFlags::StandardType;

    plReflectedPropertyDescriptor propDesc(plPropertyCategory::Member, parameter.m_sName, pType->GetTypeName(), flags);

    AddAttributes(parameter, pType, propDesc.m_Attributes);

    desc.m_Properties.PushBack(propDesc);
  }

  // Register and return the phantom type. If the type already exists this will update the type
  // and patch any existing instances of it so they should show up in the prop grid right away.
  plPhantomRttiManager::s_Events.RemoveEventHandler(plMakeDelegate(&plShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
  {
    // We do not want to listen to type changes that we triggered ourselves.
    data.m_pType = plPhantomRttiManager::RegisterType(desc);
  }
  plPhantomRttiManager::s_Events.AddEventHandler(plMakeDelegate(&plShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
}

void plShaderTypeRegistry::PhantomTypeRegistryEventHandler(const plPhantomRttiManagerEvent& e)
{
  if (e.m_Type == plPhantomRttiManagerEvent::Type::TypeAdded)
  {
    if (e.m_pChangedType->GetParentType() == m_pBaseType)
    {
      GetShaderType(e.m_pChangedType->GetTypeName());
    }
  }
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

/// \brief Changes the base class of all shader types to plShaderTypeBase (version 1) and
/// sets their own version to 2.
class plShaderTypePatch_1_2 : public plGraphPatch
{
public:
  plShaderTypePatch_1_2()
    : plGraphPatch(nullptr, 2, plGraphPatch::PatchType::GraphPatch)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode*) const override
  {
    plString sDescTypeName = plGetStaticRTTI<plReflectedTypeDescriptor>()->GetTypeName();

    auto& nodes = pGraph->GetAllNodes();
    bool bNeedAddBaseClass = false;
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      plAbstractObjectNode* pNode = it.Value();
      if (pNode->GetType() == sDescTypeName)
      {
        auto* pTypeProperty = pNode->FindProperty("TypeName");
        if (plStringUtils::EndsWith(pTypeProperty->m_Value.Get<plString>(), ".plShader"))
        {
          auto* pTypeVersionProperty = pNode->FindProperty("TypeVersion");
          auto* pParentTypeProperty = pNode->FindProperty("ParentTypeName");
          if (pTypeVersionProperty->m_Value == 1)
          {
            pParentTypeProperty->m_Value = "plShaderTypeBase";
            pTypeVersionProperty->m_Value = (plUInt32)2;
            bNeedAddBaseClass = true;
          }
        }
      }
    }

    if (bNeedAddBaseClass)
    {
      plRttiConverterContext context;
      plRttiConverterWriter rttiConverter(pGraph, &context, true, true);

      plReflectedTypeDescriptor desc;
      desc.m_sTypeName = "plShaderTypeBase";
      desc.m_sPluginName = "ShaderTypes";
      desc.m_sParentTypeName = plGetStaticRTTI<plReflectedClass>()->GetTypeName();
      desc.m_Flags = plTypeFlags::Phantom | plTypeFlags::Abstract | plTypeFlags::Class;
      desc.m_uiTypeVersion = 1;

      context.RegisterObject(plUuid::StableUuidForString(desc.m_sTypeName.GetData()), plGetStaticRTTI<plReflectedTypeDescriptor>(), &desc);
      rttiConverter.AddObjectToGraph(plGetStaticRTTI<plReflectedTypeDescriptor>(), &desc);
    }
  }
};

plShaderTypePatch_1_2 g_plShaderTypePatch_1_2;

// TODO: Increase plShaderTypeBase version to 2 and implement enum renames, see plReflectedPropertyDescriptorPatch_1_2
class plShaderBaseTypePatch_1_2 : public plGraphPatch
{
public:
  plShaderBaseTypePatch_1_2()
    : plGraphPatch("plShaderTypeBase", 2)
  {
  }

  static void FixEnumString(plStringBuilder& sValue, const char* szName)
  {
    if (sValue.StartsWith(szName))
      sValue.Shrink(plStringUtils::GetCharacterCount(szName), 0);

    if (sValue.StartsWith("::"))
      sValue.Shrink(2, 0);

    if (sValue.StartsWith(szName))
      sValue.Shrink(plStringUtils::GetCharacterCount(szName), 0);

    if (sValue.StartsWith("_"))
      sValue.Shrink(1, 0);

    sValue.PrependFormat("{0}::{0}_", szName);
  }

  void FixEnum(plAbstractObjectNode* pNode, const char* szEnum) const
  {
    if (plAbstractObjectNode::Property* pProp = pNode->FindProperty(szEnum))
    {
      plStringBuilder sValue = pProp->m_Value.Get<plString>();
      FixEnumString(sValue, szEnum);
      pProp->m_Value = sValue.GetData();
    }
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    FixEnum(pNode, "SHADING_MODE");
    FixEnum(pNode, "BLEND_MODE");
    FixEnum(pNode, "RENDER_PASS");
  }
};

plShaderBaseTypePatch_1_2 g_plShaderBaseTypePatch_1_2;
