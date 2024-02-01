#include <Core/CorePCH.h>

#include <Core/World/World.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plWorldModule, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plWorldModule::plWorldModule(plWorld* pWorld)
  : m_pWorld(pWorld)
{
}

plWorldModule::~plWorldModule() = default;

plUInt32 plWorldModule::GetWorldIndex() const
{
  return GetWorld()->GetIndex();
}

// protected methods

void plWorldModule::RegisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->RegisterUpdateFunction(desc);
}

void plWorldModule::DeregisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->DeregisterUpdateFunction(desc);
}

plAllocator* plWorldModule::GetAllocator()
{
  return m_pWorld->GetAllocator();
}

plInternal::WorldLargeBlockAllocator* plWorldModule::GetBlockAllocator()
{
  return m_pWorld->GetBlockAllocator();
}

bool plWorldModule::GetWorldSimulationEnabled() const
{
  return m_pWorld->GetWorldSimulationEnabled();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(Core, WorldModuleFactory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Reflection"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plPlugin::Events().AddEventHandler(plWorldModuleFactory::PluginEventHandler);
    plWorldModuleFactory::GetInstance()->FillBaseTypeIds();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plPlugin::Events().RemoveEventHandler(plWorldModuleFactory::PluginEventHandler);
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

static plWorldModuleTypeId s_uiNextTypeId = 0;
static plDynamicArray<plWorldModuleTypeId> s_freeTypeIds;
static constexpr plWorldModuleTypeId s_InvalidWorldModuleTypeId = plWorldModuleTypeId(-1);

plWorldModuleFactory::plWorldModuleFactory() = default;

// static
plWorldModuleFactory* plWorldModuleFactory::GetInstance()
{
  static plWorldModuleFactory* pInstance = new plWorldModuleFactory();
  return pInstance;
}

plWorldModuleTypeId plWorldModuleFactory::GetTypeId(const plRTTI* pRtti)
{
  plWorldModuleTypeId uiTypeId = s_InvalidWorldModuleTypeId;
  m_TypeToId.TryGetValue(pRtti, uiTypeId);
  return uiTypeId;
}

plWorldModule* plWorldModuleFactory::CreateWorldModule(plWorldModuleTypeId typeId, plWorld* pWorld)
{
  if (typeId < m_CreatorFuncs.GetCount())
  {
    CreatorFunc func = m_CreatorFuncs[typeId].m_Func;
    return (*func)(pWorld->GetAllocator(), pWorld);
  }

  return nullptr;
}

void plWorldModuleFactory::RegisterInterfaceImplementation(plStringView sInterfaceName, plStringView sImplementationName)
{
  m_InterfaceImplementations.Insert(sInterfaceName, sImplementationName);

  plStringBuilder sTemp = sInterfaceName;
  const plRTTI* pInterfaceRtti = plRTTI::FindTypeByName(sTemp);

  sTemp = sImplementationName;
  const plRTTI* pImplementationRtti = plRTTI::FindTypeByName(sTemp);

  if (pInterfaceRtti != nullptr && pImplementationRtti != nullptr)
  {
    m_TypeToId[pInterfaceRtti] = m_TypeToId[pImplementationRtti];
    return;
  }

  // Clear existing mapping if it maps to the wrong type
  plUInt16 uiTypeId;
  if (pInterfaceRtti != nullptr && m_TypeToId.TryGetValue(pInterfaceRtti, uiTypeId))
  {
    if (m_CreatorFuncs[uiTypeId].m_pRtti->GetTypeName() != sImplementationName)
    {
      PL_ASSERT_DEV(pImplementationRtti == nullptr, "Implementation error");
      m_TypeToId.Remove(pInterfaceRtti);
    }
  }
}
plWorldModuleTypeId plWorldModuleFactory::RegisterWorldModule(const plRTTI* pRtti, CreatorFunc creatorFunc)
{
  PL_ASSERT_DEV(pRtti != plGetStaticRTTI<plWorldModule>(), "Trying to register a world module that is not reflected!");
  PL_ASSERT_DEV(
    m_TypeToId.GetCount() < plWorld::GetMaxNumWorldModules(), "Max number of world modules reached: {}", plWorld::GetMaxNumWorldModules());

  plWorldModuleTypeId uiTypeId = s_InvalidWorldModuleTypeId;
  if (m_TypeToId.TryGetValue(pRtti, uiTypeId))
  {
    return uiTypeId;
  }

  if (s_freeTypeIds.IsEmpty())
  {
    PL_ASSERT_DEV(s_uiNextTypeId < PL_MAX_WORLD_MODULE_TYPES - 1, "World module id overflow!");

  uiTypeId = s_uiNextTypeId++;
  }
  else
  {
    uiTypeId = s_freeTypeIds.PeekBack();
    s_freeTypeIds.PopBack();
  }

  m_TypeToId.Insert(pRtti, uiTypeId);

  m_CreatorFuncs.EnsureCount(uiTypeId + 1);

  auto& creatorFuncContext = m_CreatorFuncs[uiTypeId];
  creatorFuncContext.m_Func = creatorFunc;
  creatorFuncContext.m_pRtti = pRtti;

  return uiTypeId;
}

// static
void plWorldModuleFactory::PluginEventHandler(const plPluginEvent& EventData)
{
  if (EventData.m_EventType == plPluginEvent::AfterLoadingBeforeInit)
  {
    plWorldModuleFactory::GetInstance()->FillBaseTypeIds();
  }

  if (EventData.m_EventType == plPluginEvent::AfterUnloading)
  {
    plWorldModuleFactory::GetInstance()->ClearUnloadedTypeToIDs();
  }
}

namespace
{
  struct NewEntry
  {
    PL_DECLARE_POD_TYPE();

    const plRTTI* m_pRtti;
    plWorldModuleTypeId m_uiTypeId;
  };
} // namespace

void plWorldModuleFactory::AdjustBaseTypeId(const plRTTI* pParentRtti, const plRTTI* pRtti, plUInt16 uiParentTypeId)
{
  plDynamicArray<plPlugin::PluginInfo> infos;
  plPlugin::GetAllPluginInfos(infos);

  auto HasManualDependency = [&](plStringView sPluginName) -> bool {
    for (const auto& p : infos)
    {
      if (p.m_sName == sPluginName)
      {
        return !p.m_LoadFlags.IsSet(plPluginLoadFlags::CustomDependency);
      }
    }

    return false;
  };

  plStringView szPlugin1 = m_CreatorFuncs[uiParentTypeId].m_pRtti->GetPluginName();
  plStringView szPlugin2 = pRtti->GetPluginName();

  const bool bPrio1 = HasManualDependency(szPlugin1);
  const bool bPrio2 = HasManualDependency(szPlugin2);

  if (bPrio1 && !bPrio2)
  {
    // keep the previous one
    return;
  }

  if (!bPrio1 && bPrio2)
  {
    // take the new one
    m_TypeToId[pParentRtti] = m_TypeToId[pRtti];
    return;
  }

  plLog::Error("Interface '{}' is already implemented by '{}'. Specify which implementation should be used via RegisterInterfaceImplementation() or WorldModules.ddl config file.", pParentRtti->GetTypeName(), m_CreatorFuncs[uiParentTypeId].m_pRtti->GetTypeName());
}

void plWorldModuleFactory::FillBaseTypeIds()
{
  // m_TypeToId contains RTTI types for plWorldModules and plComponents
  // m_TypeToId[plComponent] maps to TypeID for its respective plComponentManager
  // m_TypeToId[plWorldModule] maps to TypeID for itself OR in case of an interface to the derived type that implements the interface
  // after types are registered we only have a mapping for m_TypeToId[plWorldModule(impl)] and now we want to add
  // the mapping for m_TypeToId[plWorldModule(interface)], such that querying the TypeID for the interface works as well
  // and yields the implementation

  plHybridArray<NewEntry, 64, plStaticsAllocatorWrapper> newEntries;
  const plRTTI* pModuleRtti = plGetStaticRTTI<plWorldModule>(); // base type where we want to stop iterating upwards

  // explicit mappings
  for (auto it = m_InterfaceImplementations.GetIterator(); it.IsValid(); ++it)
  {
    const plRTTI* pInterfaceRtti = plRTTI::FindTypeByName(it.Key());
    const plRTTI* pImplementationRtti = plRTTI::FindTypeByName(it.Value());

    if (pInterfaceRtti != nullptr && pImplementationRtti != nullptr)
    {
      m_TypeToId[pInterfaceRtti] = m_TypeToId[pImplementationRtti];
    }
  }

  // automatic mappings
  for (auto it = m_TypeToId.GetIterator(); it.IsValid(); ++it)
  {
    const plRTTI* pRtti = it.Key();

    // ignore components, we only want to fill out mappings for the base types of world modules
    if (!pRtti->IsDerivedFrom<plWorldModule>())
      continue;

    const plWorldModuleTypeId uiTypeId = it.Value();

    for (const plRTTI* pParentRtti = pRtti->GetParentType(); pParentRtti != pModuleRtti; pParentRtti = pParentRtti->GetParentType())
    {
      // we are only interested in parent types that are pure interfaces
      if (!pParentRtti->GetTypeFlags().IsSet(plTypeFlags::Abstract))
        continue;

      // skip if we have an explicit mapping for this interface, they are already handled above
      if (m_InterfaceImplementations.GetValue(pParentRtti->GetTypeName()) != nullptr)
        continue;


      if (plUInt16* pParentTypeId = m_TypeToId.GetValue(pParentRtti))
      {
        if (*pParentTypeId != uiTypeId)
        {
          AdjustBaseTypeId(pParentRtti, pRtti, *pParentTypeId);
        }
      }
      else
      {
        auto& newEntry = newEntries.ExpandAndGetRef();
        newEntry.m_pRtti = pParentRtti;
        newEntry.m_uiTypeId = uiTypeId;
      }
    }
  }

  // delayed insertion to not interfere with the iteration above
  for (auto& newEntry : newEntries)
  {
    m_TypeToId.Insert(newEntry.m_pRtti, newEntry.m_uiTypeId);
  }
}

void plWorldModuleFactory::ClearUnloadedTypeToIDs()
{
  plSet<const plRTTI*> allRttis;
  plRTTI::ForEachType([&](const plRTTI* pRtti) { allRttis.Insert(pRtti); });

  plSet<plWorldModuleTypeId> mappedIdsToRemove;

  for (auto it = m_TypeToId.GetIterator(); it.IsValid();)
  {
    const plRTTI* pRtti = it.Key();
    const plWorldModuleTypeId uiTypeId = it.Value();

    if (!allRttis.Contains(pRtti))
    {
      // type got removed, clear it from the map
      it = m_TypeToId.Remove(it);

      // and record that all other types that map to the same typeId also must be removed
      mappedIdsToRemove.Insert(uiTypeId);
    }
    else
    {
      ++it;
    }
  }

  // now remove all mappings that map to an invalid typeId
  // this can be more than one, since we can map multiple (interface) types to the same implementation
  for (auto it = m_TypeToId.GetIterator(); it.IsValid();)
  {
    const plWorldModuleTypeId uiTypeId = it.Value();

    if (mappedIdsToRemove.Contains(uiTypeId))
    {
      it = m_TypeToId.Remove(it);
    }
    else
    {
      ++it;
    }
  }

  // Finally, adding all invalid typeIds to the free list for reusing later
  for (plWorldModuleTypeId removedId : mappedIdsToRemove)
  {
    s_freeTypeIds.PushBack(removedId);
  }
}

PL_STATICLINK_FILE(Core, Core_World_Implementation_WorldModule);
