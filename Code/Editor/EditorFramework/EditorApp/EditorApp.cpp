#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/CheckVersion.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <QFileDialog>
#include <ToolsFoundation/Application/ApplicationServices.h>

PLASMA_IMPLEMENT_SINGLETON(plQtEditorApp);

plEvent<const plEditorAppEvent&> plQtEditorApp::m_Events;

plQtEditorApp::plQtEditorApp()
  : m_SingletonRegistrar(this)
  , m_RecentProjects(20)
  , m_RecentDocuments(100)
{
  m_bSavePreferencesAfterOpenProject = false;
  m_pVersionChecker = PLASMA_DEFAULT_NEW(plQtVersionChecker);

  m_pTimer = new QTimer(nullptr);
}

plQtEditorApp::~plQtEditorApp()
{
  delete m_pTimer;
  m_pTimer = nullptr;

  CloseSplashScreen();
}

plInt32 plQtEditorApp::RunEditor()
{
  plInt32 ret = m_pQtApplication->exec();
  return ret;
}

void plQtEditorApp::SlotTimedUpdate()
{
  if (plToolsProject::IsProjectOpen())
  {
    if (plEditorEngineProcessConnection::GetSingleton())
      plEditorEngineProcessConnection::GetSingleton()->Update();

    plAssetCurator::GetSingleton()->MainThreadTick(true);
  }
  plTaskSystem::FinishFrameTasks();

  // Close the splash screen when we get to the first idle event
  CloseSplashScreen();

  Q_EMIT IdleEvent();

  RestartEngineProcessIfPluginsChanged(false);

  m_pTimer->start(1);
}

void plQtEditorApp::SlotSaveSettings()
{
  SaveSettings();
}

void plQtEditorApp::SlotVersionCheckCompleted(bool bNewVersionReleased, bool bForced)
{
  // Close the splash screen so it doesn't become the parent window of our message boxes.
  CloseSplashScreen();

  if (bForced || bNewVersionReleased)
  {
    if (m_pVersionChecker->IsLatestNewer())
    {
      plQtUiServices::GetSingleton()->MessageBoxInformation(
        plFmt("<html>A new version is available: {}<br><br>Your version is: {}<br><br>Please check the <A "
              "href=\"https://github.com/plEngine/plEngine/releases\">Releases</A> for details.</html>",
          m_pVersionChecker->GetKnownLatestVersion(), m_pVersionChecker->GetOwnVersion()));
    }
    else
    {
      plStringBuilder tmp("You have the latest version: \n");
      tmp.Append(m_pVersionChecker->GetOwnVersion());

      plQtUiServices::GetSingleton()->MessageBoxInformation(tmp);
    }
  }

  if (m_pVersionChecker->IsLatestNewer())
  {
    plQtUiServices::GetSingleton()->ShowGlobalStatusBarMessage(
      plFmt("New version '{}' available, please update.", m_pVersionChecker->GetKnownLatestVersion()));
  }
}

void plQtEditorApp::EngineProcessMsgHandler(const plEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case plEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (auto pTypeMsg = plDynamicCast<const plUpdateReflectionTypeMsgToEditor*>(e.m_pMsg))
      {
        plPhantomRttiManager::RegisterType(pTypeMsg->m_desc);
      }
      if (auto pDynEnumMsg = plDynamicCast<const plDynamicStringEnumMsgToEditor*>(e.m_pMsg))
      {
        auto& dynEnum = plDynamicStringEnum::CreateDynamicEnum(pDynEnumMsg->m_sEnumName);
        for (auto& sEnumValue : pDynEnumMsg->m_EnumValues)
        {
          dynEnum.AddValidValue(sEnumValue);
        }
        dynEnum.SortValues();
      }
      else if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<plProjectReadyMsgToEditor>())
      {
        // This message is waited upon (blocking) but does not contain any data.
      }
    }
    break;

    case plEditorEngineProcessConnection::Event::Type::ProcessRestarted:
      StoreEnginePluginModificationTimes();
      break;

    default:
      return;
  }
}

void plQtEditorApp::UiServicesEvents(const plQtUiServices::Event& e)
{
  if (e.m_Type == plQtUiServices::Event::Type::CheckForUpdates)
  {
    m_pVersionChecker->Check(true);
  }
}

void plQtEditorApp::SaveAllOpenDocuments()
{
  for (auto pMan : plDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : pMan->plDocumentManager::GetAllOpenDocuments())
    {
      plQtDocumentWindow* pWnd = plQtDocumentWindow::FindWindowByDocument(pDoc);
      // Layers for example will share a window with the scene document and the window will always save the scene.
      if (pWnd && pWnd->GetDocument() == pDoc)
      {
        if (pWnd->SaveDocument().m_Result.Failed())
          return;
      }
      // There might be no window for this document.
      else
      {
        pDoc->SaveDocument().LogFailure();
      }
    }
  }
}

bool plQtEditorApp::IsProgressBarProcessingEvents() const
{
  return m_pQtProgressbar != nullptr && m_pQtProgressbar->IsProcessingEvents();
}

void plQtEditorApp::OnDemandDynamicStringEnumLoad(plStringView sEnumName, plDynamicStringEnum& e)
{
  plStringBuilder sFile;
  sFile.Format(":project/Editor/{}.txt", sEnumName);

  // enums loaded this way are user editable
  e.SetStorageFile(sFile);
  e.ReadFromStorage();

  m_DynamicEnumStringsToClear.Insert(sEnumName);
}

bool ContainsPlugin(const plDynamicArray<plApplicationPluginConfig::PluginConfig>& all, const char* szPlugin)
{
  for (const plApplicationPluginConfig::PluginConfig& one : all)
  {
    if (one.m_sAppDirRelativePath == szPlugin)
      return true;
  }

  return false;
}

plResult plQtEditorApp::AddBundlesInOrder(plDynamicArray<plApplicationPluginConfig::PluginConfig>& order, const plPluginBundleSet& bundles, const plString& start, bool bEditor, bool bEditorEngine, bool bRuntime) const
{
  const plPluginBundle& bundle = bundles.m_Plugins.Find(start).Value();

  for (const plString& req : bundle.m_RequiredBundles)
  {
    auto it = bundles.m_Plugins.Find(req);

    if (!it.IsValid())
    {
      plLog::Error("Plugin bundle '{}' has a dependency on bundle '{}' which does not exist.", start, req);
      return PLASMA_FAILURE;
    }

    PLASMA_SUCCEED_OR_RETURN(AddBundlesInOrder(order, bundles, req, bEditor, bEditorEngine, bRuntime));
  }

  if (bRuntime)
  {
    for (const plString& dll : bundle.m_RuntimePlugins)
    {
      if (!ContainsPlugin(order, dll))
      {
        plApplicationPluginConfig::PluginConfig& p = order.ExpandAndGetRef();
        p.m_sAppDirRelativePath = dll;
        p.m_bLoadCopy = bundle.m_bLoadCopy;
      }
    }
  }

  if (bEditorEngine)
  {
    for (const plString& dll : bundle.m_EditorEnginePlugins)
    {
      if (!ContainsPlugin(order, dll))
      {
        plApplicationPluginConfig::PluginConfig& p = order.ExpandAndGetRef();
        p.m_sAppDirRelativePath = dll;
        p.m_bLoadCopy = bundle.m_bLoadCopy;
      }
    }
  }

  if (bEditor)
  {
    for (const plString& dll : bundle.m_EditorPlugins)
    {
      if (!ContainsPlugin(order, dll))
      {
        plApplicationPluginConfig::PluginConfig& p = order.ExpandAndGetRef();
        p.m_sAppDirRelativePath = dll;
        p.m_bLoadCopy = bundle.m_bLoadCopy;
      }
    }
  }

  return PLASMA_SUCCESS;
}

plStatus plQtEditorApp::MakeRemoteProjectLocal(plStringBuilder& inout_sFilePath)
{
  // already a local project?
  if (inout_sFilePath.EndsWith_NoCase("plProject"))
    return plStatus(PLASMA_SUCCESS);

  {
    plStringBuilder tmp = inout_sFilePath;
    tmp.AppendPath("plProject");

    if (plOSFile::ExistsFile(tmp))
    {
      inout_sFilePath = tmp;
      return plStatus(PLASMA_SUCCESS);
    }
  }

  PLASMA_LOG_BLOCK("Open Remote Project", inout_sFilePath.GetData());

  plStringBuilder sRedirFile = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder(inout_sFilePath);
  sRedirFile.AppendPath("LocalCheckout.txt");

  // read redirection file, if available
  {
    plOSFile file;
    if (file.Open(sRedirFile, plFileOpenMode::Read).Succeeded())
    {
      plDataBuffer content;
      file.ReadAll(content);

      const plStringView sContent((const char*)content.GetData(), content.GetCount());

      if (sContent.EndsWith_NoCase("plProject") && plOSFile::ExistsFile(sContent))
      {
        inout_sFilePath = sContent;
        return plStatus(PLASMA_SUCCESS);
      }
    }
  }

  plString sName;
  plString sType;
  plString sUrl;
  plString sProjectFile;

  // read the info about the remote project from the OpenDDL config file
  {
    plOSFile file;
    if (file.Open(inout_sFilePath, plFileOpenMode::Read).Failed())
    {
      return plStatus(plFmt("Remote project file '{}' doesn't exist.", inout_sFilePath));
    }

    plDataBuffer content;
    file.ReadAll(content);

    plMemoryStreamContainerWrapperStorage<plDataBuffer> storage(&content);
    plMemoryStreamReader reader(&storage);

    plOpenDdlReader ddl;
    if (ddl.ParseDocument(reader).Failed())
    {
      return plStatus("Error in remote project DDL config file");
    }

    if (auto pRoot = ddl.GetRootElement())
    {
      if (auto pProject = pRoot->FindChildOfType("RemoteProject"))
      {
        if (auto pName = pProject->FindChildOfType(plOpenDdlPrimitiveType::String, "Name"))
        {
          sName = pName->GetPrimitivesString()[0];
        }
        if (auto pType = pProject->FindChildOfType(plOpenDdlPrimitiveType::String, "Type"))
        {
          sType = pType->GetPrimitivesString()[0];
        }
        if (auto pUrl = pProject->FindChildOfType(plOpenDdlPrimitiveType::String, "Url"))
        {
          sUrl = pUrl->GetPrimitivesString()[0];
        }
        if (auto pProjectFile = pProject->FindChildOfType(plOpenDdlPrimitiveType::String, "ProjectFile"))
        {
          sProjectFile = pProjectFile->GetPrimitivesString()[0];
        }
      }
    }
  }

  if (sType.IsEmpty() || sName.IsEmpty())
  {
    return plStatus(plFmt("Remote project '{}' DDL configuration is invalid.", inout_sFilePath));
  }

  plQtUiServices::GetSingleton()->MessageBoxInformation("This is a 'remote' project, meaning the data is not yet available on your machine.\n\nPlease select a folder where the project should be downloaded to.");

  static QString sPreviousFolder = plOSFile::GetUserDocumentsFolder().GetData();

  QString sSelectedDir = QFileDialog::getExistingDirectory(QApplication::activeWindow(), QLatin1String("Choose Folder"), sPreviousFolder, QFileDialog::Option::ShowDirsOnly | QFileDialog::Option::DontResolveSymlinks);

  if (sSelectedDir.isEmpty())
  {
    return plStatus("");
  }

  sPreviousFolder = sSelectedDir;
  plStringBuilder sTargetDir = sSelectedDir.toUtf8().data();

  // if it is a git repository, clone it
  if (sType == "git" && !sUrl.IsEmpty())
  {
    QStringList args;
    args << "clone";
    args << plMakeQString(sUrl);
    args << plMakeQString(sName);

    QProcess proc;
    proc.setWorkingDirectory(sTargetDir.GetData());
    proc.start("git.exe", args);

    if (!proc.waitForStarted())
    {
      return plStatus(plFmt("Running 'git' to download the remote project failed."));
    }

    proc.waitForFinished(60 * 1000);

    if (proc.exitStatus() != QProcess::ExitStatus::NormalExit)
    {
      return plStatus(plFmt("Failed to git clone the remote project '{}' from '{}'", sName, sUrl));
    }

    plLog::Success("Cloned remote project '{}' from '{}' to '{}'", sName, sUrl, sTargetDir);

    inout_sFilePath.Format("{}/{}/{}", sTargetDir, sName, sProjectFile);

    // write redirection file
    {
      plOSFile file;
      if (file.Open(sRedirFile, plFileOpenMode::Write).Succeeded())
      {
        file.Write(inout_sFilePath.GetData(), inout_sFilePath.GetElementCount()).AssertSuccess();
      }
    }

    return plStatus(PLASMA_SUCCESS);
  }

  return plStatus(plFmt("Unknown remote project type '{}' or invalid URL '{}'", sType, sUrl));
}

bool plQtEditorApp::ExistsPluginSelectionStateDDL(const char* szProjectDir /*= ":project"*/)
{
  plStringBuilder path = szProjectDir;
  path.MakeCleanPath();

  if (path.EndsWith_NoCase("/plProject"))
    path.PathParentDirectory();

  path.AppendPath("Editor/PluginSelection.ddl");

  return plOSFile::ExistsFile(path);
}

void plQtEditorApp::WritePluginSelectionStateDDL(const char* szProjectDir /*= ":project"*/)
{
  if (m_StartupFlags.IsAnySet(StartupFlags::Background | StartupFlags::Headless | StartupFlags::UnitTest))
    return;

  plStringBuilder path = szProjectDir;
  path.AppendPath("Editor/PluginSelection.ddl");

  plFileWriter file;
  file.Open(path).AssertSuccess();

  plOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  m_PluginBundles.WriteStateToDDL(ddl);
}

void plQtEditorApp::CreatePluginSelectionDDL(const char* szProjectFile, const char* szTemplate)
{
  if (m_StartupFlags.IsAnySet(StartupFlags::Background | StartupFlags::Headless | StartupFlags::UnitTest))
    return;

  plStringBuilder sPath = szProjectFile;
  sPath.PathParentDirectory();

  for (auto it : m_PluginBundles.m_Plugins)
  {
    plPluginBundle& bundle = it.Value();

    bundle.m_bSelected = bundle.m_EnabledInTemplates.Contains(szTemplate);
  }

  WritePluginSelectionStateDDL(sPath);
}

void plQtEditorApp::LoadPluginBundleDlls(const char* szProjectFile)
{
  plStringBuilder sPath = szProjectFile;
  sPath.PathParentDirectory();
  sPath.AppendPath("Editor/PluginSelection.ddl");

  plFileReader file;
  if (file.Open(sPath).Succeeded())
  {
    plOpenDdlReader ddl;
    if (ddl.ParseDocument(file).Failed())
    {
      plLog::Error("Syntax error in plugin bundle file '{}'", sPath);
    }
    else
    {
      auto pState = ddl.GetRootElement()->FindChildOfType("PluginState");
      while (pState)
      {
        if (auto pName = pState->FindChildOfType(plOpenDdlPrimitiveType::String, "ID"))
        {
          const plString sID = pName->GetPrimitivesString()[0];

          bool bExisted = false;
          auto itPlug = m_PluginBundles.m_Plugins.FindOrAdd(sID, &bExisted);

          if (!bExisted)
          {
            plPluginBundle& bundle = itPlug.Value();
            bundle.m_bMissing = true;
            bundle.m_sDisplayName = sID;
            bundle.m_sDescription = "This plugin bundle is referenced by the project, but doesn't exist. Please check that all plugins are built correctly and their respective *.plPluginBundle files are copied to the binary directory.";
            bundle.m_LastModificationTime = plTimestamp::CurrentTimestamp();
          }
        }

        pState = pState->GetSibling();
      }

      m_PluginBundles.ReadStateFromDDL(ddl);
    }
  }

  plDynamicArray<plApplicationPluginConfig::PluginConfig> order;

  // first all the mandatory bundles
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (!it.Value().m_bMandatory)
      continue;

    if (AddBundlesInOrder(order, m_PluginBundles, it.Key(), true, false, false).Failed())
    {
      plQtUiServices::MessageBoxWarning("The mandatory plugin bundles have non-existing dependencies. Please make sure all plugins are properly built and the plPluginBundle files correctly reference each other.");

      return;
    }
  }

  // now the non-mandatory bundles
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (it.Value().m_bMandatory || !it.Value().m_bSelected)
      continue;

    if (AddBundlesInOrder(order, m_PluginBundles, it.Key(), true, false, false).Failed())
    {
      plQtUiServices::MessageBoxWarning("The plugin bundles have non-existing dependencies. Please make sure all plugins are properly built and the plPluginBundle files correctly reference each other.");

      return;
    }
  }

  plSet<plString> NotLoaded;
  for (const plApplicationPluginConfig::PluginConfig& it : order)
  {
    if (plPlugin::LoadPlugin(it.m_sAppDirRelativePath, it.m_bLoadCopy ? plPluginLoadFlags::LoadCopy : plPluginLoadFlags::Default).Failed())
    {
      NotLoaded.Insert(it.m_sAppDirRelativePath);
    }
  }

  if (!NotLoaded.IsEmpty())
  {
    plStringBuilder s = "The following plugins could not be loaded. Scenes may not load correctly.\n\n";

    for (auto it = NotLoaded.GetIterator(); it.IsValid(); ++it)
    {
      s.AppendFormat(" '{0}' \n", it.Key());
    }

    plQtUiServices::MessageBoxWarning(s);
  }
}

void plQtEditorApp::LaunchEditor(const char* szProject, bool bCreate)
{
  if (m_bWroteCrashIndicatorFile)
  {
    // orderly shutdown -> make sure the crash indicator file is gone
    plStringBuilder sTemp = plOSFile::GetTempDataFolder("plEditor");
    sTemp.AppendPath("plEditorCrashIndicator");
    plOSFile::DeleteFile(sTemp).IgnoreResult();
    m_bWroteCrashIndicatorFile = false;
  }

  plStringBuilder app;
  app = plOSFile::GetApplicationDirectory();
  app.AppendPath("Editor");
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  app.Append(".exe");
#endif
  app.MakeCleanPath();

  // TODO: pass through all command line arguments ?

  QStringList args;
  args << "-nosplash";
  args << (bCreate ? "-newproject" : "-project");
  args << QString::fromUtf8(szProject);

  if (m_StartupFlags.IsSet(StartupFlags::SafeMode))
    args << "-safe";
  if (m_StartupFlags.IsSet(StartupFlags::NoRecent))
    args << "-noRecent";

  QProcess proc;
  proc.startDetached(QString::fromUtf8(app, app.GetElementCount()), args);
}

const plApplicationPluginConfig plQtEditorApp::GetRuntimePluginConfig(bool bIncludeEditorPlugins) const
{
  plApplicationPluginConfig cfg;

  plHybridArray<plString, 16> order;
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (it.Value().m_bMandatory || it.Value().m_bSelected)
    {
      AddBundlesInOrder(cfg.m_Plugins, m_PluginBundles, it.Key(), false, bIncludeEditorPlugins, true).IgnoreResult();
    }
  }

  return cfg;
}

void plQtEditorApp::ReloadEngineResources()
{
  plSimpleConfigMsgToEngine msg;
  msg.m_sWhatToDo = "ReloadResources";
  msg.m_sPayload = "ReloadAllResources";
  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
