#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/PhantomRtti.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

plCopyOnBroadcastEvent<const plPhantomRttiManagerEvent&> plPhantomRttiManager::s_Events;

plHashTable<plStringView, plPhantomRTTI*> plPhantomRttiManager::s_NameToPhantom;

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ReflectedTypeManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plPhantomRttiManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plPhantomRttiManager::Shutdown();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// plPhantomRttiManager public functions
////////////////////////////////////////////////////////////////////////

const plRTTI* plPhantomRttiManager::RegisterType(plReflectedTypeDescriptor& ref_desc)
{
  PLASMA_PROFILE_SCOPE("RegisterType");
  const plRTTI* pType = plRTTI::FindTypeByName(ref_desc.m_sTypeName);
  plPhantomRTTI* pPhantom = nullptr;
  s_NameToPhantom.TryGetValue(ref_desc.m_sTypeName, pPhantom);

  // concrete type !
  if (pPhantom == nullptr && pType != nullptr)
  {
    return pType;
  }

  if (pPhantom != nullptr && pPhantom->IsEqualToDescriptor(ref_desc))
    return pPhantom;

  if (pPhantom == nullptr)
  {
    pPhantom = PLASMA_DEFAULT_NEW(plPhantomRTTI, ref_desc.m_sTypeName.GetData(), plRTTI::FindTypeByName(ref_desc.m_sParentTypeName), 0,
      ref_desc.m_uiTypeVersion, plVariantType::Invalid, ref_desc.m_Flags, ref_desc.m_sPluginName.GetData());

    pPhantom->SetProperties(ref_desc.m_Properties);
    pPhantom->SetAttributes(ref_desc.m_Attributes);
    pPhantom->SetFunctions(ref_desc.m_Functions);
    pPhantom->SetupParentHierarchy();

    s_NameToPhantom[pPhantom->GetTypeName()] = pPhantom;

    plPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = plPhantomRttiManagerEvent::Type::TypeAdded;
    s_Events.Broadcast(msg, 1); /// \todo Had to increase the recursion depth to allow registering phantom types that are based on actual
                                /// types coming from the engine process
  }
  else
  {
    pPhantom->UpdateType(ref_desc);

    plPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = plPhantomRttiManagerEvent::Type::TypeChanged;
    s_Events.Broadcast(msg, 1);
  }

  return pPhantom;
}

bool plPhantomRttiManager::UnregisterType(const plRTTI* pRtti)
{
  plPhantomRTTI* pPhantom = nullptr;
  s_NameToPhantom.TryGetValue(pRtti->GetTypeName(), pPhantom);

  if (pPhantom == nullptr)
    return false;

  {
    plPhantomRttiManagerEvent msg;
    msg.m_pChangedType = pPhantom;
    msg.m_Type = plPhantomRttiManagerEvent::Type::TypeRemoved;
    s_Events.Broadcast(msg);
  }

  s_NameToPhantom.Remove(pPhantom->GetTypeName());

  PLASMA_DEFAULT_DELETE(pPhantom);
  return true;
}

////////////////////////////////////////////////////////////////////////
// plPhantomRttiManager private functions
////////////////////////////////////////////////////////////////////////

void plPhantomRttiManager::PluginEventHandler(const plPluginEvent& e)
{
  if (e.m_EventType == plPluginEvent::Type::BeforeUnloading)
  {
    while (!s_NameToPhantom.IsEmpty())
    {
      UnregisterType(s_NameToPhantom.GetIterator().Value());
    }

    PLASMA_ASSERT_DEV(s_NameToPhantom.IsEmpty(), "plPhantomRttiManager::Shutdown: Removal of types failed!");
  }
}

void plPhantomRttiManager::Startup()
{
  plPlugin::Events().AddEventHandler(&plPhantomRttiManager::PluginEventHandler);
}


void plPhantomRttiManager::Shutdown()
{
  plPlugin::Events().RemoveEventHandler(&plPhantomRttiManager::PluginEventHandler);

  while (!s_NameToPhantom.IsEmpty())
  {
    UnregisterType(s_NameToPhantom.GetIterator().Value());
  }

  PLASMA_ASSERT_DEV(s_NameToPhantom.IsEmpty(), "plPhantomRttiManager::Shutdown: Removal of types failed!");
}
