#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/ExposedParametersTypeRegistry.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

PLASMA_IMPLEMENT_SINGLETON(plExposedParametersTypeRegistry);

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, ExposedParametersTypeRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager", "AssetCurator"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    PLASMA_DEFAULT_NEW(plExposedParametersTypeRegistry);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plExposedParametersTypeRegistry* pDummy = plExposedParametersTypeRegistry::GetSingleton();
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

plExposedParametersTypeRegistry::plExposedParametersTypeRegistry()
  : m_SingletonRegistrar(this)
{
  plReflectedTypeDescriptor desc;
  desc.m_sTypeName = "plExposedParametersTypeBase";
  desc.m_sPluginName = "ExposedParametersTypes";
  desc.m_sParentTypeName = plGetStaticRTTI<plReflectedClass>()->GetTypeName();
  desc.m_Flags = plTypeFlags::Phantom | plTypeFlags::Abstract | plTypeFlags::Class;
  desc.m_uiTypeVersion = 0;

  m_pBaseType = plPhantomRttiManager::RegisterType(desc);

  plAssetCurator::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plExposedParametersTypeRegistry::AssetCuratorEventHandler, this));
  plPhantomRttiManager::s_Events.AddEventHandler(plMakeDelegate(&plExposedParametersTypeRegistry::PhantomTypeRegistryEventHandler, this));
}


plExposedParametersTypeRegistry::~plExposedParametersTypeRegistry()
{
  plAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plExposedParametersTypeRegistry::AssetCuratorEventHandler, this));
  plPhantomRttiManager::s_Events.RemoveEventHandler(plMakeDelegate(&plExposedParametersTypeRegistry::PhantomTypeRegistryEventHandler, this));
}

const plRTTI* plExposedParametersTypeRegistry::GetExposedParametersType(const char* szResource)
{
  if (plStringUtils::IsNullOrEmpty(szResource))
    return nullptr;

  const auto asset = plAssetCurator::GetSingleton()->FindSubAsset(szResource);
  if (!asset)
    return nullptr;

  auto params = asset->m_pAssetInfo->m_Info->GetMetaInfo<plExposedParameters>();
  if (!params)
    return nullptr;

  auto it = m_ShaderTypes.Find(asset->m_Data.m_Guid);
  if (it.IsValid())
  {
    if (!it.Value().m_bUpToDate)
    {
      UpdateExposedParametersType(it.Value(), *params);
    }
  }
  else
  {
    it = m_ShaderTypes.Insert(asset->m_Data.m_Guid, ParamData());
    it.Value().m_SubAssetGuid = asset->m_Data.m_Guid;
    UpdateExposedParametersType(it.Value(), *params);
  }

  return it.Value().m_pType;
}

void plExposedParametersTypeRegistry::UpdateExposedParametersType(ParamData& data, const plExposedParameters& params)
{
  plStringBuilder name;
  name.Format("plExposedParameters_{0}", data.m_SubAssetGuid);
  PLASMA_LOG_BLOCK("Updating Type", name.GetData());
  plReflectedTypeDescriptor desc;
  desc.m_sTypeName = name;
  desc.m_sPluginName = "ExposedParametersTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = plTypeFlags::Phantom | plTypeFlags::Class;
  desc.m_uiTypeVersion = 2;

  for (const auto* parameter : params.m_Parameters)
  {
    const plRTTI* pType = plReflectionUtils::GetTypeFromVariant(parameter->m_DefaultValue);
    if (!parameter->m_sType.IsEmpty())
    {
      if (const plRTTI* pType2 = plRTTI::FindTypeByName(parameter->m_sType))
        pType = pType2;
    }
    if (pType == nullptr)
      continue;

    plBitflags<plPropertyFlags> flags = plPropertyFlags::Phantom;
    if (pType->IsDerivedFrom<plEnumBase>())
      flags |= plPropertyFlags::IsEnum;
    if (pType->IsDerivedFrom<plBitflagsBase>())
      flags |= plPropertyFlags::Bitflags;
    if (plReflectionUtils::IsBasicType(pType))
      flags |= plPropertyFlags::StandardType;
    else
      flags |= plPropertyFlags::Class;

    plReflectedPropertyDescriptor propDesc(plPropertyCategory::Member, parameter->m_sName, pType->GetTypeName(), flags);
    for (auto attrib : parameter->m_Attributes)
    {
      propDesc.m_Attributes.PushBack(plReflectionSerializer::Clone(attrib));
    }
    desc.m_Properties.PushBack(propDesc);
  }

  // Register and return the phantom type. If the type already exists this will update the type
  // and patch any existing instances of it so they should show up in the prop grid right away.
  {
    // This fkt is called by the property grid, but calling RegisterType will update the property grid
    // and we will recurse into this. So we listen for the plPhantomRttiManager events to fill out
    // the data.m_pType in it to make sure recursion into GetExposedParametersType does not return a nullptr.
    m_pAboutToBeRegistered = &data;
    data.m_bUpToDate = true;
    data.m_pType = plPhantomRttiManager::RegisterType(desc);
    m_pAboutToBeRegistered = nullptr;
  }
}

void plExposedParametersTypeRegistry::AssetCuratorEventHandler(const plAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case plAssetCuratorEvent::Type::AssetRemoved:
    {
      // Ignore for now, doesn't hurt. Removing types is more hassle than it is worth.
      if (auto* data = m_ShaderTypes.GetValue(e.m_AssetGuid))
      {
        data->m_bUpToDate = false;
      }
    }
    break;
    case plAssetCuratorEvent::Type::AssetListReset:
    {
      for (auto it = m_ShaderTypes.GetIterator(); it.IsValid(); ++it)
      {
        it.Value().m_bUpToDate = false;
      }
    }
    break;
    case plAssetCuratorEvent::Type::AssetUpdated:
    {
      if (auto* data = m_ShaderTypes.GetValue(e.m_AssetGuid))
      {
        data->m_bUpToDate = false;
        if (auto params = e.m_pInfo->m_pAssetInfo->m_Info->GetMetaInfo<plExposedParameters>())
          UpdateExposedParametersType(*data, *params);
      }
    }
    break;
    default:
      break;
  }
}

void plExposedParametersTypeRegistry::PhantomTypeRegistryEventHandler(const plPhantomRttiManagerEvent& e)
{
  if (e.m_Type == plPhantomRttiManagerEvent::Type::TypeAdded || e.m_Type == plPhantomRttiManagerEvent::Type::TypeChanged)
  {
    if (e.m_pChangedType->GetParentType() == m_pBaseType && m_pAboutToBeRegistered)
    {
      // We listen for the plPhantomRttiManager events to fill out the m_pType pointer. This is needed as otherwise
      // Recursion into GetExposedParametersType would return a nullptr.
      m_pAboutToBeRegistered->m_pType = e.m_pChangedType;
    }
  }
}
