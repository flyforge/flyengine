#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetProcessorMessages.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

PLASMA_IMPLEMENT_SINGLETON(plAssetProcessor);

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetProcessor)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "AssetCurator"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    PLASMA_DEFAULT_NEW(plAssetProcessor);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plAssetProcessor* pDummy = plAssetProcessor::GetSingleton();
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

////////////////////////////////////////////////////////////////////////
// plCuratorLog
////////////////////////////////////////////////////////////////////////

void plAssetProcessorLog::HandleLogMessage(const plLoggingEventData& le)
{
  const plLogMsgType::Enum ThisType = le.m_EventType;
  m_LoggingEvent.Broadcast(le);
}

void plAssetProcessorLog::AddLogWriter(plLoggingEvent::Handler handler)
{
  m_LoggingEvent.AddEventHandler(handler);
}

void plAssetProcessorLog::RemoveLogWriter(plLoggingEvent::Handler handler)
{
  m_LoggingEvent.RemoveEventHandler(handler);
}


////////////////////////////////////////////////////////////////////////
// plAssetProcessor
////////////////////////////////////////////////////////////////////////

plAssetProcessor::plAssetProcessor()
  : m_SingletonRegistrar(this)
{
}


plAssetProcessor::~plAssetProcessor()
{
  if (m_Thread)
  {
    m_Thread->Join();
    m_Thread.Clear();
  }
  PLASMA_ASSERT_DEV(m_ProcessTaskState == ProcessTaskState::Stopped, "Call StopProcessTask first before destroying the plAssetProcessor.");
}

void plAssetProcessor::StartProcessTask()
{
  PLASMA_LOCK(m_ProcessorMutex);
  if (m_ProcessTaskState != ProcessTaskState::Stopped)
  {
    return;
  }

  // Join old thread.
  if (m_Thread)
  {
    m_Thread->Join();
    m_Thread.Clear();
  }

  m_ProcessTaskState = ProcessTaskState::Running;

  const plUInt32 uiWorkerCount = plTaskSystem::GetWorkerThreadCount(plWorkerThreadType::LongTasks);
  m_ProcessRunning.SetCount(uiWorkerCount, false);
  m_ProcessTasks.SetCount(uiWorkerCount);

  for (plUInt32 idx = 0; idx < uiWorkerCount; ++idx)
  {
    m_ProcessTasks[idx].m_uiProcessorID = idx;
  }

  m_Thread = PLASMA_DEFAULT_NEW(plProcessThread);
  m_Thread->Start();

  {
    plAssetProcessorEvent e;
    e.m_Type = plAssetProcessorEvent::Type::ProcessTaskStateChanged;
    m_Events.Broadcast(e);
  }
}

void plAssetProcessor::StopProcessTask(bool bForce)
{
  {
    PLASMA_LOCK(m_ProcessorMutex);
    switch (m_ProcessTaskState)
    {
      case ProcessTaskState::Running:
      {
        m_ProcessTaskState = ProcessTaskState::Stopping;
        {
          plAssetProcessorEvent e;
          e.m_Type = plAssetProcessorEvent::Type::ProcessTaskStateChanged;
          m_Events.Broadcast(e);
        }
      }
      break;
      case ProcessTaskState::Stopping:
        if (!bForce)
          return;
        break;
      default:
      case ProcessTaskState::Stopped:
        return;
    }
  }

  if (bForce)
  {
    m_bForceStop = true;
    m_Thread->Join();
    m_Thread.Clear();
    PLASMA_ASSERT_DEV(m_ProcessTaskState == ProcessTaskState::Stopped, "Process task shoul have set the state to stopped.");
  }
}

void plAssetProcessor::AddLogWriter(plLoggingEvent::Handler handler)
{
  m_CuratorLog.AddLogWriter(handler);
}

void plAssetProcessor::RemoveLogWriter(plLoggingEvent::Handler handler)
{
  m_CuratorLog.RemoveLogWriter(handler);
}

void plAssetProcessor::Run()
{
  while (m_ProcessTaskState == ProcessTaskState::Running)
  {
    for (plUInt32 i = 0; i < m_ProcessTasks.GetCount(); i++)
    {
      if (m_ProcessRunning[i])
      {
        m_ProcessRunning[i] = !m_ProcessTasks[i].FinishExecute();
      }
      else
      {
        m_ProcessRunning[i] = m_ProcessTasks[i].BeginExecute();
      }
    }
    plThreadUtils::Sleep(plTime::Milliseconds(100));
  }

  while (true)
  {
    bool bAnyRunning = false;

    for (plUInt32 i = 0; i < m_ProcessTasks.GetCount(); i++)
    {
      if (m_ProcessRunning[i])
      {
        if (m_bForceStop)
          m_ProcessTasks[i].ShutdownProcess();

        m_ProcessRunning[i] = !m_ProcessTasks[i].FinishExecute();
        bAnyRunning |= m_ProcessRunning[i];
      }
    }

    if (bAnyRunning)
      plThreadUtils::Sleep(plTime::Milliseconds(100));
    else
      break;
  }

  PLASMA_LOCK(m_ProcessorMutex);
  m_ProcessRunning.Clear();
  m_ProcessTasks.Clear();
  m_ProcessTaskState = ProcessTaskState::Stopped;
  m_bForceStop = false;
  {
    plAssetProcessorEvent e;
    e.m_Type = plAssetProcessorEvent::Type::ProcessTaskStateChanged;
    m_Events.Broadcast(e);
  }
}


////////////////////////////////////////////////////////////////////////
// plProcessTask
////////////////////////////////////////////////////////////////////////

plProcessTask::plProcessTask()
  : m_bProcessShouldBeRunning(false)
  , m_bProcessCrashed(false)
  , m_bWaiting(false)
  , m_Status(PLASMA_SUCCESS)
{
  m_pIPC = PLASMA_DEFAULT_NEW(PlasmaEditorProcessCommunicationChannel);
  m_pIPC->m_Events.AddEventHandler(plMakeDelegate(&plProcessTask::EventHandlerIPC, this));
}

plProcessTask::~plProcessTask()
{
  ShutdownProcess();
  m_pIPC->m_Events.RemoveEventHandler(plMakeDelegate(&plProcessTask::EventHandlerIPC, this));
  PLASMA_DEFAULT_DELETE(m_pIPC);
}


void plProcessTask::StartProcess()
{
  const plRTTI* pFirstAllowedMessageType = nullptr;
  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed = false;

  plStringBuilder tmp;

  QStringList args;
  args << "-appname";
  args << plApplication::GetApplicationInstance()->GetApplicationName().GetData();
  args << "-appid";
  args << QString::number(m_uiProcessorID);
  args << "-project";
  args << plToolsProject::GetSingleton()->GetProjectFile().GetData();
  args << "-renderer";
  args << plGameApplication::GetActiveRenderer().GetData(tmp);

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  const char* EditorProcessorExecutable = "EditorProcessor.exe";
#else
  const char* EditorProcessorExecutable = "EditorProcessor";
#endif

  if (m_pIPC->StartClientProcess(EditorProcessorExecutable, args, false, pFirstAllowedMessageType).Failed())
  {
    m_bProcessCrashed = true;
  }
}

void plProcessTask::ShutdownProcess()
{
  if (!m_bProcessShouldBeRunning)
    return;

  m_bProcessShouldBeRunning = false;
  m_pIPC->CloseConnection();
}

void plProcessTask::EventHandlerIPC(const plProcessCommunicationChannel::Event& e)
{
  if (const plProcessAssetResponseMsg* pMsg = plDynamicCast<const plProcessAssetResponseMsg*>(e.m_pMessage))
  {
    m_Status = pMsg->m_Status;
    m_bWaiting = false;
    m_LogEntries.Swap(pMsg->m_LogEntries);
  }
}

bool plProcessTask::GetNextAssetToProcess(plAssetInfo* pInfo, plUuid& out_guid, plStringBuilder& out_sAbsPath, plStringBuilder& out_sRelPath)
{
  bool bComplete = true;

  const plDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (plDocumentManager::FindDocumentTypeFromPath(pInfo->m_sAbsolutePath, false, pTypeDesc).Succeeded())
  {
    auto flags = static_cast<const plAssetDocumentTypeDescriptor*>(pTypeDesc)->m_AssetDocumentFlags;

    if (flags.IsAnySet(plAssetDocumentFlags::OnlyTransformManually | plAssetDocumentFlags::DisableTransform))
      return false;
  }

  auto TestFunc = [this, &bComplete](const plSet<plString>& Files) -> plAssetInfo* {
    for (const auto& sFile : Files)
    {
      if (plAssetInfo* pFileInfo = plAssetCurator::GetSingleton()->GetAssetInfo(sFile))
      {
        switch (pFileInfo->m_TransformState)
        {
          case plAssetInfo::TransformState::Unknown:
          case plAssetInfo::TransformState::TransformError:
          case plAssetInfo::TransformState::MissingDependency:
          case plAssetInfo::TransformState::MissingReference:
          {
            bComplete = false;
            continue;
          }
          case plAssetInfo::TransformState::NeedsTransform:
          case plAssetInfo::TransformState::NeedsThumbnail:
          {
            bComplete = false;
            return pFileInfo;
          }
          case plAssetInfo::TransformState::UpToDate:
            continue;

          case plAssetInfo::TransformState::NeedsImport:
            // the main processor has to do this itself
            continue;

            PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
    }
    return nullptr;
  };

  if (plAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_AssetTransformDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_sAbsPath, out_sRelPath);
  }

  if (plAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_RuntimeDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_sAbsPath, out_sRelPath);
  }

  if (bComplete && !plAssetCurator::GetSingleton()->m_Updating.Contains(pInfo->m_Info->m_DocumentID) &&
      !plAssetCurator::GetSingleton()->m_TransformStateStale.Contains(pInfo->m_Info->m_DocumentID))
  {
    plAssetCurator::GetSingleton()->m_Updating.Insert(pInfo->m_Info->m_DocumentID);
    out_guid = pInfo->m_Info->m_DocumentID;
    out_sAbsPath = pInfo->m_sAbsolutePath;
    out_sRelPath = pInfo->m_sDataDirParentRelativePath;
    return true;
  }

  return false;
}

bool plProcessTask::GetNextAssetToProcess(plUuid& out_guid, plStringBuilder& out_sAbsPath, plStringBuilder& out_sRelPath)
{
  PLASMA_LOCK(plAssetCurator::GetSingleton()->m_CuratorMutex);

  for (auto it = plAssetCurator::GetSingleton()->m_TransformState[plAssetInfo::TransformState::NeedsTransform].GetIterator(); it.IsValid(); ++it)
  {
    plAssetInfo* pInfo = plAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_sAbsPath, out_sRelPath);
      if (bRes)
        return true;
    }
  }

  for (auto it = plAssetCurator::GetSingleton()->m_TransformState[plAssetInfo::TransformState::NeedsThumbnail].GetIterator(); it.IsValid(); ++it)
  {
    plAssetInfo* pInfo = plAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_sAbsPath, out_sRelPath);
      if (bRes)
        return true;
    }
  }

  return false;
}


void plProcessTask::OnProcessCrashed()
{
  m_Status = plStatus("Asset processor crashed");
  plLogEntryDelegate logger([this](plLogEntry& entry) { m_LogEntries.PushBack(std::move(entry)); });
  plLog::Error(&logger, "AssetProcessor crashed!");
  plLog::Error(&plAssetProcessor::GetSingleton()->m_CuratorLog, "AssetProcessor crashed!");
}

bool plProcessTask::BeginExecute()
{
  plStringBuilder sAssetRelPath;

  m_LogEntries.Clear();
  m_TransitiveHull.Clear();
  m_Status = plStatus(PLASMA_SUCCESS);
  {
    PLASMA_LOCK(plAssetCurator::GetSingleton()->m_CuratorMutex);

    if (!GetNextAssetToProcess(m_AssetGuid, m_sAssetPath, sAssetRelPath))
    {
      m_AssetGuid = plUuid();
      m_sAssetPath.Clear();
      m_bDidWork = false;
      return false;
    }

    m_bDidWork = true;
    plAssetInfo::TransformState state = plAssetCurator::GetSingleton()->IsAssetUpToDate(m_AssetGuid, nullptr, nullptr, m_uiAssetHash, m_uiThumbHash);
    PLASMA_ASSERT_DEV(state == plAssetInfo::TransformState::NeedsTransform || state == plAssetInfo::TransformState::NeedsThumbnail, "An asset was selected that is already up to date.");

    plSet<plString> dependencies;

    plStringBuilder sTemp;
    plAssetCurator::GetSingleton()->GenerateTransitiveHull(plConversionUtils::ToString(m_AssetGuid, sTemp), &dependencies, &dependencies);

    m_TransitiveHull.Reserve(dependencies.GetCount());
    for (const plString& str : dependencies)
    {
      m_TransitiveHull.PushBack(str);
    }
  }

  if (!m_bProcessShouldBeRunning)
  {
    StartProcess();
  }

  if (m_bProcessCrashed)
  {
    OnProcessCrashed();
    return false;
  }
  else
  {
    plLog::Info(&plAssetProcessor::GetSingleton()->m_CuratorLog, "Processing '{0}'", sAssetRelPath);
    // Send and wait
    plProcessAssetMsg msg;
    msg.m_AssetGuid = m_AssetGuid;
    msg.m_AssetHash = m_uiAssetHash;
    msg.m_ThumbHash = m_uiThumbHash;
    msg.m_sAssetPath = m_sAssetPath;
    msg.m_DepRefHull.Swap(m_TransitiveHull);
    msg.m_sPlatform = plAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();

    m_pIPC->SendMessage(&msg);
    m_bWaiting = true;
    return true;
  }
}

bool plProcessTask::FinishExecute()
{
  if (m_bWaiting)
  {
    m_pIPC->ProcessMessages();
    if (!m_pIPC->IsClientAlive())
    {
      m_bProcessCrashed = true;
    }

    if (m_bProcessCrashed)
    {
      m_bWaiting = false;
      OnProcessCrashed();
    }
    if (m_bWaiting)
      return false;
  }

  if (m_Status.Succeeded())
  {
    plAssetCurator::GetSingleton()->NotifyOfAssetChange(m_AssetGuid);
    plAssetCurator::GetSingleton()->NeedsReloadResources();
  }
  else
  {
    if (m_Status.m_Result == plTransformResult::NeedsImport)
    {
      plAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, plAssetInfo::TransformState::NeedsImport);
    }
    else
    {
      plAssetCurator::GetSingleton()->UpdateAssetTransformLog(m_AssetGuid, m_LogEntries);
      plAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, plAssetInfo::TransformState::TransformError);
    }
  }

  PLASMA_LOCK(plAssetCurator::GetSingleton()->m_CuratorMutex);
  plAssetCurator::GetSingleton()->m_Updating.Remove(m_AssetGuid);
  return true;
}

plUInt32 plProcessThread::Run()
{
  plAssetProcessor::GetSingleton()->Run();
  return 0;
}
