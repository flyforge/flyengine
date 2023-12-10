#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/ThreadUtils.h>

PLASMA_ENUMERABLE_CLASS_IMPLEMENTATION(plSubSystem);

bool plStartup::s_bPrintAllSubSystems = true;
plStartupStage::Enum plStartup::s_CurrentState = plStartupStage::None;
plDynamicArray<const char*> plStartup::s_ApplicationTags;


void plStartup::AddApplicationTag(const char* szTag)
{
  s_ApplicationTags.PushBack(szTag);
}

bool plStartup::HasApplicationTag(const char* szTag)
{
  for (plUInt32 i = 0; i < s_ApplicationTags.GetCount(); ++i)
  {
    if (plStringUtils::IsEqual_NoCase(s_ApplicationTags[i], szTag))
      return true;
  }

  return false;
}

void plStartup::PrintAllSubsystems()
{
  PLASMA_LOG_BLOCK("Available Subsystems");

  plSubSystem* pSub = plSubSystem::GetFirstInstance();

  while (pSub)
  {
    plLog::Debug("Subsystem: '{0}::{1}'", pSub->GetGroupName(), pSub->GetSubSystemName());

    if (pSub->GetDependency(0) == nullptr)
      plLog::Debug("  <no dependencies>");
    else
    {
      for (plInt32 i = 0; pSub->GetDependency(i) != nullptr; ++i)
        plLog::Debug("  depends on '{0}'", pSub->GetDependency(i));
    }

    plLog::Debug("");

    pSub = pSub->GetNextInstance();
  }
}

void plStartup::AssignSubSystemPlugin(plStringView sPluginName)
{
  // iterates over all existing subsystems and finds those that have no plugin name yet
  // assigns the given name to them

  plSubSystem* pSub = plSubSystem::GetFirstInstance();

  while (pSub)
  {
    if (pSub->m_sPluginName.IsEmpty())
    {
      pSub->m_sPluginName = sPluginName;
    }

    pSub = pSub->GetNextInstance();
  }
}

void plStartup::PluginEventHandler(const plPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case plPluginEvent::BeforeLoading:
    {
      AssignSubSystemPlugin("Static");
    }
    break;

    case plPluginEvent::AfterLoadingBeforeInit:
    {
      AssignSubSystemPlugin(EventData.m_sPluginBinary);
    }
    break;

    case plPluginEvent::StartupShutdown:
    {
      plStartup::UnloadPluginSubSystems(EventData.m_sPluginBinary);
    }
    break;

    case plPluginEvent::AfterPluginChanges:
    {
      plStartup::ReinitToCurrentState();
    }
    break;

    default:
      break;
  }
}

static bool IsGroupName(plStringView sName)
{
  plSubSystem* pSub = plSubSystem::GetFirstInstance();

  bool bGroup = false;
  bool bSubSystem = false;

  while (pSub)
  {
    if (pSub->GetGroupName() == sName)
      bGroup = true;

    if (pSub->GetSubSystemName() == sName)
      bSubSystem = true;

    pSub = pSub->GetNextInstance();
  }

  PLASMA_ASSERT_ALWAYS(!bGroup || !bSubSystem, "There cannot be a SubSystem AND a Group called '{0}'.", sName);

  return bGroup;
}

static plStringView GetGroupSubSystems(plStringView sGroup, plInt32 iSubSystem)
{
  plSubSystem* pSub = plSubSystem::GetFirstInstance();

  while (pSub)
  {
    if (pSub->GetGroupName() == sGroup)
    {
      if (iSubSystem == 0)
        return pSub->GetSubSystemName();

      --iSubSystem;
    }

    pSub = pSub->GetNextInstance();
  }

  return nullptr;
}

void plStartup::ComputeOrder(plDeque<plSubSystem*>& Order)
{
  Order.Clear();
  plSet<plString> sSystemsInited;

  bool bCouldInitAny = true;

  while (bCouldInitAny)
  {
    bCouldInitAny = false;

    plSubSystem* pSub = plSubSystem::GetFirstInstance();

    while (pSub)
    {
      if (!sSystemsInited.Find(pSub->GetSubSystemName()).IsValid())
      {
        bool bAllDependsFulfilled = true;
        plInt32 iDep = 0;

        while (pSub->GetDependency(iDep) != nullptr)
        {
          if (IsGroupName(pSub->GetDependency(iDep)))
          {
            plInt32 iSubSystemIndex = 0;
            plStringView sNextSubSystem = GetGroupSubSystems(pSub->GetDependency(iDep), iSubSystemIndex);
            while (sNextSubSystem.IsValid())
            {
              if (!sSystemsInited.Find(sNextSubSystem).IsValid())
              {
                bAllDependsFulfilled = false;
                break;
              }

              ++iSubSystemIndex;
              sNextSubSystem = GetGroupSubSystems(pSub->GetDependency(iDep), iSubSystemIndex);
            }
          }
          else
          {
            if (!sSystemsInited.Find(pSub->GetDependency(iDep)).IsValid())
            {
              bAllDependsFulfilled = false;
              break;
            }
          }

          ++iDep;
        }

        if (bAllDependsFulfilled)
        {
          bCouldInitAny = true;
          Order.PushBack(pSub);
          sSystemsInited.Insert(pSub->GetSubSystemName());
        }
      }

      pSub = pSub->GetNextInstance();
    }
  }
}

void plStartup::Startup(plStartupStage::Enum stage)
{
  if (stage == plStartupStage::BaseSystems)
  {
    plFoundation::Initialize();
  }

  const char* szStartup[] = {"Startup Base", "Startup Core", "Startup Engine"};

  if (stage == plStartupStage::CoreSystems)
  {
    Startup(plStartupStage::BaseSystems);

    plGlobalEvent::Broadcast(PLASMA_GLOBALEVENT_STARTUP_CORESYSTEMS_BEGIN);

    if (s_bPrintAllSubSystems)
    {
      s_bPrintAllSubSystems = false;
      PrintAllSubsystems();
    }
  }

  if (stage == plStartupStage::HighLevelSystems)
  {
    Startup(plStartupStage::CoreSystems);

    plGlobalEvent::Broadcast(PLASMA_GLOBALEVENT_STARTUP_HIGHLEVELSYSTEMS_BEGIN);
  }

  PLASMA_LOG_BLOCK(szStartup[stage]);

  plDeque<plSubSystem*> Order;
  ComputeOrder(Order);

  for (plUInt32 i = 0; i < Order.GetCount(); ++i)
  {
    if (!Order[i]->m_bStartupDone[stage])
    {
      Order[i]->m_bStartupDone[stage] = true;

      switch (stage)
      {
        case plStartupStage::BaseSystems:
          plLog::Debug("Executing 'Base' startup for sub-system '{1}::{0}'", Order[i]->GetSubSystemName(), Order[i]->GetGroupName());
          Order[i]->OnBaseSystemsStartup();
          break;
        case plStartupStage::CoreSystems:
          plLog::Debug("Executing 'Core' startup for sub-system '{1}::{0}'", Order[i]->GetSubSystemName(), Order[i]->GetGroupName());
          Order[i]->OnCoreSystemsStartup();
          break;
        case plStartupStage::HighLevelSystems:
          plLog::Debug("Executing 'Engine' startup for sub-system '{1}::{0}'", Order[i]->GetSubSystemName(), Order[i]->GetGroupName());
          Order[i]->OnHighLevelSystemsStartup();
          break;

        default:
          break;
      }
    }
  }

  // now everything should be started
  {
    PLASMA_LOG_BLOCK("Failed SubSystems");

    plSet<plString> sSystemsFound;

    plSubSystem* pSub = plSubSystem::GetFirstInstance();

    while (pSub)
    {
      sSystemsFound.Insert(pSub->GetSubSystemName());
      pSub = pSub->GetNextInstance();
    }

    pSub = plSubSystem::GetFirstInstance();

    while (pSub)
    {
      if (!pSub->m_bStartupDone[stage])
      {
        plInt32 iDep = 0;

        while (pSub->GetDependency(iDep) != nullptr)
        {
          if (!sSystemsFound.Find(pSub->GetDependency(iDep)).IsValid())
          {
            plLog::Error("SubSystem '{0}::{1}' could not be started because dependency '{2}' is unknown.", pSub->GetGroupName(),
              pSub->GetSubSystemName(), pSub->GetDependency(iDep));
          }
          else
          {
            plLog::Error("SubSystem '{0}::{1}' could not be started because dependency '{2}' has not been initialized.", pSub->GetGroupName(),
              pSub->GetSubSystemName(), pSub->GetDependency(iDep));
          }

          ++iDep;
        }
      }

      pSub = pSub->GetNextInstance();
    }
  }

  switch (stage)
  {
    case plStartupStage::BaseSystems:
      break;
    case plStartupStage::CoreSystems:
      plGlobalEvent::Broadcast(PLASMA_GLOBALEVENT_STARTUP_CORESYSTEMS_END);
      break;
    case plStartupStage::HighLevelSystems:
      plGlobalEvent::Broadcast(PLASMA_GLOBALEVENT_STARTUP_HIGHLEVELSYSTEMS_END);
      break;

    default:
      break;
  }

  if (s_CurrentState == plStartupStage::None)
  {
    plPlugin::Events().AddEventHandler(PluginEventHandler);
  }

  s_CurrentState = stage;
}

void plStartup::Shutdown(plStartupStage::Enum stage)
{
  // without that we cannot function, so make sure it is up and running
  plFoundation::Initialize();

  {
    const char* szStartup[] = {"Shutdown Base", "Shutdown Core", "Shutdown Engine"};

    if (stage == plStartupStage::BaseSystems)
    {
      Shutdown(plStartupStage::CoreSystems);
    }

    if (stage == plStartupStage::CoreSystems)
    {
      Shutdown(plStartupStage::HighLevelSystems);
      s_bPrintAllSubSystems = true;

      plGlobalEvent::Broadcast(PLASMA_GLOBALEVENT_SHUTDOWN_CORESYSTEMS_BEGIN);
    }

    if (stage == plStartupStage::HighLevelSystems)
    {
      plGlobalEvent::Broadcast(PLASMA_GLOBALEVENT_SHUTDOWN_HIGHLEVELSYSTEMS_BEGIN);
    }

    PLASMA_LOG_BLOCK(szStartup[stage]);

    plDeque<plSubSystem*> Order;
    ComputeOrder(Order);

    for (plInt32 i = (plInt32)Order.GetCount() - 1; i >= 0; --i)
    {
      if (Order[i]->m_bStartupDone[stage])
      {
        switch (stage)
        {
          case plStartupStage::CoreSystems:
            plLog::Debug("Executing 'Core' shutdown of sub-system '{0}::{1}'", Order[i]->GetGroupName(), Order[i]->GetSubSystemName());
            Order[i]->OnCoreSystemsShutdown();
            break;

          case plStartupStage::HighLevelSystems:
            plLog::Debug("Executing 'Engine' shutdown of sub-system '{0}::{1}'", Order[i]->GetGroupName(), Order[i]->GetSubSystemName());
            Order[i]->OnHighLevelSystemsShutdown();
            break;

          default:
            break;
        }

        Order[i]->m_bStartupDone[stage] = false;
      }
    }
  }

  switch (stage)
  {
    case plStartupStage::CoreSystems:
      plGlobalEvent::Broadcast(PLASMA_GLOBALEVENT_SHUTDOWN_CORESYSTEMS_END);
      break;

    case plStartupStage::HighLevelSystems:
      plGlobalEvent::Broadcast(PLASMA_GLOBALEVENT_SHUTDOWN_HIGHLEVELSYSTEMS_END);
      break;

    default:
      break;
  }

  if (s_CurrentState != plStartupStage::None)
  {
    s_CurrentState = (plStartupStage::Enum)(((plInt32)stage) - 1);

    if (s_CurrentState == plStartupStage::None)
    {
      plPlugin::Events().RemoveEventHandler(PluginEventHandler);
    }
  }
}

bool plStartup::HasDependencyOnPlugin(plSubSystem* pSubSystem, plStringView sModule)
{
  if (pSubSystem->m_sPluginName == sModule)
    return true;

  for (plUInt32 i = 0; pSubSystem->GetDependency(i) != nullptr; ++i)
  {
    plSubSystem* pSub = plSubSystem::GetFirstInstance();
    while (pSub)
    {
      if (pSub->GetSubSystemName() == pSubSystem->GetDependency(i))
      {
        if (HasDependencyOnPlugin(pSub, sModule))
          return true;

        break;
      }

      pSub = pSub->GetNextInstance();
    }
  }

  return false;
}

void plStartup::UnloadPluginSubSystems(plStringView sPluginName)
{
  PLASMA_LOG_BLOCK("Unloading Plugin SubSystems", sPluginName);
  plLog::Dev("Plugin to unload: '{0}'", sPluginName);

  plGlobalEvent::Broadcast(PLASMA_GLOBALEVENT_UNLOAD_PLUGIN_BEGIN, plVariant(sPluginName));

  plDeque<plSubSystem*> Order;
  ComputeOrder(Order);

  for (plInt32 i = (plInt32)Order.GetCount() - 1; i >= 0; --i)
  {
    if (Order[i]->m_bStartupDone[plStartupStage::HighLevelSystems] && HasDependencyOnPlugin(Order[i], sPluginName))
    {
      plLog::Info("Engine shutdown of SubSystem '{0}::{1}', because it depends on Plugin '{2}'.", Order[i]->GetGroupName(), Order[i]->GetSubSystemName(), sPluginName);
      Order[i]->OnHighLevelSystemsShutdown();
      Order[i]->m_bStartupDone[plStartupStage::HighLevelSystems] = false;
    }
  }

  for (plInt32 i = (plInt32)Order.GetCount() - 1; i >= 0; --i)
  {
    if (Order[i]->m_bStartupDone[plStartupStage::CoreSystems] && HasDependencyOnPlugin(Order[i], sPluginName))
    {
      plLog::Info("Core shutdown of SubSystem '{0}::{1}', because it depends on Plugin '{2}'.", Order[i]->GetGroupName(), Order[i]->GetSubSystemName(), sPluginName);
      Order[i]->OnCoreSystemsShutdown();
      Order[i]->m_bStartupDone[plStartupStage::CoreSystems] = false;
    }
  }


  plGlobalEvent::Broadcast(PLASMA_GLOBALEVENT_UNLOAD_PLUGIN_END, plVariant(sPluginName));
}

void plStartup::ReinitToCurrentState()
{
  if (s_CurrentState != plStartupStage::None)
    Startup(s_CurrentState);
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_Startup);
