#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Dialogs/RemoteConnectionDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GuiFoundation/UIServices/QtWaitForOperationDlg.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>


PLASMA_IMPLEMENT_SINGLETON(plEditorEngineProcessConnection);

plEvent<const plEditorEngineProcessConnection::Event&> plEditorEngineProcessConnection::s_Events;

plEditorEngineProcessConnection::plEditorEngineProcessConnection()
  : m_SingletonRegistrar(this)
{
  m_bProcessShouldBeRunning = false;
  m_bProcessCrashed = false;
  m_bClientIsConfigured = false;

  m_IPC.m_Events.AddEventHandler(plMakeDelegate(&plEditorEngineProcessConnection::HandleIPCEvent, this));
}

plEditorEngineProcessConnection::~plEditorEngineProcessConnection()
{
  m_IPC.m_Events.RemoveEventHandler(plMakeDelegate(&plEditorEngineProcessConnection::HandleIPCEvent, this));
}

void plEditorEngineProcessConnection::HandleIPCEvent(const plProcessCommunicationChannel::Event& e)
{
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<plSyncWithProcessMsgToEditor>())
  {
    const plSyncWithProcessMsgToEditor* msg = static_cast<const plSyncWithProcessMsgToEditor*>(e.m_pMessage);
    m_uiRedrawCountReceived = msg->m_uiRedrawCount;
  }
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<plEditorEngineDocumentMsg>())
  {
    const plEditorEngineDocumentMsg* pMsg = static_cast<const plEditorEngineDocumentMsg*>(e.m_pMessage);

    plAssetDocument* pDocument = m_DocumentByGuid[pMsg->m_DocumentGuid];

    if (pDocument)
    {
      pDocument->HandleEngineMessage(pMsg);
    }
  }
  else if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<plEditorEngineMsg>())
  {
    Event ee;
    ee.m_pMsg = static_cast<const plEditorEngineMsg*>(e.m_pMessage);
    ee.m_Type = Event::Type::ProcessMessage;

    s_Events.Broadcast(ee);
  }
}

void plEditorEngineProcessConnection::UIServicesTickEventHandler(const plQtUiServices::TickEvent& e)
{
  if (e.m_Type == plQtUiServices::TickEvent::Type::EndFrame)
  {
    if (!IsProcessCrashed())
    {
      plSyncWithProcessMsgToEngine sm;
      sm.m_uiRedrawCount = m_uiRedrawCountSent + 1;
      SendMessage(&sm);

      if (m_uiRedrawCountSent > m_uiRedrawCountReceived)
      {
        WaitForMessage(plGetStaticRTTI<plSyncWithProcessMsgToEditor>(), plTime::MakeFromSeconds(2.0)).IgnoreResult();
      }

      ++m_uiRedrawCountSent;
    }
  }
}

plEditorEngineConnection* plEditorEngineProcessConnection::CreateEngineConnection(plAssetDocument* pDocument)
{
  plEditorEngineConnection* pConnection = new plEditorEngineConnection(pDocument);

  m_DocumentByGuid[pDocument->GetGuid()] = pDocument;

  pDocument->SendDocumentOpenMessage(true);

  return pConnection;
}

void plEditorEngineProcessConnection::DestroyEngineConnection(plAssetDocument* pDocument)
{
  pDocument->SendDocumentOpenMessage(false);

  m_DocumentByGuid.Remove(pDocument->GetGuid());

  delete pDocument->GetEditorEngineConnection();
}

void plEditorEngineProcessConnection::Initialize(const plRTTI* pFirstAllowedMessageType)
{
  PLASMA_PROFILE_SCOPE("Initialize");
  if (m_IPC.IsClientAlive())
    return;

  plLog::Dev("Starting Client Engine Process");

  PLASMA_ASSERT_DEBUG(m_TickEventSubscriptionID == 0, "A previous subscription is still in place. ShutdownProcess not called?");
  m_TickEventSubscriptionID = plQtUiServices::s_TickEvent.AddEventHandler(plMakeDelegate(&plEditorEngineProcessConnection::UIServicesTickEventHandler, this));

  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed = false;
  m_bClientIsConfigured = false;

  plStringBuilder tmp;

  QStringList args = QCoreApplication::arguments();
  args.pop_front(); // Remove first argument which is the name of the path to the editor executable

  {
    plStringBuilder sWndCfgPath = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sWndCfgPath.AppendPath("RuntimeConfigs/Window.ddl");

#if PLASMA_ENABLED(PLASMA_MIGRATE_RUNTIMECONFIGS)
    plStringBuilder sWndCfgPathOld = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sWndCfgPathOld.AppendPath("Window.ddl");
    sWndCfgPath = plFileSystem::MigrateFileLocation(sWndCfgPathOld, sWndCfgPath);
#endif

    if (plFileSystem::ExistsFile(sWndCfgPath))
    {
      args << "-wnd";
      args << sWndCfgPath.GetData();
    }
  }

  // set up the EditorEngineProcess telemetry server on a different port
  {
    args << "-TelemetryPort";
    args << plCommandLineUtils::GetGlobalInstance()->GetStringOption("-TelemetryPort", 0, "1050").GetData(tmp);
  }

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  const char* EditorEngineProcessExecutableName = "EditorEngineProcess.exe";
#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
  const char* EditorEngineProcessExecutableName = "EditorEngineProcess";
#else
#  error Platform not supported
#endif


  if (m_IPC.StartClientProcess(EditorEngineProcessExecutableName, args, false, pFirstAllowedMessageType).Failed())
  {
    m_bProcessCrashed = true;
  }
  else
  {
    Event e;
    e.m_Type = Event::Type::ProcessStarted;
    s_Events.Broadcast(e);
  }
}

void plEditorEngineProcessConnection::ActivateRemoteProcess(const plAssetDocument* pDocument, plUInt32 uiViewID)
{
  // make sure process is started
  if (!ConnectToRemoteProcess())
    return;

  // resend entire document
  {
    // open document message
    {
      plDocumentOpenMsgToEngine msg;
      msg.m_DocumentGuid = pDocument->GetGuid();
      msg.m_bDocumentOpen = true;
      msg.m_sDocumentType = pDocument->GetDocumentTypeDescriptor()->m_sDocumentTypeName;
      m_pRemoteProcess->SendMessage(&msg);
    }

    if (pDocument->GetDynamicRTTI()->IsDerivedFrom<plAssetDocument>())
    {
      plAssetDocument* pAssetDoc = (plAssetDocument*)pDocument;
      plDocumentOpenResponseMsgToEditor response;
      response.m_DocumentGuid = pDocument->GetGuid();
      pAssetDoc->HandleEngineMessage(&response);
    }
  }

  // send activation message
  {
    plActivateRemoteViewMsgToEngine msg;
    msg.m_DocumentGuid = pDocument->GetGuid();
    msg.m_uiViewID = uiViewID;
    m_pRemoteProcess->SendMessage(&msg);
  }
}

bool plEditorEngineProcessConnection::ConnectToRemoteProcess()
{
  if (m_pRemoteProcess != nullptr)
  {
    if (m_pRemoteProcess->IsConnected())
      return true;

    ShutdownRemoteProcess();
  }

  plQtRemoteConnectionDlg dlg(QApplication::activeWindow());

  if (dlg.exec() == QDialog::Rejected)
    return false;

  m_pRemoteProcess = PLASMA_DEFAULT_NEW(plEditorProcessRemoteCommunicationChannel);
  m_pRemoteProcess->ConnectToServer(dlg.GetResultingAddress().toUtf8().data()).IgnoreResult();

  plQtWaitForOperationDlg waitDialog(QApplication::activeWindow());
  waitDialog.m_OnIdle = [this]() -> bool {
    if (m_pRemoteProcess->IsConnected())
      return false;

    m_pRemoteProcess->TryConnect();
    return true;
  };

  const int iRet = waitDialog.exec();

  if (iRet == QDialog::Accepted)
  {
    // Send project setup.
    plSetupProjectMsgToEngine msg;
    msg.m_sProjectDir = plToolsProject::GetSingleton()->GetProjectDirectory();
    msg.m_FileSystemConfig = m_FileSystemConfig;
    msg.m_PluginConfig = m_PluginConfig;
    msg.m_sFileserveAddress = dlg.GetResultingFsAddress().toUtf8().data();
    msg.m_sAssetProfile = plAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();

    m_pRemoteProcess->SendMessage(&msg);
  }

  return iRet == QDialog::Accepted;
}


void plEditorEngineProcessConnection::ShutdownRemoteProcess()
{
  if (m_pRemoteProcess != nullptr)
  {
    plLog::Info("Shutting down Remote Engine Process");
    m_pRemoteProcess->CloseConnection();

    m_pRemoteProcess = nullptr;
  }
}

void plEditorEngineProcessConnection::ShutdownProcess()
{
  if (!m_bProcessShouldBeRunning)
    return;

  ShutdownRemoteProcess();

  plLog::Info("Shutting down Engine Process");

  if (m_TickEventSubscriptionID != 0)
    plQtUiServices::s_TickEvent.RemoveEventHandler(m_TickEventSubscriptionID);

  m_bClientIsConfigured = false;
  m_bProcessShouldBeRunning = false;
  m_IPC.CloseConnection();

  Event e;
  e.m_Type = Event::Type::ProcessShutdown;
  s_Events.Broadcast(e);
}

bool plEditorEngineProcessConnection::SendMessage(plProcessMessage* pMessage)
{
  bool res = m_IPC.SendMessage(pMessage);

  if (m_pRemoteProcess)
  {
    m_pRemoteProcess->SendMessage(pMessage);
  }
  return res;
}

plResult plEditorEngineProcessConnection::WaitForMessage(const plRTTI* pMessageType, plTime timeout, plProcessCommunicationChannel::WaitForMessageCallback* pCallback)
{
  PLASMA_PROFILE_SCOPE(pMessageType->GetTypeName());
  return m_IPC.WaitForMessage(pMessageType, timeout, pCallback);
}

plResult plEditorEngineProcessConnection::WaitForDocumentMessage(const plUuid& assetGuid, const plRTTI* pMessageType, plTime timeout, plProcessCommunicationChannel::WaitForMessageCallback* pCallback /*= nullptr*/)
{
  if (!m_bProcessShouldBeRunning)
  {
    return PLASMA_FAILURE; // if the process is not running, we can't wait for a message
  }
  PLASMA_ASSERT_DEBUG(pMessageType->IsDerivedFrom(plGetStaticRTTI<plEditorEngineDocumentMsg>()), "The type of the message to wait for must be a document message.");
  struct WaitData
  {
    plUuid m_AssetGuid;
    plProcessCommunicationChannel::WaitForMessageCallback* m_pCallback;
  };

  WaitData data;
  data.m_AssetGuid = assetGuid;
  data.m_pCallback = pCallback;

  plProcessCommunicationChannel::WaitForMessageCallback callback = [&data](plProcessMessage* pMsg) -> bool {
    plEditorEngineDocumentMsg* pMsg2 = plDynamicCast<plEditorEngineDocumentMsg*>(pMsg);
    if (pMsg2 && data.m_AssetGuid == pMsg2->m_DocumentGuid)
    {
      if (data.m_pCallback && data.m_pCallback->IsValid() && !(*data.m_pCallback)(pMsg))
      {
        return false;
      }
      return true;
    }
    return false;
  };

  return m_IPC.WaitForMessage(pMessageType, timeout, &callback);
}

plResult plEditorEngineProcessConnection::RestartProcess()
{
  PLASMA_PROFILE_SCOPE("RestartProcess");
  PLASMA_LOG_BLOCK("Restarting Engine Process");

  plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Reloading Engine Process...", plTime::MakeFromSeconds(5));

  ShutdownProcess();

  Initialize(plGetStaticRTTI<plSetupProjectMsgToEngine>());

  if (m_bProcessCrashed)
  {
    plLog::Error("Engine process crashed during startup.");
    ShutdownProcess();
    return PLASMA_FAILURE;
  }

  plLog::Dev("Waiting for IPC connection");

  if (m_IPC.WaitForConnection(plTime()).Failed())
  {
    plLog::Error("Engine process did not connect. Engine process output:\n{}", m_IPC.GetStdoutContents());
    ShutdownProcess();
    return PLASMA_FAILURE;
  }

  {
    // Send project setup.
    plSetupProjectMsgToEngine msg;
    msg.m_sProjectDir = plToolsProject::GetSingleton()->GetProjectDirectory();
    msg.m_FileSystemConfig = m_FileSystemConfig;
    msg.m_PluginConfig = m_PluginConfig;
    msg.m_sAssetProfile = plAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();

    SendMessage(&msg);
  }

  plLog::Dev("Waiting for Engine Process response");

  if (WaitForMessage(plGetStaticRTTI<plProjectReadyMsgToEditor>(), plTime()).Failed())
  {
    plLog::Error("Failed to restart the engine process. Engine Process Output:\n", m_IPC.GetStdoutContents());
    ShutdownProcess();
    return PLASMA_FAILURE;
  }

  plLog::Dev("Transmitting open documents to Engine Process");

  plHybridArray<plAssetDocument*, 6> docs;
  docs.Reserve(m_DocumentByGuid.GetCount());

  // Resend all open documents. Make sure to send main documents before child documents.
  for (auto it = m_DocumentByGuid.GetIterator(); it.IsValid(); ++it)
  {
    docs.PushBack(it.Value());
  }
  docs.Sort([](const plAssetDocument* a, const plAssetDocument* b) {
    if (a->IsMainDocument() != b->IsMainDocument())
      return a->IsMainDocument();
    return a < b; });

  for (plAssetDocument* pDoc : docs)
  {
    pDoc->SendDocumentOpenMessage(true);
  }

  plAssetCurator::GetSingleton()->InvalidateAssetsWithTransformState(plAssetInfo::TransformState::TransformError);

  plLog::Success("Engine Process is running");

  m_bClientIsConfigured = true;

  Event e;
  e.m_Type = Event::Type::ProcessRestarted;
  s_Events.Broadcast(e);

  return PLASMA_SUCCESS;
}

void plEditorEngineProcessConnection::Update()
{
  if (!m_bProcessShouldBeRunning)
    return;

  if (!m_IPC.IsClientAlive())
  {
    ShutdownProcess();
    m_bProcessCrashed = true;

    Event e;
    e.m_Type = Event::Type::ProcessCrashed;
    s_Events.Broadcast(e);

    return;
  }

  m_IPC.ProcessMessages();

  if (m_pRemoteProcess)
  {
    m_pRemoteProcess->ProcessMessages();
  }
}

bool plEditorEngineConnection::SendMessage(plEditorEngineDocumentMsg* pMessage)
{
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wtautological-undefined-compare"
#endif
  PLASMA_ASSERT_DEV(this != nullptr, "No connection between editor and engine was created. This typically happens when an asset document does "
                                 "not enable the engine-connection through the constructor of plAssetDocument."); // NOLINT
#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif
  pMessage->m_DocumentGuid = m_pDocument->GetGuid();

  return plEditorEngineProcessConnection::GetSingleton()->SendMessage(pMessage);
}

void plEditorEngineConnection::SendHighlightObjectMessage(plViewHighlightMsgToEngine* pMessage)
{
  // without this check there will be so many messages, that the editor comes to a crawl (< 10 FPS)
  // This happens because Qt sends hundreds of mouse-move events and since each 'SendMessageToEngine'
  // requires a round-trip to the engine process, doing this too often will be sloooow

  static plUuid LastHighlightGuid;

  if (LastHighlightGuid == pMessage->m_HighlightObject)
    return;

  LastHighlightGuid = pMessage->m_HighlightObject;
  SendMessage(pMessage);
}
