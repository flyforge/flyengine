#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/RTTI.h>
#include <Foundation/Types/VariantTypeRegistry.h>

PLASMA_IMPLEMENT_SINGLETON(plVariantTypeRegistry);

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Foundation, VariantTypeRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Reflection"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    PLASMA_DEFAULT_NEW(plVariantTypeRegistry);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plVariantTypeRegistry * pDummy = plVariantTypeRegistry::GetSingleton();
    PLASMA_DEFAULT_DELETE(pDummy);
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plVariantTypeRegistry::plVariantTypeRegistry()
  : m_SingletonRegistrar(this)
{
  plPlugin::Events().AddEventHandler(plMakeDelegate(&plVariantTypeRegistry::PluginEventHandler, this));

  UpdateTypes();
}

plVariantTypeRegistry::~plVariantTypeRegistry()
{
  plPlugin::Events().RemoveEventHandler(plMakeDelegate(&plVariantTypeRegistry::PluginEventHandler, this));
}

const plVariantTypeInfo* plVariantTypeRegistry::FindVariantTypeInfo(const plRTTI* pType) const
{
  const plVariantTypeInfo* pTypeInfo = nullptr;
  m_TypeInfos.TryGetValue(pType, pTypeInfo);
  return pTypeInfo;
}

void plVariantTypeRegistry::PluginEventHandler(const plPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case plPluginEvent::AfterLoadingBeforeInit:
    case plPluginEvent::AfterUnloading:
      UpdateTypes();
      break;
    default:
      break;
  }
}

void plVariantTypeRegistry::UpdateTypes()
{
  m_TypeInfos.Clear();
  plVariantTypeInfo* pInstance = plVariantTypeInfo::GetFirstInstance();

  while (pInstance)
  {
    PLASMA_ASSERT_DEV(pInstance->GetType()->GetAllocator()->CanAllocate(), "Custom type '{0}' needs to be allocatable.", pInstance->GetType()->GetTypeName());

    m_TypeInfos.Insert(pInstance->GetType(), pInstance);
    pInstance = pInstance->GetNextInstance();
  }
}

//////////////////////////////////////////////////////////////////////////

PLASMA_ENUMERABLE_CLASS_IMPLEMENTATION(plVariantTypeInfo);

plVariantTypeInfo::plVariantTypeInfo() = default;


PLASMA_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_VariantTypeRegistry);
