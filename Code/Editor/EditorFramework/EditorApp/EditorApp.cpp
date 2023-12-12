#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

PLASMA_IMPLEMENT_SINGLETON(plQtEditorApp);

plEvent<const PlasmaEditorAppEvent&> plQtEditorApp::m_Events;

plQtEditorApp::plQtEditorApp()
  : m_SingletonRegistrar(this)
  , m_RecentProjects(20)
  , m_RecentDocuments(100)
{
  m_bSavePreferencesAfterOpenProject = false;

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
    if (PlasmaEditorEngineProcessConnection::GetSingleton())
      PlasmaEditorEngineProcessConnection::GetSingleton()->Update();

    plAssetCurator::GetSingleton()->MainThreadTick(true);
  }
  plTaskSystem::FinishFrameTasks();

  // Close the splash screen when we get to the first idle event
  CloseSplashScreen();

  Q_EMIT IdleEvent();

  RestartEngineProcessIfPluginsChanged(false);

  if (m_bWroteCrashIndicatorFile)
  {
    m_bWroteCrashIndicatorFile = false;
    QTimer::singleShot(2000, []() {
        plStringBuilder sTemp = plOSFile::GetTempDataFolder("PlasmaEditor");
        sTemp.AppendPath("PlasmaEditorCrashIndicator");
        plOSFile::DeleteFile(sTemp).IgnoreResult();
      });
  }

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
    if (m_VersionChecker.IsLatestNewer())
    {
      plQtUiServices::GetSingleton()->MessageBoxInformation(
        plFmt("<html>A new version is available: {}<br><br>Your version is: {}<br><br>Please check the <A "
              "href=\"https://github.com/PlasmaEngine/PlasmaEngine/releases\">Releases</A> for details.</html>",
          m_VersionChecker.GetKnownLatestVersion(), m_VersionChecker.GetOwnVersion()));
    }
    else
    {
      plStringBuilder tmp("You have the latest version: \n");
      tmp.Append(m_VersionChecker.GetOwnVersion());

      plQtUiServices::GetSingleton()->MessageBoxInformation(tmp);
    }
  }

  if (m_VersionChecker.IsLatestNewer())
  {
    plQtUiServices::GetSingleton()->ShowGlobalStatusBarMessage(
      plFmt("New version '{}' available, please update.", m_VersionChecker.GetKnownLatestVersion()));
  }
}

void plQtEditorApp::EngineProcessMsgHandler(const PlasmaEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case PlasmaEditorEngineProcessConnection::Event::Type::ProcessMessage:
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

    case PlasmaEditorEngineProcessConnection::Event::Type::ProcessRestarted:
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
    m_VersionChecker.Check(true);
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
        pDoc->SaveDocument().IgnoreResult();
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
  if (m_StartupFlags.IsSet(StartupFlags::Debug))
    args << "-debug";

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
  PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
