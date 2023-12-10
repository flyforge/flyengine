#include <EditorEngineProcess/EditorEngineProcessPCH.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/System/SystemInformation.h>

#include <Core/Console/QuakeConsole.h>
#include <EditorEngineProcess/EngineProcGameApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#  include <shellscalingapi.h>
#endif
#include <Foundation/Profiling/ProfilingUtils.h>

// Will forward assert messages and crash handler messages to the log system and then to the editor.
// Note that this is unsafe as in some crash situation allocating memory will not be possible but it's better to have some logs compared to none.
void EditorPrintFunction(const char* szText)
{
  plLog::Info("{}", szText);
}

static plAssertHandler g_PreviousAssertHandler = nullptr;

plEngineProcessGameApplication::plEngineProcessGameApplication()
  : plGameApplication("plEditorEngineProcess", nullptr)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
#endif

  m_LongOpWorkerManager.Startup(&m_IPC);
}

plEngineProcessGameApplication::~plEngineProcessGameApplication() = default;

plResult plEngineProcessGameApplication::BeforeCoreSystemsStartup()
{
  m_pApp = CreateEngineProcessApp();
  plStartup::AddApplicationTag("editorengineprocess");

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
  // Make sure to disable the fileserve plugin
  plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-fs_off");
#endif

  AddEditorAssertHandler();
  return SUPER::BeforeCoreSystemsStartup();
}

void plEngineProcessGameApplication::AfterCoreSystemsStartup()
{
  // skip project creation at this point
  // SUPER::AfterCoreSystemsStartup();

#if PLASMA_DISABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP) && PLASMA_DISABLED(PLASMA_PLATFORM_LINUX)
  {
    // on all 'mobile' platforms, we assume we are in remote mode
    plEditorEngineProcessApp::GetSingleton()->SetRemoteMode();
  }
#else
  if (plCommandLineUtils::GetGlobalInstance()->GetBoolOption("-remote", false))
  {
    plEditorEngineProcessApp::GetSingleton()->SetRemoteMode();
  }
#endif

  WaitForDebugger();

  DisableErrorReport();

  plTaskSystem::SetTargetFrameTime(plTime::MakeFromSeconds(1.0 / 20.0));

  ConnectToHost();
}


void plEngineProcessGameApplication::ConnectToHost()
{
  PLASMA_VERIFY(m_IPC.ConnectToHostProcess().Succeeded(), "Could not connect to host");

  m_IPC.m_Events.AddEventHandler(plMakeDelegate(&plEngineProcessGameApplication::EventHandlerIPC, this));

  // wait indefinitely (not necessary anymore, should work regardless)
  // m_IPC.WaitForMessage(plGetStaticRTTI<plSetupProjectMsgToEngine>(), plTime());
}

void plEngineProcessGameApplication::DisableErrorReport()
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
  // this also speeds up process termination significantly (down to less than a second)
  DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
  SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif
}

void plEngineProcessGameApplication::WaitForDebugger()
{
  if (plCommandLineUtils::GetGlobalInstance()->GetBoolOption("-WaitForDebugger"))
  {
    while (!plSystemInformation::IsDebuggerAttached())
    {
      plThreadUtils::Sleep(plTime::MakeFromMilliseconds(10));
    }
  }
}

bool plEngineProcessGameApplication::EditorAssertHandler(const char* szSourceFile, plUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  plLog::Error("*** Assertion ***:\nFile: \"{}\",\nLine: \"{}\",\nFunction: \"{}\",\nExpression: \"{}\",\nMessage: \"{}\"", szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);
  // Wait for flush of IPC messages
  plThreadUtils::Sleep(plTime::MakeFromMilliseconds(500));

  if (g_PreviousAssertHandler)
    return g_PreviousAssertHandler(szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

  return true;
}

void plEngineProcessGameApplication::AddEditorAssertHandler()
{
  g_PreviousAssertHandler = plGetAssertHandler();
  plSetAssertHandler(EditorAssertHandler);
}

void plEngineProcessGameApplication::RemoveEditorAssertHandler()
{
  plSetAssertHandler(g_PreviousAssertHandler);
  g_PreviousAssertHandler = nullptr;
}

void plEngineProcessGameApplication::BeforeCoreSystemsShutdown()
{
  RemoveEditorAssertHandler();

  m_pApp = nullptr;

  m_LongOpWorkerManager.Shutdown();

  m_IPC.m_Events.RemoveEventHandler(plMakeDelegate(&plEngineProcessGameApplication::EventHandlerIPC, this));

  SUPER::BeforeCoreSystemsShutdown();
}

plApplication::Execution plEngineProcessGameApplication::Run()
{
  bool bPendingOpInProgress = false;
  do
  {
    bPendingOpInProgress = plEngineProcessDocumentContext::PendingOperationsInProgress();
    if (ProcessIPCMessages(bPendingOpInProgress))
    {
      plEngineProcessDocumentContext::UpdateDocumentContexts();
    }
  } while (!bPendingOpInProgress && m_uiRedrawCountExecuted == m_uiRedrawCountReceived);

  m_uiRedrawCountExecuted = m_uiRedrawCountReceived;

  // Normally rendering is done in EventHandlerIPC as a response to plSyncWithProcessMsgToEngine. However, when playing or when pending operations are in progress we need to render even if we didn't receive a draw request.
  plApplication::Execution res = SUPER::Run();
  plRenderWorld::ClearMainViews();
  return res;
}

void plEngineProcessGameApplication::LogWriter(const plLoggingEventData& e)
{
  plLogMsgToEditor msg;
  msg.m_Entry = plLogEntry(e);

  // the editor does not care about flushing caches, so no need to send this over
  if (msg.m_Entry.m_Type == plLogMsgType::Flush)
    return;

  if (msg.m_Entry.m_sTag == "IPC")
    return;

  // Prevent infinite recursion by disabeling logging until we are done sending the message
  PLASMA_LOG_BLOCK_MUTE();

  m_IPC.SendMessage(&msg);
}

static bool EmptyAssertHandler(const char* szSourceFile, plUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  return false;
}

bool plEngineProcessGameApplication::ProcessIPCMessages(bool bPendingOpInProgress)
{
  PLASMA_PROFILE_SCOPE("ProcessIPCMessages");

  if (!m_IPC.IsHostAlive()) // check whether the host crashed
  {
    // The problem here is, that the editor process crashed (or was terminated through Visual Studio),
    // but our process depends on it for cleanup!
    // That means, this process created rendering resources through a device that is bound to a window handle, which belonged to the editor
    // process. So now we can't clean up, and therefore we can only crash. Therefore we try to crash as silently as possible.

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    // Make sure that Windows doesn't show a default message box when we call abort
    _set_abort_behavior(0, _WRITE_ABORT_MSG);
    TerminateProcess(GetCurrentProcess(), 0);
#endif

    // The OS will still call destructors for our objects (even though we called abort ... what a pointless design).
    // Our code might assert on destruction, so make sure our assert handler doesn't show anything.
    plSetAssertHandler(EmptyAssertHandler);
    std::abort();
  }
  else
  {
    // if an operation is still pending or this process is a remote process, we do NOT want to block
    // remote processes shall run as fast as they can
    if (bPendingOpInProgress || plEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    {
      m_IPC.ProcessMessages();
    }
    else
    {
      PLASMA_PROFILE_SCOPE("WaitForMessages");
      // Only suspend and wait if no more pending ops need to be done.
      m_IPC.WaitForMessages();
    }
    return true;
  }
}

void plEngineProcessGameApplication::SendProjectReadyMessage()
{
  plProjectReadyMsgToEditor msg;
  m_IPC.SendMessage(&msg);
}

void plEngineProcessGameApplication::SendReflectionInformation()
{
  if (plEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  plSet<const plRTTI*> types;
  plRTTI::ForEachType(
    [&](const plRTTI* pRtti) {
      if (pRtti->GetTypeFlags().IsSet(plTypeFlags::StandardType) == false)
      {
        types.Insert(pRtti);
      }
    });

  plDynamicArray<const plRTTI*> sortedTypes;
  plReflectionUtils::CreateDependencySortedTypeArray(types, sortedTypes).AssertSuccess("Sorting failed");

  for (auto type : sortedTypes)
  {
    plUpdateReflectionTypeMsgToEditor TypeMsg;
    plToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(type, TypeMsg.m_desc);
    m_IPC.SendMessage(&TypeMsg);
  }
}

void plEngineProcessGameApplication::EventHandlerIPC(const plEngineProcessCommunicationChannel::Event& e)
{
  if (const auto* pMsg = plDynamicCast<const plSyncWithProcessMsgToEngine*>(e.m_pMessage))
  {
    plSyncWithProcessMsgToEditor msg;
    msg.m_uiRedrawCount = pMsg->m_uiRedrawCount;
    m_uiRedrawCountReceived = msg.m_uiRedrawCount;

    // We must clear the main views after rendering so that if the editor runs in lock step with the engine we don't render a view twice or request update again without rendering being done.
    RunOneFrame();
    plRenderWorld::ClearMainViews();

    m_IPC.SendMessage(&msg);
    return;
  }

  if (const auto* pMsg = plDynamicCast<const plShutdownProcessMsgToEngine*>(e.m_pMessage))
  {
    // in non-remote mode, the process needs to be properly killed, to prevent error messages
    // this is taken care of by the editor process
    if (plEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
      RequestQuit();

    return;
  }

  // Project Messages:
  if (const auto* pMsg = plDynamicCast<const plSetupProjectMsgToEngine*>(e.m_pMessage))
  {
    if (!m_sProjectDirectory.IsEmpty())
    {
      // ignore this message, if it is for the same project
      if (m_sProjectDirectory == pMsg->m_sProjectDir)
        return;

      plLog::Error("Engine Process must restart to switch to another project ('{0}' -> '{1}').", m_sProjectDirectory, pMsg->m_sProjectDir);
      return;
    }

    m_sProjectDirectory = pMsg->m_sProjectDir;
    m_CustomFileSystemConfig = pMsg->m_FileSystemConfig;
    m_CustomPluginConfig = pMsg->m_PluginConfig;

    if (!pMsg->m_sAssetProfile.IsEmpty())
    {
      plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-profile");
      plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(pMsg->m_sAssetProfile);
    }

    if (!pMsg->m_sFileserveAddress.IsEmpty())
    {
      // we have no link dependency on the fileserve plugin here, it might not be loaded (yet / at all)
      // but we can pass the address to the command line, then it will pick it up, if necessary
      plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-fs_server");
      plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(pMsg->m_sFileserveAddress);
    }

    // now that we know which project to initialize, do the delayed project setup
    {
      ExecuteInitFunctions();

      plStartup::StartupHighLevelSystems();

      plRenderContext::GetDefaultInstance()->SetAllowAsyncShaderLoading(true);
    }

    // after the plSetupProjectMsgToEngine was processed, all dynamic plugins should be loaded and we can finally send the reflection
    // information over
    SendReflectionInformation();

    // Project setup, we are now ready to accept document messages.
    SendProjectReadyMessage();
    return;
  }
  else if (const auto* pMsg1 = plDynamicCast<const plReloadResourceMsgToEngine*>(e.m_pMessage))
  {
    PLASMA_PROFILE_SCOPE("ReloadResource");

    const plRTTI* pType = plResourceManager::FindResourceForAssetType(pMsg1->m_sResourceType);
    if (auto hResource = plResourceManager::GetExistingResourceByType(pType, pMsg1->m_sResourceID); hResource.IsValid())
    {
      plResourceManager::ReloadResource(pType, hResource, false);
    }
  }
  else if (const auto* pMsg1 = plDynamicCast<const plSimpleConfigMsgToEngine*>(e.m_pMessage))
  {
    if (pMsg1->m_sWhatToDo == "ChangeActivePlatform")
    {
      plStringBuilder sRedirFile("AssetCache/", pMsg1->m_sPayload, ".plAidlt");

      plDataDirectory::FolderType::s_sRedirectionFile = sRedirFile;

      plFileSystem::ReloadAllExternalDataDirectoryConfigs();

      m_PlatformProfile.m_sName = pMsg1->m_sPayload;
      Init_PlatformProfile_LoadForRuntime();

      plResourceManager::ReloadAllResources(false);
      plRenderWorld::DeleteAllCachedRenderData();
    }
    else if (pMsg1->m_sWhatToDo == "ReloadAssetLUT")
    {
      plFileSystem::ReloadAllExternalDataDirectoryConfigs();
    }
    else if (pMsg1->m_sWhatToDo == "ReloadResources")
    {
      if (pMsg1->m_sPayload == "ReloadAllResources")
      {
        plResourceManager::ReloadAllResources(false);
      }
      plRenderWorld::DeleteAllCachedRenderData();
    }
    else if (pMsg1->m_sWhatToDo == "SaveProfiling")
    {
      plProfilingUtils::SaveProfilingCapture(pMsg1->m_sPayload).IgnoreResult();

      plSaveProfilingResponseToEditor response;
      plStringBuilder sAbsPath;
      if (plFileSystem::ResolvePath(pMsg1->m_sPayload, &sAbsPath, nullptr).Succeeded())
      {
        response.m_sProfilingFile = sAbsPath;
      }
      m_IPC.SendMessage(&response);
    }
    else
      plLog::Warning("Unknown plSimpleConfigMsgToEngine '{0}'", pMsg1->m_sWhatToDo);
  }
  else if (const auto* pMsg2 = plDynamicCast<const plResourceUpdateMsgToEngine*>(e.m_pMessage))
  {
    HandleResourceUpdateMsg(*pMsg2);
  }
  else if (const auto* pMsg2a = plDynamicCast<const plRestoreResourceMsgToEngine*>(e.m_pMessage))
  {
    HandleResourceRestoreMsg(*pMsg2a);
  }
  else if (const auto* pMsg2b = plDynamicCast<const plGlobalSettingsMsgToEngine*>(e.m_pMessage))
  {
    plGizmoRenderer::s_fGizmoScale = pMsg2b->m_fGizmoScale;
  }
  else if (const auto* pMsg3 = plDynamicCast<const plChangeCVarMsgToEngine*>(e.m_pMessage))
  {
    if (plCVar* pCVar = plCVar::FindCVarByName(pMsg3->m_sCVarName))
    {
      if (pCVar->GetType() == plCVarType::Int && pMsg3->m_NewValue.CanConvertTo<plInt32>())
      {
        *static_cast<plCVarInt*>(pCVar) = pMsg3->m_NewValue.ConvertTo<plInt32>();
      }
      else if (pCVar->GetType() == plCVarType::Float && pMsg3->m_NewValue.CanConvertTo<float>())
      {
        *static_cast<plCVarFloat*>(pCVar) = pMsg3->m_NewValue.ConvertTo<float>();
      }
      else if (pCVar->GetType() == plCVarType::Bool && pMsg3->m_NewValue.CanConvertTo<bool>())
      {
        *static_cast<plCVarBool*>(pCVar) = pMsg3->m_NewValue.ConvertTo<bool>();
      }
      else if (pCVar->GetType() == plCVarType::String && pMsg3->m_NewValue.CanConvertTo<plString>())
      {
        *static_cast<plCVarString*>(pCVar) = pMsg3->m_NewValue.ConvertTo<plString>();
      }
      else
      {
        plLog::Warning("plChangeCVarMsgToEngine: New value for CVar '{0}' is incompatible with CVar type", pMsg3->m_sCVarName);
      }
    }
    else
      plLog::Warning("plChangeCVarMsgToEngine: Unknown CVar '{0}'", pMsg3->m_sCVarName);
  }
  else if (const auto* pMsg4 = plDynamicCast<const plConsoleCmdMsgToEngine*>(e.m_pMessage))
  {
    if (m_pConsole->GetCommandInterpreter())
    {
      plCommandInterpreterState s;
      s.m_sInput = pMsg4->m_sCommand;

      plStringBuilder tmp;

      if (pMsg4->m_iType == 1)
      {
        m_pConsole->GetCommandInterpreter()->AutoComplete(s);
        tmp.AppendFormat(";;00||<{}", s.m_sInput);
      }
      else
        m_pConsole->GetCommandInterpreter()->Interpret(s);

      for (auto l : s.m_sOutput)
      {
        tmp.AppendFormat(";;{}||{}", plArgI((int)l.m_Type, 2, true), l.m_sText);
      }

      plConsoleCmdResultMsgToEditor r;
      r.m_sResult = tmp;

      m_IPC.SendMessage(&r);
    }
  }

  // Document Messages:
  if (!e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<plEditorEngineDocumentMsg>())
    return;

  const plEditorEngineDocumentMsg* pDocMsg = (const plEditorEngineDocumentMsg*)e.m_pMessage;

  plEngineProcessDocumentContext* pDocumentContext = plEngineProcessDocumentContext::GetDocumentContext(pDocMsg->m_DocumentGuid);

  if (const auto* pMsg5 = plDynamicCast<const plDocumentOpenMsgToEngine*>(e.m_pMessage)) // Document was opened or closed
  {
    if (pMsg5->m_bDocumentOpen)
    {
      pDocumentContext = CreateDocumentContext(pMsg5);
      PLASMA_ASSERT_DEV(pDocumentContext != nullptr, "Could not create a document context for document type '{0}'", pMsg5->m_sDocumentType);
    }
    else
    {
      plEngineProcessDocumentContext::DestroyDocumentContext(pDocMsg->m_DocumentGuid);
    }

    return;
  }

  if (const auto* pMsg6 = plDynamicCast<const plDocumentClearMsgToEngine*>(e.m_pMessage))
  {
    pDocumentContext = plEngineProcessDocumentContext::GetDocumentContext(pMsg6->m_DocumentGuid);

    if (pDocumentContext)
    {
      pDocumentContext->ClearExistingObjects();
    }
    return;
  }

  // can be null if the asset was deleted on disk manually
  if (pDocumentContext)
  {
    pDocumentContext->HandleMessage(pDocMsg);
  }
}

plEngineProcessDocumentContext* plEngineProcessGameApplication::CreateDocumentContext(const plDocumentOpenMsgToEngine* pMsg)
{
  plDocumentOpenResponseMsgToEditor m;
  m.m_DocumentGuid = pMsg->m_DocumentGuid;
  plEngineProcessDocumentContext* pDocumentContext = plEngineProcessDocumentContext::GetDocumentContext(pMsg->m_DocumentGuid);

  if (pDocumentContext == nullptr)
  {
    plRTTI::ForEachDerivedType<plEngineProcessDocumentContext>(
      [&](const plRTTI* pRtti) {
        auto* pProp = pRtti->FindPropertyByName("DocumentType");
        if (pProp && pProp->GetCategory() == plPropertyCategory::Constant)
        {
          const plStringBuilder sDocTypes(";", static_cast<const plAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<plString>(), ";");
          const plStringBuilder sRequestedType(";", pMsg->m_sDocumentType, ";");

          if (sDocTypes.FindSubString(sRequestedType) != nullptr)
          {
            plLog::Dev("Created Context of type '{0}' for '{1}'", pRtti->GetTypeName(), pMsg->m_sDocumentType);
            for (auto pFunc : pRtti->GetFunctions())
            {
              if (plStringUtils::IsEqual(pFunc->GetPropertyName(), "AllocateContext"))
              {
                plVariant res;
                plHybridArray<plVariant, 1> params;
                params.PushBack(pMsg);
                pFunc->Execute(nullptr, params, res);
                if (res.IsA<plEngineProcessDocumentContext*>())
                {
                  pDocumentContext = res.Get<plEngineProcessDocumentContext*>();
                }
                else
                {
                  plLog::Error("Failed to call custom allocator '{}::{}'.", pRtti->GetTypeName(), pFunc->GetPropertyName());
                }
              }
            }

            if (!pDocumentContext)
            {
              pDocumentContext = pRtti->GetAllocator()->Allocate<plEngineProcessDocumentContext>();
            }

            plEngineProcessDocumentContext::AddDocumentContext(pMsg->m_DocumentGuid, pMsg->m_DocumentMetaData, pDocumentContext, &m_IPC, pMsg->m_sDocumentType);
          }
        }
      });
  }
  else
  {
    pDocumentContext->Reset();
  }

  m_IPC.SendMessage(&m);
  return pDocumentContext;
}

void plEngineProcessGameApplication::Init_LoadProjectPlugins()
{
  m_CustomPluginConfig.m_Plugins.Sort([](const plApplicationPluginConfig::PluginConfig& lhs, const plApplicationPluginConfig::PluginConfig& rhs) -> bool {
    const bool isEnginePluginLhs = lhs.m_sAppDirRelativePath.FindSubString_NoCase("EnginePlugin") != nullptr;
    const bool isEnginePluginRhs = rhs.m_sAppDirRelativePath.FindSubString_NoCase("EnginePlugin") != nullptr;

    if (isEnginePluginLhs != isEnginePluginRhs)
    {
      // make sure the "engine plugins" end up at the back of the list
      // the reason for this is, that the engine plugins often have a link dependency on runtime plugins and pull their reflection data in right away
      // but then the plPlugin system doesn't know that certain reflected types actually come from some runtime plugin
      // by loading the editor engine plugins last, this solves that problem
      return isEnginePluginRhs;
    }

    return lhs.m_sAppDirRelativePath.Compare_NoCase(rhs.m_sAppDirRelativePath) < 0; });

  m_CustomPluginConfig.Apply();
}

plString plEngineProcessGameApplication::FindProjectDirectory() const
{
  return m_sProjectDirectory;
}

void plEngineProcessGameApplication::Init_FileSystem_ConfigureDataDirs()
{
  plStringBuilder sAppDir = ">sdk/Data/Tools/EditorEngineProcess";
  plStringBuilder sUserData = ">user/plEngine Project/EditorEngineProcess";

  // make sure these directories exist
  plFileSystem::CreateDirectoryStructure(sAppDir).IgnoreResult();
  plFileSystem::CreateDirectoryStructure(sUserData).IgnoreResult();

  plFileSystem::AddDataDirectory("", "EngineProcess", ":", plFileSystem::AllowWrites).IgnoreResult();                   // for absolute paths
  plFileSystem::AddDataDirectory(">appdir/", "EngineProcess", "bin", plFileSystem::ReadOnly).IgnoreResult();            // writing to the binary directory
  plFileSystem::AddDataDirectory(">appdir/", "EngineProcess", "shadercache", plFileSystem::AllowWrites).IgnoreResult(); // for shader files
  plFileSystem::AddDataDirectory(sAppDir.GetData(), "EngineProcess", "app").IgnoreResult();                             // app specific data
  plFileSystem::AddDataDirectory(sUserData, "EngineProcess", "appdata", plFileSystem::AllowWrites).IgnoreResult();      // for writing app user data

  m_CustomFileSystemConfig.Apply();

  {
    // We need the file system before we can start the html logger.
    plOsProcessID uiProcessID = plProcess::GetCurrentProcessID();
    plStringBuilder sLogFile;
    sLogFile.Format(":appdata/Log_{0}.htm", uiProcessID);
    m_LogHTML.BeginLog(sLogFile, "EditorEngineProcess");
  }
}

bool plEngineProcessGameApplication::Run_ProcessApplicationInput()
{
  // override the escape action to not shut down the app, but instead close the play-the-game window
  if (plInputManager::GetInputActionState("GameApp", "CloseApp") != plKeyState::Up)
  {
    if (m_pGameState)
    {
      m_pGameState->RequestQuit();
    }
  }
  else
  {
    return SUPER::Run_ProcessApplicationInput();
  }

  return true;
}

plUniquePtr<plEditorEngineProcessApp> plEngineProcessGameApplication::CreateEngineProcessApp()
{
  return PLASMA_DEFAULT_NEW(plEditorEngineProcessApp);
}

void plEngineProcessGameApplication::BaseInit_ConfigureLogging()
{
  SUPER::BaseInit_ConfigureLogging();

  plGlobalLog::AddLogWriter(plMakeDelegate(&plEngineProcessGameApplication::LogWriter, this));
  plGlobalLog::AddLogWriter(plLoggingEvent::Handler(&plLogWriter::HTML::LogMessageHandler, &m_LogHTML));

  plLog::SetCustomPrintFunction(&EditorPrintFunction);

  // used for sending CVar changes over to the editor
  plCVar::s_AllCVarEvents.AddEventHandler(plMakeDelegate(&plEngineProcessGameApplication::EventHandlerCVar, this));
  plPlugin::Events().AddEventHandler(plMakeDelegate(&plEngineProcessGameApplication::EventHandlerCVarPlugin, this));
}

void plEngineProcessGameApplication::Deinit_ShutdownLogging()
{
  plGlobalLog::RemoveLogWriter(plLoggingEvent::Handler(&plLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  m_LogHTML.EndLog();

  plGlobalLog::RemoveLogWriter(plMakeDelegate(&plEngineProcessGameApplication::LogWriter, this));

  // used for sending CVar changes over to the editor
  plCVar::s_AllCVarEvents.RemoveEventHandler(plMakeDelegate(&plEngineProcessGameApplication::EventHandlerCVar, this));
  plPlugin::Events().RemoveEventHandler(plMakeDelegate(&plEngineProcessGameApplication::EventHandlerCVarPlugin, this));

  SUPER::Deinit_ShutdownLogging();
}

void plEngineProcessGameApplication::EventHandlerCVar(const plCVarEvent& e)
{
  if (e.m_EventType == plCVarEvent::ValueChanged)
  {
    TransmitCVar(e.m_pCVar);
  }

  if (e.m_EventType == plCVarEvent::ListOfVarsChanged)
  {
    // currently no way to remove CVars from the editor UI

    for (plCVar* pCVar = plCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
    {
      TransmitCVar(pCVar);
    }
  }
}

void plEngineProcessGameApplication::EventHandlerCVarPlugin(const plPluginEvent& e)
{
  if (e.m_EventType == plPluginEvent::Type::AfterLoadingBeforeInit)
  {
    for (plCVar* pCVar = plCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
    {
      TransmitCVar(pCVar);
    }
  }
}

void plEngineProcessGameApplication::TransmitCVar(const plCVar* pCVar)
{
  plCVarMsgToEditor msg;
  msg.m_sName = pCVar->GetName();
  msg.m_sPlugin = pCVar->GetPluginName();
  msg.m_sDescription = pCVar->GetDescription();

  switch (pCVar->GetType())
  {
    case plCVarType::Int:
      msg.m_Value = ((plCVarInt*)pCVar)->GetValue();
      break;
    case plCVarType::Float:
      msg.m_Value = ((plCVarFloat*)pCVar)->GetValue();
      break;
    case plCVarType::Bool:
      msg.m_Value = ((plCVarBool*)pCVar)->GetValue();
      break;
    case plCVarType::String:
      msg.m_Value = ((plCVarString*)pCVar)->GetValue();
      break;

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED
  }

  m_IPC.SendMessage(&msg);
}

void plEngineProcessGameApplication::HandleResourceUpdateMsg(const plResourceUpdateMsgToEngine& msg)
{
  const plRTTI* pRtti = plResourceManager::FindResourceForAssetType(msg.m_sResourceType);

  if (pRtti == nullptr)
  {
    plLog::Error("Resource Type '{}' is unknown.", msg.m_sResourceType);
    return;
  }

  plTypelessResourceHandle hResource = plResourceManager::GetExistingResourceByType(pRtti, msg.m_sResourceID);

  if (hResource.IsValid())
  {
    plStringBuilder sResourceDesc;
    sResourceDesc.Set(msg.m_sResourceType, "LiveUpdate");

    plUniquePtr<plResourceLoaderFromMemory> loader(PLASMA_DEFAULT_NEW(plResourceLoaderFromMemory));
    loader->m_ModificationTimestamp = plTimestamp::CurrentTimestamp();
    loader->m_sResourceDescription = sResourceDesc;

    plMemoryStreamWriter memoryWriter(&loader->m_CustomData);
    memoryWriter.WriteBytes(msg.m_Data.GetData(), msg.m_Data.GetCount()).IgnoreResult();

    plResourceManager::UpdateResourceWithCustomLoader(hResource, std::move(loader));

    plResourceManager::ForceLoadResourceNow(hResource);
  }
}

void plEngineProcessGameApplication::HandleResourceRestoreMsg(const plRestoreResourceMsgToEngine& msg)
{
  const plRTTI* pRtti = plResourceManager::FindResourceForAssetType(msg.m_sResourceType);

  if (pRtti == nullptr)
  {
    plLog::Error("Resource Type '{}' is unknown.", msg.m_sResourceType);
    return;
  }

  plTypelessResourceHandle hResource = plResourceManager::GetExistingResourceByType(pRtti, msg.m_sResourceID);

  if (hResource.IsValid())
  {
    plResourceManager::RestoreResource(hResource);
  }
}
