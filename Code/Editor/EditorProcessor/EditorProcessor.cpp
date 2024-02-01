#include <EditorProcessor/EditorProcessorPCH.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessorMessages.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

plCommandLineOptionPath opt_OutputDir("_EditorProcessor", "-outputDir", "Output directory", "");
plCommandLineOptionBool opt_SaveProfilingData("_EditorProcessor", "-profiling", "Saves performance profiling information into the output folder.", false);
plCommandLineOptionPath opt_Project("_EditorProcessor", "-project", "Path to the project folder.", "");
plCommandLineOptionBool opt_Resave("_EditorProcessor", "-resave", "If specified, assets will be resaved.", false);
plCommandLineOptionString opt_Transform("_EditorProcessor", "-transform", "If specified, assets will be transformed for the given platform profile.\n\
\n\
Example:\n\
  -transform PC\n\
",
  "");

class plEditorApplication : public plApplication
{
public:
  using SUPER = plApplication;

  plEditorApplication()
    : plApplication("plEditor")
  {
    EnableMemoryLeakReporting(true);
    m_pEditorEngineProcessAppDummy = PL_DEFAULT_NEW(plEditorEngineProcessApp);

    m_pEditorApp = new plQtEditorApp;
  }

  virtual plResult BeforeCoreSystemsStartup() override
  {
    plStartup::AddApplicationTag("tool");
    plStartup::AddApplicationTag("editor");
    plStartup::AddApplicationTag("editorprocessor");

    plQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());

    plString sUserDataFolder = plApplicationServices::GetSingleton()->GetApplicationUserDataFolder();
    plString sOutputFolder = opt_OutputDir.GetOptionValue(plCommandLineOption::LogMode::Never);
    plCrashHandler_WriteMiniDump::g_Instance.SetDumpFilePath(sOutputFolder.IsEmpty() ? sUserDataFolder : sOutputFolder, "EditorProcessor");
    plCrashHandler::SetCrashHandler(&plCrashHandler_WriteMiniDump::g_Instance);

    return PL_SUCCESS;
  }

  virtual void AfterCoreSystemsShutdown() override
  {
    m_pEditorEngineProcessAppDummy = nullptr;

    plQtEditorApp::GetSingleton()->DeInitQt();

    delete m_pEditorApp;
    m_pEditorApp = nullptr;
  }

  void EventHandlerIPC(const plProcessCommunicationChannel::Event& e)
  {
    if (const plProcessAssetMsg* pMsg = plDynamicCast<const plProcessAssetMsg*>(e.m_pMessage))
    {
      if (pMsg->m_sAssetPath.HasExtension("plPrefab") || pMsg->m_sAssetPath.HasExtension("plScene"))
      {
        plQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
      }

      plProcessAssetResponseMsg msg;
      {
        plLogEntryDelegate logger([&msg](plLogEntry& ref_entry) -> void { msg.m_LogEntries.PushBack(std::move(ref_entry)); },
          plLogMsgType::WarningMsg);
        plLogSystemScope logScope(&logger);

        const plUInt32 uiPlatform = plAssetCurator::GetSingleton()->FindAssetProfileByName(pMsg->m_sPlatform);

        if (uiPlatform == plInvalidIndex)
        {
          plLog::Error("Asset platform config '{0}' is unknown", pMsg->m_sPlatform);
        }
        else
        {
          plUInt64 uiAssetHash = 0;
          plUInt64 uiThumbHash = 0;

          // TODO: there is currently no 'nice' way to switch the active platform for the asset processors
          // it is also not clear whether this is actually safe to execute here
          plAssetCurator::GetSingleton()->SetActiveAssetProfileByIndex(uiPlatform);
          // First, force checking for file system changes for the asset and the transitive hull of all dependencies and runtime references. This needs to be done as this EditorProcessor instance might not know all the files yet as some might just have been written. We can't rely on the filesystem watcher as it is not instant and also might just miss some events.
          for (const plString& sDepOrRef : pMsg->m_DepRefHull)
          {
            if (sDepOrRef.IsAbsolutePath())
            {
              plAssetCurator::GetSingleton()->NotifyOfFileChange(sDepOrRef);
            }
            else
            {
              plStringBuilder sTemp = sDepOrRef;
              if (plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTemp))
              {
                plAssetCurator::GetSingleton()->NotifyOfFileChange(sTemp);
              }
            }
          }
          plAssetCurator::GetSingleton()->NotifyOfFileChange(pMsg->m_sAssetPath);

          // Next, we force checking that the asset is up to date. This EditorProcessor instance might not have observed the generation of the output files of various dependencies yet and incorrectly assume that some dependencies still need to be transformed. To prevent this, we force checking the asset and all its dependencies via the filesystem, ignoring the caching.
          plAssetInfo::TransformState state = plAssetCurator::GetSingleton()->IsAssetUpToDate(pMsg->m_AssetGuid, plAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform), nullptr, uiAssetHash, uiThumbHash, true);

          if (uiAssetHash != pMsg->m_AssetHash || uiThumbHash != pMsg->m_ThumbHash)
          {
            plLog::Warning("Asset '{}' of state '{}' in processor with hashes '{}{}' differs from the state in the editor with hashes '{}{}'", pMsg->m_sAssetPath, (int)state, uiAssetHash, uiThumbHash, pMsg->m_AssetHash, pMsg->m_ThumbHash);
          }

          if (state == plAssetInfo::NeedsThumbnail || state == plAssetInfo::NeedsTransform)
          {
            msg.m_Status = plAssetCurator::GetSingleton()->TransformAsset(pMsg->m_AssetGuid, plTransformFlags::BackgroundProcessing, plAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform));

            if (msg.m_Status.Failed())
            {
              // make sure the result message ends up in the log
              plLog::Error("{}", msg.m_Status.m_sMessage);
            }
          }
          else if (state == plAssetInfo::UpToDate)
          {
            msg.m_Status = plTransformStatus();
            plLog::Warning("Asset already up to date: '{}'", pMsg->m_sAssetPath);
          }
          else
          {
            msg.m_Status = plTransformStatus(plFmt("Asset {} is in state {}, can't process asset.", pMsg->m_sAssetPath, (int)state)); // TODO nicer state to string
            plLog::Error("{}", msg.m_Status.m_sMessage);
          }
        }
      }
      m_IPC.SendMessage(&msg);
    }
  }

  virtual Execution Run() override
  {
    {
      plStringBuilder cmdHelp;
      if (plCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, plCommandLineOption::LogAvailableModes::IfHelpRequested, "_EditorProcessor;cvar"))
      {
        plQtUiServices::GetSingleton()->MessageBoxInformation(cmdHelp);
        return plApplication::Execution::Quit;
      }
    }

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
    // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
    // this also speeds up process termination significantly (down to less than a second)
    DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif
    const plString sTransformProfile = opt_Transform.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified);
    const bool bResave = opt_Resave.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified);
    const bool bBackgroundMode = sTransformProfile.IsEmpty() && !bResave;
    const plString sOutputDir = opt_OutputDir.GetOptionValue(plCommandLineOption::LogMode::Always);
    const plBitflags<plQtEditorApp::StartupFlags> startupFlags = bBackgroundMode ? plQtEditorApp::StartupFlags::Headless | plQtEditorApp::StartupFlags::Background : plQtEditorApp::StartupFlags::Headless;
    plQtEditorApp::GetSingleton()->StartupEditor(startupFlags, sOutputDir);
    plQtUiServices::SetHeadless(true);

    const plStringBuilder sProject = opt_Project.GetOptionValue(plCommandLineOption::LogMode::Always);

    if (!sTransformProfile.IsEmpty())
    {
      if (plQtEditorApp::GetSingleton()->OpenProject(sProject).Failed())
      {
        SetReturnCode(2);
        return plApplication::Execution::Quit;
      }

      // before we transform any assets, make sure the C++ code is properly built
      {
        plCppSettings cppSettings;
        if (cppSettings.Load().Succeeded())
        {
          if (plCppProject::BuildCodeIfNecessary(cppSettings).Failed())
          {
            SetReturnCode(3);
            return plApplication::Execution::Quit;
          }

          plQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
        }
      }

      bool bTransform = true;

      plQtEditorApp::GetSingleton()->connect(plQtEditorApp::GetSingleton(), &plQtEditorApp::IdleEvent, plQtEditorApp::GetSingleton(), [this, &bTransform, &sTransformProfile]() {
        if (!bTransform)
          return;

        bTransform = false;

        const plUInt32 uiPlatform = plAssetCurator::GetSingleton()->FindAssetProfileByName(sTransformProfile);

        if (uiPlatform == plInvalidIndex)
        {
          plLog::Error("Asset platform config '{0}' is unknown", sTransformProfile);
        }
        else
        {
          plStatus status = plAssetCurator::GetSingleton()->TransformAllAssets(plTransformFlags::TriggeredManually, plAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform));
          if (status.Failed())
          {
            status.LogFailure();
            SetReturnCode(1);
          }

          if (opt_SaveProfilingData.GetOptionValue(plCommandLineOption::LogMode::Always))
          {
            plActionContext context;
            plActionManager::ExecuteAction("Engine", "Editor.SaveProfiling", context).IgnoreResult();
          }
        }

        QApplication::quit(); });

      const plInt32 iReturnCode = plQtEditorApp::GetSingleton()->RunEditor();
      if (iReturnCode != 0)
        SetReturnCode(iReturnCode);
    }
    else if (opt_Resave.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified))
    {
      plQtEditorApp::GetSingleton()->OpenProject(sProject).IgnoreResult();

      plQtEditorApp::GetSingleton()->connect(plQtEditorApp::GetSingleton(), &plQtEditorApp::IdleEvent, plQtEditorApp::GetSingleton(), [this]() {
        plAssetCurator::GetSingleton()->ResaveAllAssets();

        if (opt_SaveProfilingData.GetOptionValue(plCommandLineOption::LogMode::Always))
        {
          plActionContext context;
          plActionManager::ExecuteAction("Engine", "Editor.SaveProfiling", context).IgnoreResult();
          }

        QApplication::quit(); });

      const plInt32 iReturnCode = plQtEditorApp::GetSingleton()->RunEditor();
      if (iReturnCode != 0)
        SetReturnCode(iReturnCode);
    }
    else
    {
      plResult res = m_IPC.ConnectToHostProcess();
      if (res.Succeeded())
      {
        m_IPC.m_Events.AddEventHandler(plMakeDelegate(&plEditorApplication::EventHandlerIPC, this));

        plQtEditorApp::GetSingleton()->OpenProject(sProject).IgnoreResult();
        plQtEditorApp::GetSingleton()->connect(plQtEditorApp::GetSingleton(), &plQtEditorApp::IdleEvent, plQtEditorApp::GetSingleton(), [this]() {
          static bool bRecursionBlock = false;
          if (bRecursionBlock)
            return;
          bRecursionBlock = true;

          if (!m_IPC.IsHostAlive())
            QApplication::quit();

          m_IPC.WaitForMessages();

          bRecursionBlock = false; });

        const plInt32 iReturnCode = plQtEditorApp::GetSingleton()->RunEditor();
        SetReturnCode(iReturnCode);
      }
      else
      {
        plLog::Error("Failed to connect with host process");
      }
    }

    plQtEditorApp::GetSingleton()->ShutdownEditor();

    return plApplication::Execution::Quit;
  }

private:
  plQtEditorApp* m_pEditorApp;
  plEngineProcessCommunicationChannel m_IPC;
  plUniquePtr<plEditorEngineProcessApp> m_pEditorEngineProcessAppDummy;
};

PL_APPLICATION_ENTRY_POINT(plEditorApplication);
