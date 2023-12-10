#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/System/Window.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GuiFoundation/Dialogs/ModifiedDocumentsDlg.moc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

void UpdateInputDynamicEnumValues();

void plQtEditorApp::CloseProject()
{
  QMetaObject::invokeMethod(this, "SlotQueuedCloseProject", Qt::ConnectionType::QueuedConnection);
}

void plQtEditorApp::SlotQueuedCloseProject()
{
  // purge the image loading queue when a project is closed, but keep the existing cache
  plQtImageCache::GetSingleton()->StopRequestProcessing(false);

  plToolsProject::CloseProject();

  // enable image loading again, the queue is purged now
  plQtImageCache::GetSingleton()->EnableRequestProcessing();
}

plResult plQtEditorApp::OpenProject(const char* szProject, bool bImmediate /*= false*/)
{
  if (bImmediate)
  {
    return CreateOrOpenProject(false, szProject);
  }
  else
  {
    QMetaObject::invokeMethod(this, "SlotQueuedOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, szProject));
    return PLASMA_SUCCESS;
  }
}

void plQtEditorApp::SlotQueuedOpenProject(QString sProject)
{
  CreateOrOpenProject(false, sProject.toUtf8().data()).IgnoreResult();
}

plResult plQtEditorApp::CreateOrOpenProject(bool bCreate, plStringView sFile0)
{
  plStringBuilder sFile = sFile0;
  if (!bCreate)
  {
    const plStatus status = MakeRemoteProjectLocal(sFile);
    if (status.Failed())
    {
      // if the message is empty, the user decided not to continue, so don't show an error message in this case
      if (!status.m_sMessage.IsEmpty())
      {
        plQtUiServices::GetSingleton()->MessageBoxStatus(status, "Opening remote project failed.");
      }

      return PLASMA_FAILURE;
    }
  }

  // check that we don't attempt to open a project from a different repository, due to code changes this often doesn't work too well
  if (!IsInHeadlessMode())
  {
    plStringBuilder sdkDirFromProject;
    if (plFileSystem::FindFolderWithSubPath(sdkDirFromProject, sFile, "Data/Base", "plSdkRoot.txt").Succeeded())
    {
      sdkDirFromProject.MakeCleanPath();
      sdkDirFromProject.Trim(nullptr, "/");

      plStringView sdkDir = plFileSystem::GetSdkRootDirectory();
      sdkDir.Trim(nullptr, "/");

      if (sdkDirFromProject != sdkDir)
      {
        if (plQtUiServices::MessageBoxQuestion(plFmt("You are attempting to open a project that's located in a different SDK directory.\n\nSDK location: '{}'\nProject path: '{}'\n\nThis may make problems.\n\nContinue anyway?", sdkDir, sFile), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
        {
          return PLASMA_FAILURE;
        }
      }
    }
  }

  PLASMA_PROFILE_SCOPE("CreateOrOpenProject");
  m_bLoadingProjectInProgress = true;
  PLASMA_SCOPE_EXIT(m_bLoadingProjectInProgress = false;);

  CloseSplashScreen();

  plStringBuilder sProjectFile = sFile;
  sProjectFile.MakeCleanPath();

  if (bCreate == false && !sProjectFile.EndsWith_NoCase("/plProject"))
  {
    sProjectFile.AppendPath("plProject");
  }

  if (plToolsProject::IsProjectOpen() && plToolsProject::GetSingleton()->GetProjectFile() == sProjectFile)
  {
    plQtUiServices::MessageBoxInformation("The selected project is already open");
    return PLASMA_FAILURE;
  }

  if (!plToolsProject::CanCloseProject())
    return PLASMA_FAILURE;

  plToolsProject::CloseProject();

  // create default plugin selection
  if (!ExistsPluginSelectionStateDDL(sProjectFile))
    CreatePluginSelectionDDL(sProjectFile, "General3D");

  plStatus res;
  if (bCreate)
  {
    if (m_bAnyProjectOpened)
    {
      // if we opened any project before, spawn a new editor instance and open the project there
      // this way, a different set of editor plugins can be loaded
      LaunchEditor(sProjectFile, true);

      QApplication::closeAllWindows();
      return PLASMA_SUCCESS;
    }
    else
    {
      // once we start loading any plugins, we can't reuse the same instance again for another project
      m_bAnyProjectOpened = true;

      LoadPluginBundleDlls(sProjectFile);

      res = plToolsProject::CreateProject(sProjectFile);
    }
  }
  else
  {
    if (m_bAnyProjectOpened)
    {
      // if we opened any project before, spawn a new editor instance and open the project there
      // this way, a different set of editor plugins can be loaded
      LaunchEditor(sProjectFile, false);

      QApplication::closeAllWindows();
      return PLASMA_SUCCESS;
    }
    else
    {
      // once we start loading any plugins, we can't reuse the same instance again for another project
      m_bAnyProjectOpened = true;

      plStringBuilder sTemp = plOSFile::GetTempDataFolder("plEditor");
      sTemp.AppendPath("plEditorCrashIndicator");
      plOSFile f;
      if (f.Open(sTemp, plFileOpenMode::Write, plFileShareMode::Exclusive).Succeeded())
      {
        f.Write(sTemp.GetData(), sTemp.GetElementCount()).IgnoreResult();
        f.Close();
        m_bWroteCrashIndicatorFile = true;
      }

      {
        plStringBuilder sProjectDir = sProjectFile;
        sProjectDir.PathParentDirectory();

        plStringBuilder sSettingsFile = sProjectDir;
        sSettingsFile.AppendPath("Editor/CppProject.ddl");

        // first attempt to load project specific plugin bundles
        plCppSettings cppSettings;
        if (cppSettings.Load(sSettingsFile).Succeeded())
        {
          plQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(plCppProject::GetPluginSourceDir(cppSettings, sProjectDir));
        }

        // now load the plugin DLLs
        LoadPluginBundleDlls(sProjectFile);
      }

      res = plToolsProject::OpenProject(sProjectFile);
    }
  }

  if (res.m_Result.Failed())
  {
    plStringBuilder s;
    s.Format("Failed to open project:\n'{0}'", sProjectFile);

    plQtUiServices::MessageBoxStatus(res, s);
    return PLASMA_FAILURE;
  }


  if (m_StartupFlags.AreNoneSet(StartupFlags::SafeMode | StartupFlags::Headless))
  {
    plStringBuilder sAbsPath;

    if (!m_DocumentsToOpen.IsEmpty())
    {
      for (const auto& doc : m_DocumentsToOpen)
      {
        sAbsPath = doc;

        if (MakeDataDirectoryRelativePathAbsolute(sAbsPath))
        {
          SlotQueuedOpenDocument(sAbsPath.GetData(), nullptr);
        }
        else
        {
          plLog::Error("Document '{}' does not exist in this project.", doc);
        }
      }

      // don't try to open the same documents when the user switches to another project
      m_DocumentsToOpen.Clear();
    }
    else if (!m_StartupFlags.IsSet(StartupFlags::NoRecent))
    {
      const plRecentFilesList allDocs = LoadOpenDocumentsList();

      // Unfortunately this crashes in Qt due to the processEvents in the QtProgressBar
      // plProgressRange range("Restoring Documents", allDocs.GetFileList().GetCount(), true);

      for (auto& doc : allDocs.GetFileList())
      {
        // if (range.WasCanceled())
        //    break;

        // range.BeginNextStep(doc.m_File);
        SlotQueuedOpenDocument(doc.m_File.GetData(), nullptr);
      }
    }

    if (!plQtEditorApp::GetSingleton()->IsInSafeMode())
    {
      plQtContainerWindow::GetContainerWindow()->ScheduleRestoreWindowLayout();
    }
  }
  return PLASMA_SUCCESS;
}

void plQtEditorApp::ProjectEventHandler(const plToolsProjectEvent& r)
{
  switch (r.m_Type)
  {
    case plToolsProjectEvent::Type::ProjectCreated:
      SetupNewProject();
      m_bSavePreferencesAfterOpenProject = true;
      break;

    case plToolsProjectEvent::Type::ProjectOpened:
    {
      PLASMA_PROFILE_SCOPE("ProjectOpened");
      plDynamicStringEnum::s_RequestUnknownCallback = plMakeDelegate(&plQtEditorApp::OnDemandDynamicStringEnumLoad, this);
      LoadProjectPreferences();
      SetupDataDirectories();
      ReadTagRegistry();
      UpdateInputDynamicEnumValues();

      // add project specific translations
      // (these are currently never removed)
      {
        m_pTranslatorFromFiles->AddTranslationFilesFromFolder(":project/Editor/Localization/en");
      }

      // tell the engine process which file system and plugin configuration to use
      plEditorEngineProcessConnection::GetSingleton()->SetFileSystemConfig(m_FileSystemConfig);
      plEditorEngineProcessConnection::GetSingleton()->SetPluginConfig(GetRuntimePluginConfig(true));

      plAssetCurator::GetSingleton()->StartInitialize(m_FileSystemConfig);
      if (plEditorEngineProcessConnection::GetSingleton()->RestartProcess().Failed())
      {
        PLASMA_PROFILE_SCOPE("ErrorLog");
        plLog::Error("Failed to start the engine process. Project loading incomplete.");
      }
      plAssetCurator::GetSingleton()->WaitForInitialize();

      m_sLastDocumentFolder = plToolsProject::GetSingleton()->GetProjectFile();
      m_sLastProjectFolder = plToolsProject::GetSingleton()->GetProjectFile();

      m_RecentProjects.Insert(plToolsProject::GetSingleton()->GetProjectFile(), 0);

      plEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<plEditorPreferencesUser>();

      // Make sure preferences are saved, this is important when the project was just created.
      if (m_bSavePreferencesAfterOpenProject)
      {
        m_bSavePreferencesAfterOpenProject = false;
        SaveSettings();
      }
      else
      {
        // Save recent project list on project open in case of crashes or stopping the debugger.
        SaveRecentFiles();
      }

      if (m_StartupFlags.AreNoneSet(plQtEditorApp::StartupFlags::Headless | plQtEditorApp::StartupFlags::SafeMode | plQtEditorApp::StartupFlags::UnitTest | plQtEditorApp::StartupFlags::Background))
      {
        if (plCppProject::IsBuildRequired())
        {
          const auto clicked = plQtUiServices::MessageBoxQuestion("<html>Compile this project's C++ plugin?<br><br>\
Explanation: This project has <a href='https://plengine.net/pages/docs/custom-code/cpp/cpp-project-generation.html'>a dedicated C++ plugin</a> with custom code. The plugin is currently not compiled and therefore the project won't fully work and certain assets will fail to transform.<br><br>\
It is advised to compile the plugin now, but you can also do so later.</html>",
            QMessageBox::StandardButton::Apply | QMessageBox::StandardButton::Ignore, QMessageBox::StandardButton::Apply);

          if (clicked == QMessageBox::StandardButton::Ignore)
            break;

          QTimer::singleShot(1000, this, [this]() { plCppProject::EnsureCppPluginReady().IgnoreResult(); });
        }

        plTimestamp lastTransform = plAssetCurator::GetSingleton()->GetLastFullTransformDate().GetTimestamp();

        if (pPreferences->m_bBackgroundAssetProcessing)
        {
          QTimer::singleShot(2000, this, [this]() { plAssetProcessor::GetSingleton()->StartProcessTask(); });
        }
        else if (!lastTransform.IsValid() || (plTimestamp::CurrentTimestamp() - lastTransform).GetHours() > 5 * 24)
        {
          const auto clicked = plQtUiServices::MessageBoxQuestion("<html>Apply asset transformation now?<br><br>\
Explanation: For assets to work properly, they must be <a href='https://plengine.net/pages/docs/assets/assets-overview.html#asset-transform'>transformed</a>. Otherwise they don't function as they should or don't even show up.<br>You can manually run the asset transform from the <a href='https://plengine.net/pages/docs/assets/asset-browser.html#transform-assets'>asset browser</a> at any time.</html>",
            QMessageBox::StandardButton::Apply | QMessageBox::StandardButton::Ignore, QMessageBox::StandardButton::Apply);

          if (clicked == QMessageBox::StandardButton::Ignore)
          {
            plAssetCurator::GetSingleton()->StoreFullTransformDate();
            break;
          }

          // check whether the project needs to be transformed
          QTimer::singleShot(2000, this, [this]() { plAssetCurator::GetSingleton()->TransformAllAssets(plTransformFlags::Default).IgnoreResult(); });
        }
      }

      break;
    }

    case plToolsProjectEvent::Type::ProjectSaveState:
    {
      m_RecentProjects.Insert(plToolsProject::GetSingleton()->GetProjectFile(), 0);
      SaveSettings();
      break;
    }

    case plToolsProjectEvent::Type::ProjectClosing:
    {
      plShutdownProcessMsgToEngine msg;
      plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
      break;
    }

    case plToolsProjectEvent::Type::ProjectClosed:
    {
      plEditorEngineProcessConnection::GetSingleton()->ShutdownProcess();

      plAssetCurator::GetSingleton()->Deinitialize();

      // remove all data directories that were loaded by the project configuration
      plApplicationFileSystemConfig::Clear();
      plFileSystem::SetSpecialDirectory("project", nullptr); // removes this directory

      m_ReloadProjectRequiredReasons.Clear();
      UpdateGlobalStatusBarMessage();

      plPreferences::ClearProjectPreferences();

      // remove all dynamic enums that were dynamically loaded from the project directory
      {
        for (const auto& val : m_DynamicEnumStringsToClear)
        {
          plDynamicStringEnum::RemoveEnum(val);
        }
        m_DynamicEnumStringsToClear.Clear();
      }

      break;
    }

    case plToolsProjectEvent::Type::SaveAll:
    {
      plToolsProject::SaveProjectState();
      SaveAllOpenDocuments();
      break;
    }

    default:
      break;
  }
}

void plQtEditorApp::ProjectRequestHandler(plToolsProjectRequest& r)
{
  switch (r.m_Type)
  {
    case plToolsProjectRequest::Type::CanCloseProject:
    case plToolsProjectRequest::Type::CanCloseDocuments:
    {
      if (r.m_bCanClose == false)
        return;

      plHybridArray<plDocument*, 32> ModifiedDocs;
      if (r.m_Type == plToolsProjectRequest::Type::CanCloseProject)
      {
        for (plDocumentManager* pMan : plDocumentManager::GetAllDocumentManagers())
        {
          for (plDocument* pDoc : pMan->GetAllOpenDocuments())
          {
            if (pDoc->IsModified())
              ModifiedDocs.PushBack(pDoc);
          }
        }
      }
      else
      {
        for (plDocument* pDoc : r.m_Documents)
        {
          if (pDoc->IsModified())
            ModifiedDocs.PushBack(pDoc);
        }
      }

      if (!ModifiedDocs.IsEmpty())
      {
        plQtModifiedDocumentsDlg dlg(QApplication::activeWindow(), ModifiedDocs);
        if (dlg.exec() == 0)
          r.m_bCanClose = false;
      }
    }
    break;
    case plToolsProjectRequest::Type::SuggestContainerWindow:
    {
      const auto& docs = GetRecentDocumentsList();
      plStringBuilder sCleanPath = r.m_Documents[0]->GetDocumentPath();
      sCleanPath.MakeCleanPath();

      for (auto& file : docs.GetFileList())
      {
        if (file.m_File == sCleanPath)
        {
          r.m_iContainerWindowUniqueIdentifier = file.m_iContainerWindow;
          break;
        }
      }
    }
    break;
    case plToolsProjectRequest::Type::GetPathForDocumentGuid:
    {
      if (plAssetCurator::plLockedSubAsset pSubAsset = plAssetCurator::GetSingleton()->GetSubAsset(r.m_documentGuid))
      {
        r.m_sAbsDocumentPath = pSubAsset->m_pAssetInfo->m_Path;
      }
    }
    break;
  }
}

void plQtEditorApp::SetupNewProject()
{
  plToolsProject::GetSingleton()->CreateSubFolder("Editor");
  plToolsProject::GetSingleton()->CreateSubFolder("RuntimeConfigs");
  plToolsProject::GetSingleton()->CreateSubFolder("Scenes");
  plToolsProject::GetSingleton()->CreateSubFolder("Prefabs");

  // write the default window config
  {
    plStringBuilder sPath = plToolsProject::GetSingleton()->GetProjectDirectory();
    sPath.AppendPath("RuntimeConfigs/Window.ddl");

    plWindowCreationDesc desc;
    desc.m_Title = plToolsProject::GetSingleton()->GetProjectName(false);
    desc.SaveToDDL(sPath).IgnoreResult();
  }

  // write a stub input mapping
  {
    plStringBuilder sPath = plToolsProject::GetSingleton()->GetProjectDirectory();
    sPath.AppendPath("RuntimeConfigs/InputConfig.ddl");

    plDeferredFileWriter file;
    file.SetOutput(sPath);

    plHybridArray<plGameAppInputConfig, 4> actions;
    plGameAppInputConfig& a = actions.ExpandAndGetRef();
    a.m_sInputSet = "Default";
    a.m_sInputAction = "Interact";
    a.m_bApplyTimeScaling = false;
    a.m_sInputSlotTrigger[0] = plInputSlot_KeySpace;
    a.m_sInputSlotTrigger[1] = plInputSlot_MouseButton0;
    a.m_sInputSlotTrigger[2] = plInputSlot_Controller0_ButtonA;

    plGameAppInputConfig::WriteToDDL(file, actions);

    file.Close().IgnoreResult();
  }
}
