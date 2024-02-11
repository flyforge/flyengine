#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorFramework/CodeGen/CompilerPreferencesWidget.moc.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/CheckVersion.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/DynamicDefaultStateProvider.h>
#include <EditorFramework/GUI/ExposedParametersDefaultStateProvider.h>
#include <EditorFramework/Manipulators/BoneManipulatorAdapter.h>
#include <EditorFramework/Manipulators/BoxManipulatorAdapter.h>
#include <EditorFramework/Manipulators/CapsuleManipulatorAdapter.h>
#include <EditorFramework/Manipulators/ConeAngleManipulatorAdapter.h>
#include <EditorFramework/Manipulators/ConeLengthManipulatorAdapter.h>
#include <EditorFramework/Manipulators/ManipulatorAdapterRegistry.h>
#include <EditorFramework/Manipulators/NonUniformBoxManipulatorAdapter.h>
#include <EditorFramework/Manipulators/SphereManipulatorAdapter.h>
#include <EditorFramework/Manipulators/TransformManipulatorAdapter.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <EditorFramework/Panels/CVarPanel/CVarPanel.moc.h>
#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Panels/LongOpsPanel/LongOpsPanel.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/ExposedBoneWidget.moc.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/FileBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/GameObjectReferencePropertyWidget.moc.h>
#include <EditorFramework/Visualizers/BoxVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CameraVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CapsuleVisualizerAdapter.h>
#include <EditorFramework/Visualizers/ConeVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CylinderVisualizerAdapter.h>
#include <EditorFramework/Visualizers/DirectionVisualizerAdapter.h>
#include <EditorFramework/Visualizers/SphereVisualizerAdapter.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/ETWWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <QSvgRenderer>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ads/DockManager.h>

void plCompilerPreferences_PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, EditorFrameworkMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "GuiFoundation",
    "PropertyGrid",
    "ManipulatorAdapterRegistry",
    "DefaultState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plDefaultState::RegisterDefaultStateProvider(plExposedParametersDefaultStateProvider::CreateProvider);
    plDefaultState::RegisterDefaultStateProvider(plDynamicDefaultStateProvider::CreateProvider);
    plProjectActions::RegisterActions();
    plAssetActions::RegisterActions();
    plViewActions::RegisterActions();
    plViewLightActions::RegisterActions();
    plGameObjectContextActions::RegisterActions();
    plGameObjectDocumentActions::RegisterActions();
    plGameObjectSelectionActions::RegisterActions();
    plQuadViewActions::RegisterActions();
    plTransformGizmoActions::RegisterActions();
    plTranslateGizmoAction::RegisterActions();
    plCommonAssetActions::RegisterActions();

    plActionMapManager::RegisterActionMap("SettingsTabMenuBar").IgnoreResult();
    plStandardMenus::MapActions("SettingsTabMenuBar", plStandardMenuTypes::Default);
    plProjectActions::MapActions("SettingsTabMenuBar");

    plActionMapManager::RegisterActionMap("AssetBrowserToolBar").IgnoreResult();
    plAssetActions::MapToolBarActions("AssetBrowserToolBar", false);

    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plFileBrowserAttribute>(), [](const plRTTI* pRtti)->plQtPropertyWidget* { return new plQtFilePropertyWidget(); });
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plAssetBrowserAttribute>(), [](const plRTTI* pRtti)->plQtPropertyWidget* { return new plQtAssetPropertyWidget(); });
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plDynamicEnumAttribute>(), [](const plRTTI* pRtti)->plQtPropertyWidget* { return new plQtDynamicEnumPropertyWidget(); });
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plDynamicStringEnumAttribute>(), [](const plRTTI* pRtti)->plQtPropertyWidget* { return new plQtDynamicStringEnumPropertyWidget(); });
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plExposedParametersAttribute>(), [](const plRTTI* pRtti)->plQtPropertyWidget* { return new plQtExposedParametersPropertyWidget(); });
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plGameObjectReferenceAttribute>(), [](const plRTTI* pRtti)->plQtPropertyWidget* { return new plQtGameObjectReferencePropertyWidget(); });
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plExposedBone>(), [](const plRTTI* pRtti)->plQtPropertyWidget* { return new plQtExposedBoneWidget(); });
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plCompilerPreferences>(), [](const plRTTI* pRtti)->plQtPropertyWidget* { return new plQtCompilerPreferencesWidget(); });
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plImageSliderUiAttribute>(), [](const plRTTI* pRtti)->plQtPropertyWidget* { return new plQtPropertyEditorSliderWidget(); });

    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plSphereManipulatorAttribute>(), [](const plRTTI* pRtti)->plManipulatorAdapter* { return PL_DEFAULT_NEW(plSphereManipulatorAdapter); });
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plCapsuleManipulatorAttribute>(), [](const plRTTI* pRtti)->plManipulatorAdapter* { return PL_DEFAULT_NEW(plCapsuleManipulatorAdapter); });
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plBoxManipulatorAttribute>(), [](const plRTTI* pRtti)->plManipulatorAdapter* { return PL_DEFAULT_NEW(plBoxManipulatorAdapter); });
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plConeAngleManipulatorAttribute>(), [](const plRTTI* pRtti)->plManipulatorAdapter* { return PL_DEFAULT_NEW(plConeAngleManipulatorAdapter); });
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plConeLengthManipulatorAttribute>(), [](const plRTTI* pRtti)->plManipulatorAdapter* { return PL_DEFAULT_NEW(plConeLengthManipulatorAdapter); });
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plNonUniformBoxManipulatorAttribute>(), [](const plRTTI* pRtti)->plManipulatorAdapter* { return PL_DEFAULT_NEW(plNonUniformBoxManipulatorAdapter); });
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plTransformManipulatorAttribute>(), [](const plRTTI* pRtti)->plManipulatorAdapter* { return PL_DEFAULT_NEW(plTransformManipulatorAdapter); });
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plBoneManipulatorAttribute>(), [](const plRTTI* pRtti)->plManipulatorAdapter* { return PL_DEFAULT_NEW(plBoneManipulatorAdapter); });

    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plBoxVisualizerAttribute>(), [](const plRTTI* pRtti)->plVisualizerAdapter* { return PL_DEFAULT_NEW(plBoxVisualizerAdapter); });
    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plSphereVisualizerAttribute>(), [](const plRTTI* pRtti)->plVisualizerAdapter* { return PL_DEFAULT_NEW(plSphereVisualizerAdapter); });
    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plCapsuleVisualizerAttribute>(), [](const plRTTI* pRtti)->plVisualizerAdapter* { return PL_DEFAULT_NEW(plCapsuleVisualizerAdapter); });
    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plCylinderVisualizerAttribute>(), [](const plRTTI* pRtti)->plVisualizerAdapter* { return PL_DEFAULT_NEW(plCylinderVisualizerAdapter); });
    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plDirectionVisualizerAttribute>(), [](const plRTTI* pRtti)->plVisualizerAdapter* { return PL_DEFAULT_NEW(plDirectionVisualizerAdapter); });
    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plConeVisualizerAttribute>(), [](const plRTTI* pRtti)->plVisualizerAdapter* { return PL_DEFAULT_NEW(plConeVisualizerAdapter); });
    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(plGetStaticRTTI<plCameraVisualizerAttribute>(), [](const plRTTI* pRtti)->plVisualizerAdapter* { return PL_DEFAULT_NEW(plCameraVisualizerAdapter); });

    plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plCompilerPreferences_PropertyMetaStateEventHandler);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plDefaultState::UnregisterDefaultStateProvider(plExposedParametersDefaultStateProvider::CreateProvider);
    plDefaultState::UnregisterDefaultStateProvider(plDynamicDefaultStateProvider::CreateProvider);
    plProjectActions::UnregisterActions();
    plAssetActions::UnregisterActions();
    plViewActions::UnregisterActions();
    plViewLightActions::UnregisterActions();
    plGameObjectContextActions::UnregisterActions();
    plGameObjectDocumentActions::UnregisterActions();
    plGameObjectSelectionActions::UnregisterActions();
    plQuadViewActions::UnregisterActions();
    plTransformGizmoActions::UnregisterActions();
    plTranslateGizmoAction::UnregisterActions();
    plCommonAssetActions::UnregisterActions();

    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plFileBrowserAttribute>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plAssetBrowserAttribute>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plDynamicEnumAttribute>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plDynamicStringEnumAttribute>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plGameObjectReferenceAttribute>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plExposedParametersAttribute>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plExposedBone>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plCompilerPreferences>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plImageSliderUiAttribute>());

    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plSphereManipulatorAttribute>());
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plCapsuleManipulatorAttribute>());
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plBoxManipulatorAttribute>());
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plConeAngleManipulatorAttribute>());
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plConeLengthManipulatorAttribute>());
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plNonUniformBoxManipulatorAttribute>());
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plTransformManipulatorAttribute>());
    plManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plBoneManipulatorAttribute>());

    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plBoxVisualizerAttribute>());
    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plSphereVisualizerAttribute>());
    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plCapsuleVisualizerAttribute>());
    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plCylinderVisualizerAttribute>());
    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plDirectionVisualizerAttribute>());
    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plConeVisualizerAttribute>());
    plVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(plGetStaticRTTI<plCameraVisualizerAttribute>());

    plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plCompilerPreferences_PropertyMetaStateEventHandler);
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plCommandLineOptionBool opt_Safe("_Editor", "-safe", "In safe-mode the editor minimizes the risk of crashing, for instance by not loading previous projects and scenes.", false);
plCommandLineOptionBool opt_NoRecent("_Editor", "-noRecent", "Disables automatic loading of recent projects and documents.", false);

void plQtEditorApp::StartupEditor()
{
  {
    plStringBuilder sTemp = plOSFile::GetTempDataFolder("plEditor");
    sTemp.AppendPath("plEditorCrashIndicator");

    if (plOSFile::ExistsFile(sTemp))
    {
      plOSFile::DeleteFile(sTemp).IgnoreResult();

      if (plQtUiServices::GetSingleton()->MessageBoxQuestion("It seems the editor ran into problems last time.\n\nDo you want to run it in safe mode, to deactivate automatic project loading and document restoration?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
      {
        opt_Safe.GetOptions(sTemp);
        plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(sTemp);
      }
    }
  }

  plBitflags<StartupFlags> startupFlags;

  startupFlags.AddOrRemove(StartupFlags::SafeMode, opt_Safe.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified));
  startupFlags.AddOrRemove(StartupFlags::NoRecent, opt_NoRecent.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified));

  StartupEditor(startupFlags);
}

void plQtEditorApp::StartupEditor(plBitflags<StartupFlags> startupFlags, const char* szUserDataFolder)
{
  PL_PROFILE_SCOPE("StartupEditor");

  QCoreApplication::setOrganizationDomain("www.plengine.net");
  QCoreApplication::setOrganizationName("plEngine Project");
  QCoreApplication::setApplicationName(plApplication::GetApplicationInstance()->GetApplicationName().GetData());
  QCoreApplication::setApplicationVersion("1.0.0");

  m_StartupFlags = startupFlags;

  auto* pCmd = plCommandLineUtils::GetGlobalInstance();

  if (!IsInHeadlessMode())
  {
    SetupAndShowSplashScreen();

    m_pProgressbar = PL_DEFAULT_NEW(plProgress);
    m_pQtProgressbar = PL_DEFAULT_NEW(plQtProgressbar);

    plProgress::SetGlobalProgressbar(m_pProgressbar);
    m_pQtProgressbar->SetProgressbar(m_pProgressbar);
  }

  // custom command line arguments
  {
    // Make sure to disable the fileserve plugin
    pCmd->InjectCustomArgument("-fs_off");
  }

  const bool bNoRecent = m_StartupFlags.IsAnySet(StartupFlags::UnitTest | StartupFlags::SafeMode | StartupFlags::Headless | StartupFlags::NoRecent);

  const plString sApplicationName = pCmd->GetStringOption("-appname", 0, plApplication::GetApplicationInstance()->GetApplicationName());
  plApplication::GetApplicationInstance()->SetApplicationName(sApplicationName);

  QLocale::setDefault(QLocale(QLocale::English));

  m_pEngineViewProcess = new plEditorEngineProcessConnection;

  m_LongOpControllerManager.Startup(&m_pEngineViewProcess->GetCommunicationChannel());

  if (!IsInHeadlessMode())
  {
    PL_PROFILE_SCOPE("plQtContainerWindow");
    SetStyleSheet();

    plQtContainerWindow* pContainer = new plQtContainerWindow();
    pContainer->show();
  }

  plDocumentManager::s_Requests.AddEventHandler(plMakeDelegate(&plQtEditorApp::DocumentManagerRequestHandler, this));
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plQtEditorApp::DocumentManagerEventHandler, this));
  plDocument::s_EventsAny.AddEventHandler(plMakeDelegate(&plQtEditorApp::DocumentEventHandler, this));
  plToolsProject::s_Requests.AddEventHandler(plMakeDelegate(&plQtEditorApp::ProjectRequestHandler, this));
  plToolsProject::s_Events.AddEventHandler(plMakeDelegate(&plQtEditorApp::ProjectEventHandler, this));
  plEditorEngineProcessConnection::s_Events.AddEventHandler(plMakeDelegate(&plQtEditorApp::EngineProcessMsgHandler, this));
  plQtDocumentWindow::s_Events.AddEventHandler(plMakeDelegate(&plQtEditorApp::DocumentWindowEventHandler, this));
  plQtUiServices::s_Events.AddEventHandler(plMakeDelegate(&plQtEditorApp::UiServicesEvents, this));

  plStartup::StartupCoreSystems();

  // prevent restoration of window layouts when in safe mode
  plQtDocumentWindow::s_bAllowRestoreWindowLayout = !IsInSafeMode();

  {
    // Make sure that we have at least 4 worker threads for short running and 4 worker threads for long running tasks.
    // Otherwise the Editor might deadlock during asset transform.
    plInt32 iLongThreads = plMath::Max(4, (plInt32)plTaskSystem::GetNumAllocatedWorkerThreads(plWorkerThreadType::LongTasks));
    plInt32 iShortThreads = plMath::Max(4, (plInt32)plTaskSystem::GetNumAllocatedWorkerThreads(plWorkerThreadType::ShortTasks));
    plTaskSystem::SetWorkerThreadCount(iShortThreads, iLongThreads);
  }

  {
    PL_PROFILE_SCOPE("Filesystem");
    plFileSystem::DetectSdkRootDirectory().IgnoreResult();

    const plString sAppDir = plApplicationServices::GetSingleton()->GetApplicationDataFolder();
    plString sUserData = plApplicationServices::GetSingleton()->GetApplicationUserDataFolder();
    if (!plStringUtils::IsNullOrEmpty(szUserDataFolder))
    {
      sUserData = szUserDataFolder;
    }
    // make sure these folders exist
    plFileSystem::CreateDirectoryStructure(sAppDir).IgnoreResult();
    plFileSystem::CreateDirectoryStructure(sUserData).IgnoreResult();

    plFileSystem::AddDataDirectory("", "AbsPaths", ":", plFileSystem::AllowWrites).IgnoreResult();             // for absolute paths
    plFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", plFileSystem::AllowWrites).IgnoreResult();     // writing to the binary directory
    plFileSystem::AddDataDirectory(sAppDir, "AppData", "app").IgnoreResult();                                  // app specific data
    plFileSystem::AddDataDirectory(sUserData, "AppData", "appdata", plFileSystem::AllowWrites).IgnoreResult(); // for writing app user data
  }

  {
    PL_PROFILE_SCOPE("Logging");
    plInt32 iApplicationID = pCmd->GetIntOption("-appid", 0);
    plStringBuilder sLogFile;
    sLogFile.SetFormat(":appdata/Log_{0}.htm", iApplicationID);
    m_LogHTML.BeginLog(sLogFile, sApplicationName);

    plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
    plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
    plGlobalLog::AddLogWriter(plLoggingEvent::Handler(&plLogWriter::HTML::LogMessageHandler, &m_LogHTML));
    plGlobalLog::AddLogWriter(plLogWriter::ETW::LogMessageHandler);
  }
  plUniquePtr<plTranslatorFromFiles> pTranslatorEn = PL_DEFAULT_NEW(plTranslatorFromFiles);
  m_pTranslatorFromFiles = pTranslatorEn.Borrow();

  // plUniquePtr<plTranslatorFromFiles> pTranslatorDe = PL_DEFAULT_NEW(plTranslatorFromFiles);

  pTranslatorEn->AddTranslationFilesFromFolder(":app/Localization/en");
  // pTranslatorDe->LoadTranslationFilesFromFolder(":app/Localization/de");

  plTranslationLookup::AddTranslator(PL_DEFAULT_NEW(plTranslatorMakeMoreReadable));
  // plTranslationLookup::AddTranslator(PL_DEFAULT_NEW(plTranslatorLogMissing));
  plTranslationLookup::AddTranslator(std::move(pTranslatorEn));
  // plTranslationLookup::AddTranslator(std::move(pTranslatorDe));

  LoadEditorPreferences();
  plCppProject::LoadPreferences();

  plQtUiServices::GetSingleton()->LoadState();

  if (!IsInHeadlessMode())
  {
    plActionManager::LoadShortcutAssignment();

    LoadRecentFiles();

    CreatePanels();

    ShowSettingsDocument();

    if (!IsInUnitTestMode())
    {
      connect(m_pVersionChecker.Borrow(), &plQtVersionChecker::VersionCheckCompleted, this, &plQtEditorApp::SlotVersionCheckCompleted, Qt::QueuedConnection);

      m_pVersionChecker->Initialize();
      m_pVersionChecker->Check(false);
    }
  }

  LoadEditorPlugins();
  CloseSplashScreen();

  {
    plEditorAppEvent e;
    e.m_Type = plEditorAppEvent::Type::EditorStarted;
    m_Events.Broadcast(e);
  }

  plEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<plEditorPreferencesUser>();

  if (pCmd->GetStringOptionArguments("-newproject") > 0)
  {
    CreateOrOpenProject(true, pCmd->GetAbsolutePathOption("-newproject")).IgnoreResult();
  }
  else if (pCmd->GetStringOptionArguments("-project") > 0)
  {
    for (plUInt32 doc = 0; doc < pCmd->GetStringOptionArguments("-documents"); ++doc)
    {
      m_DocumentsToOpen.PushBack(pCmd->GetStringOption("-documents", doc));
    }

    CreateOrOpenProject(false, pCmd->GetAbsolutePathOption("-project")).IgnoreResult();
  }
  else if (!bNoRecent && pPreferences->m_bLoadLastProjectAtStartup)
  {
    if (!m_RecentProjects.GetFileList().IsEmpty())
    {
      CreateOrOpenProject(false, m_RecentProjects.GetFileList()[0].m_File).IgnoreResult();
    }
  }
  else if (!IsInHeadlessMode() && !IsInSafeMode())
  {
    if (plQtContainerWindow::GetContainerWindow())
    {
      plQtContainerWindow::GetContainerWindow()->ScheduleRestoreWindowLayout();
    }
  }

  connect(m_pTimer, SIGNAL(timeout()), this, SLOT(SlotTimedUpdate()), Qt::QueuedConnection);
  m_pTimer->start(1);

  if (m_bWroteCrashIndicatorFile)
  {
    QTimer::singleShot(1000, [this]()
      {
        plStringBuilder sTemp = plOSFile::GetTempDataFolder("plEditor");
        sTemp.AppendPath("plEditorCrashIndicator");
        plOSFile::DeleteFile(sTemp).IgnoreResult();
        m_bWroteCrashIndicatorFile = false;
        //
      });
  }

  if (m_StartupFlags.AreNoneSet(StartupFlags::Headless | StartupFlags::UnitTest) && !plToolsProject::GetSingleton()->IsProjectOpen())
  {
    GuiOpenDashboard();
  }
}

void plQtEditorApp::ShutdownEditor()
{
  plToolsProject::SaveProjectState();

  m_pTimer->stop();

  plToolsProject::CloseProject();

  m_LongOpControllerManager.Shutdown();

  plEditorEngineProcessConnection::s_Events.RemoveEventHandler(plMakeDelegate(&plQtEditorApp::EngineProcessMsgHandler, this));
  plToolsProject::s_Requests.RemoveEventHandler(plMakeDelegate(&plQtEditorApp::ProjectRequestHandler, this));
  plToolsProject::s_Events.RemoveEventHandler(plMakeDelegate(&plQtEditorApp::ProjectEventHandler, this));
  plDocument::s_EventsAny.RemoveEventHandler(plMakeDelegate(&plQtEditorApp::DocumentEventHandler, this));
  plDocumentManager::s_Requests.RemoveEventHandler(plMakeDelegate(&plQtEditorApp::DocumentManagerRequestHandler, this));
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plQtEditorApp::DocumentManagerEventHandler, this));
  plQtDocumentWindow::s_Events.RemoveEventHandler(plMakeDelegate(&plQtEditorApp::DocumentWindowEventHandler, this));
  plQtUiServices::s_Events.RemoveEventHandler(plMakeDelegate(&plQtEditorApp::UiServicesEvents, this));

  plQtUiServices::GetSingleton()->SaveState();

  CloseSettingsDocument();

  if (!IsInHeadlessMode())
  {
    delete plQtContainerWindow::GetContainerWindow();
  }
  // HACK to figure out why the panels are not always properly destroyed together with the ContainerWindows
  // if you run into this, please try to figure this out
  // every plQtApplicationPanel actually registers itself with a container window in its constructor
  // there its Qt 'parent' is set to the container window (there is only one)
  // that means, when the application is shut down, all plQtApplicationPanel instances should get deleted by their parent
  // ie. the container window
  // however, SOMETIMES this does not happen
  // it seems to be related to whether a panel has been opened/closed (ie. shown/hidden), and maybe also with the restored state
  {
    const auto& Panels = plQtApplicationPanel::GetAllApplicationPanels();
    plUInt32 uiNumPanels = Panels.GetCount();

    PL_ASSERT_DEBUG(uiNumPanels == 0, "Not all panels have been cleaned up correctly");

    for (plUInt32 i = 0; i < uiNumPanels; ++i)
    {
      plQtApplicationPanel* pPanel = Panels[i];
      delete pPanel;
    }
  }


  QCoreApplication::sendPostedEvents();
  qApp->processEvents();

  delete m_pEngineViewProcess;

  // Unload potential plugin referenced clipboard data to prevent crash on shutdown.
  QApplication::clipboard()->clear();
  plPlugin::UnloadAllPlugins();

  if (m_bWroteCrashIndicatorFile)
  {
    // orderly shutdown -> make sure the crash indicator file is gone
    plStringBuilder sTemp = plOSFile::GetTempDataFolder("plEditor");
    sTemp.AppendPath("plEditorCrashIndicator");
    plOSFile::DeleteFile(sTemp).IgnoreResult();
    m_bWroteCrashIndicatorFile = false;
  }

  // make sure no one tries to load any further images in parallel
  plQtImageCache::GetSingleton()->StopRequestProcessing(true);

  plTranslationLookup::Clear();

  plGlobalLog::RemoveLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::RemoveLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
  plGlobalLog::RemoveLogWriter(plLoggingEvent::Handler(&plLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  m_LogHTML.EndLog();

  PL_DEFAULT_DELETE(m_pQtProgressbar);
  PL_DEFAULT_DELETE(m_pProgressbar);
}



void plQtEditorApp::CreatePanels()
{
  PL_PROFILE_SCOPE("CreatePanels");
  plQtApplicationPanel* pAssetBrowserPanel = new plQtAssetBrowserPanel();
  plQtApplicationPanel* pLogPanel = new plQtLogPanel();
  plQtApplicationPanel* pLongOpsPanel = new plQtLongOpsPanel();
  plQtApplicationPanel* pCVarPanel = new plQtCVarPanel();
  plQtApplicationPanel* pAssetCuratorPanel = new plQtAssetCuratorPanel();

  plQtContainerWindow* pMainWnd = plQtContainerWindow::GetContainerWindow();
  ads::CDockManager* pDockManager = pMainWnd->GetDockManager();
  pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, pAssetBrowserPanel);
  pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, pLogPanel);
  pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, pAssetCuratorPanel);
  pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, pCVarPanel);
  pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, pLongOpsPanel);

  pAssetBrowserPanel->raise();
}

plCommandLineOptionBool opt_NoSplashScreen("_Editor", "-NoSplash", "Disables the editor splash-screen", false);

void plQtEditorApp::SetupAndShowSplashScreen()
{
  PL_ASSERT_DEV(m_pSplashScreen == nullptr, "Splash screen shouldn't exist already.");

  if (m_StartupFlags.IsAnySet(plQtEditorApp::StartupFlags::UnitTest))
    return;

  if (opt_NoSplashScreen.GetOptionValue(plCommandLineOption::LogMode::Never))
    return;

  bool bShowSplashScreen = true;

  // preferences are not yet available here
  {
    QSettings s;
    s.beginGroup("EditorPreferences");
    bShowSplashScreen = s.value("ShowSplashscreen", true).toBool();
    s.endGroup();
  }

  if (!bShowSplashScreen)
    return;

  // QSvgRenderer svgRenderer(QString(":/Splash/Splash/splash.svg"));

  // const qreal PixelRatio = qApp->primaryScreen()->devicePixelRatio();

  //// TODO: When migrating to Qt 5.15 or newer this should have a fixed square size and
  //// let the aspect ratio mode of the svg renderer handle the difference
  // QPixmap splashPixmap(QSize(187, 256) * PixelRatio);
  // splashPixmap.fill(Qt::transparent);
  //{
  //   QPainter painter;
  //   painter.begin(&splashPixmap);
  //   svgRenderer.render(&painter);
  //   painter.end();
  // }

  QPixmap splashPixmap(QString(":/Splash/Splash/splash.png"));

  // splashPixmap.setDevicePixelRatio(PixelRatio);

  m_pSplashScreen = new QSplashScreen(splashPixmap);
  m_pSplashScreen->setMask(splashPixmap.mask());

  // Don't set always on top if a debugger is attached to prevent it being stuck over the debugger.
  if (!plSystemInformation::IsDebuggerAttached())
  {
    m_pSplashScreen->setWindowFlag(Qt::WindowStaysOnTopHint, true);
  }
  m_pSplashScreen->show();
}

void plQtEditorApp::CloseSplashScreen()
{
  if (!m_pSplashScreen)
    return;

  PL_ASSERT_DEBUG(QThread::currentThread() == this->thread(), "CloseSplashScreen must be called from the main thread");
  QSplashScreen* pLocalSplashScreen = m_pSplashScreen;
  m_pSplashScreen = nullptr;

  pLocalSplashScreen->finish(plQtContainerWindow::GetContainerWindow());
  // if the deletion is done 'later', the splashscreen can end up as the parent window of other things
  // like messageboxes, and then the deletion will make the app crash
  delete pLocalSplashScreen;
}
