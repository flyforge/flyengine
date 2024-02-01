#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetProcessorMessages.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

PL_IMPLEMENT_SINGLETON(plAssetProcessor);

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetProcessor)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "AssetCurator"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    PL_DEFAULT_NEW(plAssetProcessor);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plAssetProcessor* pDummy = plAssetProcessor::GetSingleton();
    PL_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// plCuratorLog
////////////////////////////////////////////////////////////////////////

void plAssetProcessorLog::HandleLogMessage(const plLoggingEventData& le)
{
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
  if (m_pThread)
  {
    m_pThread->Join();
    m_pThread.Clear();
  }
  PL_ASSERT_DEV(m_ProcessTaskState == ProcessTaskState::Stopped, "Call StopProcessTask first before destroying the plAssetProcessor.");
}

void plAssetProcessor::StartProcessTask()
{
  PL_LOCK(m_ProcessorMutex);
  if (m_ProcessTaskState != ProcessTaskState::Stopped)
  {
    return;
  }

  // Join old thread.
  if (m_pThread)
  {
    m_pThread->Join();
    m_pThread.Clear();
  }

  m_ProcessTaskState = ProcessTaskState::Running;

  const plUInt32 uiWorkerCount = plTaskSystem::GetWorkerThreadCount(plWorkerThreadType::LongTasks);
  m_ProcessTasks.SetCount(uiWorkerCount);

  for (plUInt32 idx = 0; idx < uiWorkerCount; ++idx)
  {
    m_ProcessTasks[idx].m_uiProcessorID = idx;
  }

  m_pThread = PL_DEFAULT_NEW(plProcessThread);
  m_pThread->Start();

  {
    plAssetProcessorEvent e;
    e.m_Type = plAssetProcessorEvent::Type::ProcessTaskStateChanged;
    m_Events.Broadcast(e);
  }
}

void plAssetProcessor::StopProcessTask(bool bForce)
{
  {
    PL_LOCK(m_ProcessorMutex);
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
    m_pThread->Join();
    m_pThread.Clear();
    PL_ASSERT_DEV(m_ProcessTaskState == ProcessTaskState::Stopped, "Process task shoul have set the state to stopped.");
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
      m_ProcessTasks[i].Tick(true);
    }
    plThreadUtils::Sleep(plTime::MakeFromMilliseconds(100));
  }

  while (true)
  {
    bool bAnyRunning = false;

    for (plUInt32 i = 0; i < m_ProcessTasks.GetCount(); i++)
    {
      if (m_bForceStop)
        m_ProcessTasks[i].ShutdownProcess();

      bAnyRunning |= m_ProcessTasks[i].Tick(false);
    }

    if (bAnyRunning)
      plThreadUtils::Sleep(plTime::MakeFromMilliseconds(100));
    else
      break;
  }

  PL_LOCK(m_ProcessorMutex);
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
  : m_Status(PL_SUCCESS)
{
  m_pIPC = PL_DEFAULT_NEW(plEditorProcessCommunicationChannel);
  m_pIPC->m_Events.AddEventHandler(plMakeDelegate(&plProcessTask::EventHandlerIPC, this));
}

plProcessTask::~plProcessTask()
{
  ShutdownProcess();
  m_pIPC->m_Events.RemoveEventHandler(plMakeDelegate(&plProcessTask::EventHandlerIPC, this));
  PL_DEFAULT_DELETE(m_pIPC);
}


plResult plProcessTask::StartProcess()
{
  const plRTTI* pFirstAllowedMessageType = nullptr;

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

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
  const char* EditorProcessorExecutable = "EditorProcessor.exe";
#else
  const char* EditorProcessorExecutable = "EditorProcessor";
#endif

  if (m_pIPC->StartClientProcess(EditorProcessorExecutable, args, false, pFirstAllowedMessageType).Failed())
  {
    return PL_FAILURE;
  }
  return PL_SUCCESS;
}

void plProcessTask::ShutdownProcess()
{
  m_pIPC->CloseConnection();
}

void plProcessTask::EventHandlerIPC(const plProcessCommunicationChannel::Event& e)
{
  if (const plProcessAssetResponseMsg* pMsg = plDynamicCast<const plProcessAssetResponseMsg*>(e.m_pMessage))
  {
    PL_ASSERT_DEV(m_State == State::Processing, "Message handling should only happen when currently processing");
    m_Status = pMsg->m_Status;
    m_State = State::ReportResult;
    m_LogEntries.Swap(pMsg->m_LogEntries);
  }
}

bool plProcessTask::GetNextAssetToProcess(plAssetInfo* pInfo, plUuid& out_guid, plDataDirPath& out_path)
{
  bool bComplete = true;

  const plDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (plDocumentManager::FindDocumentTypeFromPath(pInfo->m_Path, false, pTypeDesc).Succeeded())
  {
    auto flags = static_cast<const plAssetDocumentTypeDescriptor*>(pTypeDesc)->m_AssetDocumentFlags;

    if (flags.IsAnySet(plAssetDocumentFlags::OnlyTransformManually | plAssetDocumentFlags::DisableTransform))
      return false;
  }

  auto TestFunc = [this, &bComplete](const plSet<plString>& files) -> plAssetInfo* {
    for (const auto& sFile : files)
    {
      if (plAssetInfo* pFileInfo = plAssetCurator::GetSingleton()->GetAssetInfo(sFile))
      {
        switch (pFileInfo->m_TransformState)
        {
          case plAssetInfo::TransformState::Unknown:
          case plAssetInfo::TransformState::TransformError:
          case plAssetInfo::TransformState::MissingTransformDependency:
          case plAssetInfo::TransformState::MissingThumbnailDependency:
          case plAssetInfo::TransformState::CircularDependency:
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

            PL_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
    }
    return nullptr;
  };

  if (plAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_TransformDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_path);
  }

  if (plAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_ThumbnailDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_path);
  }

  // not needed to go through package dependencies here

  if (bComplete && !plAssetCurator::GetSingleton()->m_Updating.Contains(pInfo->m_Info->m_DocumentID) &&
      !plAssetCurator::GetSingleton()->m_TransformStateStale.Contains(pInfo->m_Info->m_DocumentID))
  {
    plAssetCurator::GetSingleton()->m_Updating.Insert(pInfo->m_Info->m_DocumentID);
    out_guid = pInfo->m_Info->m_DocumentID;
    out_path = pInfo->m_Path;
    return true;
  }

  return false;
}

bool plProcessTask::GetNextAssetToProcess(plUuid& out_guid, plDataDirPath& out_path)
{
  PL_LOCK(plAssetCurator::GetSingleton()->m_CuratorMutex);

  for (auto it = plAssetCurator::GetSingleton()->m_TransformState[plAssetInfo::TransformState::NeedsTransform].GetIterator(); it.IsValid(); ++it)
  {
    plAssetInfo* pInfo = plAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_path);
      if (bRes)
        return true;
    }
  }

  for (auto it = plAssetCurator::GetSingleton()->m_TransformState[plAssetInfo::TransformState::NeedsThumbnail].GetIterator(); it.IsValid(); ++it)
  {
    plAssetInfo* pInfo = plAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_path);
      if (bRes)
        return true;
    }
  }

  return false;
}


void plProcessTask::OnProcessCrashed(plStringView message)
{
  ShutdownProcess();
  m_Status = plStatus(message);
  plLogEntryDelegate logger([this](plLogEntry& ref_entry) { m_LogEntries.PushBack(std::move(ref_entry)); });
  plLog::Error(&logger, message);
  plLog::Error(&plAssetProcessor::GetSingleton()->m_CuratorLog, message);
}

bool plProcessTask::IsConnected()
{
  return m_pIPC->IsConnected();
}

bool plProcessTask::HasProcessCrashed()
{
  return m_pIPC->IsClientAlive();
}

bool plProcessTask::Tick(bool bStartNewWork)
{
  while (true)
  {
    switch (m_State)
    {
      case State::LookingForWork:
      {
        if (!bStartNewWork)
        {
          return false; // don't call later
        }
        m_LogEntries.Clear();
        m_TransitiveHull.Clear();
        m_Status = plStatus(PL_SUCCESS);
        {
          PL_LOCK(plAssetCurator::GetSingleton()->m_CuratorMutex);

          if (!GetNextAssetToProcess(m_AssetGuid, m_AssetPath))
          {
            m_AssetGuid = plUuid();
            m_AssetPath.Clear();
            return bStartNewWork; // call again if we should be looking for new work
          }

          plAssetInfo::TransformState state = plAssetCurator::GetSingleton()->IsAssetUpToDate(m_AssetGuid, nullptr, nullptr, m_uiAssetHash, m_uiThumbHash);
          PL_ASSERT_DEV(state == plAssetInfo::TransformState::NeedsTransform || state == plAssetInfo::TransformState::NeedsThumbnail, "An asset was selected that is already up to date.");

          plSet<plString> dependencies;
          plStringBuilder sTemp;
          plAssetCurator::GetSingleton()->GenerateTransitiveHull(plConversionUtils::ToString(m_AssetGuid, sTemp), dependencies, true, true);

          m_TransitiveHull.Reserve(dependencies.GetCount());
          for (const plString& str : dependencies)
          {
            m_TransitiveHull.PushBack(str);
          }
        }

        if (!m_pIPC->IsClientAlive() || !m_pIPC->IsConnected())
        {
          if (StartProcess().Failed())
          {
            m_State = State::ReportResult;
            OnProcessCrashed("Asset processor did not launch");
          }
          else
          {
            m_State = State::WaitingForConnection;
            return true; // call again later
          }
        }
        else
        {
          m_State = State::Ready;
        }
      }
      break;
      case State::WaitingForConnection:
      {
        if (!m_pIPC->IsClientAlive())
        {
          m_State = State::ReportResult;
          OnProcessCrashed("Asset processor crashed while waiting for connection");
          break;
        }

        if (m_pIPC->IsConnected())
        {
          m_State = State::Ready;
        }
      }
      break;
      case State::Ready:
      {
        plLog::Info(&plAssetProcessor::GetSingleton()->m_CuratorLog, "Processing '{0}'", m_AssetPath.GetDataDirRelativePath());
        // Send and wait
        plProcessAssetMsg msg;
        msg.m_AssetGuid = m_AssetGuid;
        msg.m_AssetHash = m_uiAssetHash;
        msg.m_ThumbHash = m_uiThumbHash;
        msg.m_sAssetPath = m_AssetPath;
        msg.m_DepRefHull.Swap(m_TransitiveHull);
        msg.m_sPlatform = plAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();

        if (m_pIPC->SendMessage(&msg))
        {
          m_State = State::Processing;
          return true; // call again later
        }
        else
        {
          m_State = State::ReportResult;
          OnProcessCrashed("Asset processor crashed, failed to send message");
        }
      }
      break;
      case State::Processing:
      {
        m_pIPC->ProcessMessages();
        if (!m_pIPC->IsClientAlive())
        {
          OnProcessCrashed("Asset Processor crashed during processing");
          m_State = State::ReportResult;
        }
      }
      break;
      case State::ReportResult:
      {
        if (m_Status.Succeeded())
        {
          plAssetCurator::GetSingleton()->NotifyOfAssetChange(m_AssetGuid);
          plAssetCurator::GetSingleton()->NeedsReloadResources(m_AssetGuid);
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

        {
          PL_LOCK(plAssetCurator::GetSingleton()->m_CuratorMutex);
          plAssetCurator::GetSingleton()->m_Updating.Remove(m_AssetGuid);
        }

        m_State = State::LookingForWork;
      }
      break;
    }
  }
}

plUInt32 plProcessThread::Run()
{
  plAssetProcessor::GetSingleton()->Run();
  return 0;
}
