#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Dialogs/RemoteConnectionDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GuiFoundation/UIServices/QtWaitForOperationDlg.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>


PLASMA_IMPLEMENT_SINGLETON(PlasmaEditorEngineProcessConnection);

plEvent<const PlasmaEditorEngineProcessConnection::Event&> PlasmaEditorEngineProcessConnection::s_Events;

PlasmaEditorEngineProcessConnection::PlasmaEditorEngineProcessConnection()
  : m_SingletonRegistrar(this)
{
  m_bProcessShouldBeRunning = false;
  m_bProcessCrashed = false;
  m_bClientIsConfigured = false;

  m_IPC.m_Events.AddEventHandler(plMakeDelegate(&PlasmaEditorEngineProcessConnection::HandleIPCEvent, this));
}

PlasmaEditorEngineProcessConnection::~PlasmaEditorEngineProcessConnection()
{
  m_IPC.m_Events.RemoveEventHandler(plMakeDelegate(&PlasmaEditorEngineProcessConnection::HandleIPCEvent, this));
}

void PlasmaEditorEngineProcessConnection::SendDocumentOpenMessage(const plAssetDocument* pDocument, bool bOpen)
{
  PLASMA_PROFILE_SCOPE("SendDocumentOpenMessage");

  if (!pDocument)
    return;

  // it is important to have up-to-date lookup tables in the engine process, because document contexts might try to
  // load resources, and if the file redirection does not happen correctly, derived resource types may not be created as they should
  plAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();

  plDocumentOpenMsgToEngine m;
  m.m_DocumentGuid = pDocument->GetGuid();
  m.m_bDocumentOpen = bOpen;
  m.m_sDocumentType = pDocument->GetDocumentTypeDescriptor()->m_sDocumentTypeName;
  m.m_DocumentMetaData = pDocument->GetCreateEngineMetaData();

  SendMessage(&m);
}

void PlasmaEditorEngineProcessConnection::HandleIPCEvent(const plProcessCommunicationChannel::Event& e)
{
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<plSyncWithProcessMsgToEditor>())
  {
    const plSyncWithProcessMsgToEditor* msg = static_cast<const plSyncWithProcessMsgToEditor*>(e.m_pMessage);
    m_uiRedrawCountReceived = msg->m_uiRedrawCount;
  }
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<PlasmaEditorEngineDocumentMsg>())
  {
    const PlasmaEditorEngineDocumentMsg* pMsg = static_cast<const PlasmaEditorEngineDocumentMsg*>(e.m_pMessage);

    plAssetDocument* pDocument = nullptr;
    if(m_DocumentByGuid.TryGetValue(pMsg->m_DocumentGuid, pDocument))
    {
      pDocument->HandleEngineMessage(pMsg);
    }
  }
  else if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<PlasmaEditorEngineMsg>())
  {
    Event ee;
    ee.m_pMsg = static_cast<const PlasmaEditorEngineMsg*>(e.m_pMessage);
    ee.m_Type = Event::Type::ProcessMessage;

    s_Events.Broadcast(ee);
  }
}

void PlasmaEditorEngineProcessConnection::UIServicesTickEventHandler(const plQtUiServices::TickEvent& e)
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
        WaitForMessage(plGetStaticRTTI<plSyncWithProcessMsgToEditor>(), plTime::Seconds(2.0)).IgnoreResult();
      }

      ++m_uiRedrawCountSent;
    }
  }
}

PlasmaEditorEngineConnection* PlasmaEditorEngineProcessConnection::CreateEngineConnection(plAssetDocument* pDocument)
{
  PlasmaEditorEngineConnection* pConnection = new PlasmaEditorEngineConnection(pDocument);

  m_DocumentByGuid[pDocument->GetGuid()] = pDocument;

  SendDocumentOpenMessage(pDocument, true);

  return pConnection;
}

void PlasmaEditorEngineProcessConnection::DestroyEngineConnection(plAssetDocument* pDocument)
{
  SendDocumentOpenMessage(pDocument, false);

  m_DocumentByGuid.Remove(pDocument->GetGuid());

  delete pDocument->GetEditorEngineConnection();
}

void PlasmaEditorEngineProcessConnection::Initialize(const plRTTI* pFirstAllowedMessageType)
{
  PLASMA_PROFILE_SCOPE("Initialize");
  if (m_IPC.IsClientAlive())
    return;

  plLog::Dev("Starting Client Engine Process");

  PLASMA_ASSERT_DEBUG(m_TickEventSubscriptionID == 0, "A previous subscription is still in place. ShutdownProcess not called?");
  m_TickEventSubscriptionID = plQtUiServices::s_TickEvent.AddEventHandler(plMakeDelegate(&PlasmaEditorEngineProcessConnection::UIServicesTickEventHandler, this));

  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed = false;
  m_bClientIsConfigured = false;

  plStringBuilder tmp;
  
  QStringList args;
  if (m_bProcessShouldWaitForDebugger)
  {
    args << "-debug";
  }

  if (!m_sRenderer.IsEmpty())
  {
    args << "-renderer";
    args << m_sRenderer.GetData();
  }

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

void PlasmaEditorEngineProcessConnection::ActivateRemoteProcess(const plAssetDocument* pDocument, plUInt32 uiViewID)
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

bool PlasmaEditorEngineProcessConnection::ConnectToRemoteProcess()
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

  m_pRemoteProcess = PLASMA_DEFAULT_NEW(PlasmaEditorProcessRemoteCommunicationChannel);
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


void PlasmaEditorEngineProcessConnection::ShutdownRemoteProcess()
{
  if (m_pRemoteProcess != nullptr)
  {
    plLog::Info("Shutting down Remote Engine Process");
    m_pRemoteProcess->CloseConnection();

    m_pRemoteProcess = nullptr;
  }
}

void PlasmaEditorEngineProcessConnection::ShutdownProcess()
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

void PlasmaEditorEngineProcessConnection::SendMessage(plProcessMessage* pMessage)
{
  m_IPC.SendMessage(pMessage);

  if (m_pRemoteProcess)
  {
    m_pRemoteProcess->SendMessage(pMessage);
  }
}

plResult PlasmaEditorEngineProcessConnection::WaitForMessage(const plRTTI* pMessageType, plTime tTimeout, plProcessCommunicationChannel::WaitForMessageCallback* pCallback)
{
  PLASMA_PROFILE_SCOPE(pMessageType->GetTypeName());
  return m_IPC.WaitForMessage(pMessageType, tTimeout, pCallback);
}

plResult PlasmaEditorEngineProcessConnection::WaitForDocumentMessage(const plUuid& assetGuid, const plRTTI* pMessageType, plTime tTimeout, plProcessCommunicationChannel::WaitForMessageCallback* pCallback /*= nullptr*/)
{
  if (!m_bProcessShouldBeRunning)
  {
    return PLASMA_FAILURE; // if the process is not running, we can't wait for a message
  }
  PLASMA_ASSERT_DEBUG(pMessageType->IsDerivedFrom(plGetStaticRTTI<PlasmaEditorEngineDocumentMsg>()), "The type of the message to wait for must be a document message.");
  struct WaitData
  {
    plUuid m_AssetGuid;
    plProcessCommunicationChannel::WaitForMessageCallback* m_pCallback;
  };

  WaitData data;
  data.m_AssetGuid = assetGuid;
  data.m_pCallback = pCallback;

  plProcessCommunicationChannel::WaitForMessageCallback callback = [&data](plProcessMessage* pMsg) -> bool {
    PlasmaEditorEngineDocumentMsg* pMsg2 = plDynamicCast<PlasmaEditorEngineDocumentMsg*>(pMsg);
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

  return m_IPC.WaitForMessage(pMessageType, tTimeout, &callback);
}

plResult PlasmaEditorEngineProcessConnection::RestartProcess()
{
  PLASMA_PROFILE_SCOPE("RestartProcess");
  PLASMA_LOG_BLOCK("Restarting Engine Process");

  plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Reloading Engine Process...", plTime::Seconds(5));

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
    return a < b;
  });

  for (plAssetDocument* pDoc : docs)
  {
    SendDocumentOpenMessage(pDoc, true);
  }

  plAssetCurator::GetSingleton()->InvalidateAssetsWithTransformState(plAssetInfo::TransformState::TransformError);

  plLog::Success("Engine Process is running");

  m_bClientIsConfigured = true;

  Event e;
  e.m_Type = Event::Type::ProcessRestarted;
  s_Events.Broadcast(e);

  return PLASMA_SUCCESS;
}

void PlasmaEditorEngineProcessConnection::Update()
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

void PlasmaEditorEngineConnection::SendMessage(PlasmaEditorEngineDocumentMsg* pMessage)
{
  PLASMA_ASSERT_DEV(this != nullptr, "No connection between editor and engine was created. This typically happens when an asset document does "
                                 "not enable the engine-connection through the constructor of plAssetDocument.");

  pMessage->m_DocumentGuid = m_pDocument->GetGuid();

  PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(pMessage);
}

void PlasmaEditorEngineConnection::SendHighlightObjectMessage(plViewHighlightMsgToEngine* pMessage)
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
