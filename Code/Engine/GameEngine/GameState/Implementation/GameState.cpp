
#include <GameEngine/GameEnginePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/ActorSystem/ActorPluginWindow.h>
#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/GameState/GameStateWindow.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Screen.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GameEngine/Configuration/XRConfig.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>
#include <GameEngine/XR/DummyXR.h>
#include <GameEngine/XR/XRInterface.h>
#include <GameEngine/XR/XRRemotingInterface.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Utils/CoreRenderProfile.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameState, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_GameState);
// clang-format on

plGameState::plGameState() = default;
plGameState::~plGameState() = default;

void plGameState::OnActivation(plWorld* pWorld, const plTransform* pStartPosition)
{
  m_pMainWorld = pWorld;
  {
    ConfigureMainCamera();

    CreateActors();
  }

  ConfigureInputActions();

  SpawnPlayer(pStartPosition).IgnoreResult();
}

void plGameState::OnDeactivation()
{
  if (m_bXREnabled)
  {
    m_bXREnabled = false;
    plXRInterface* pXRInterface = plSingletonRegistry::GetSingletonInstance<plXRInterface>();
    plActorManager::GetSingleton()->DestroyAllActors(pXRInterface);
    pXRInterface->Deinitialize();

    if (plXRRemotingInterface* pXRRemotingInterface = plSingletonRegistry::GetSingletonInstance<plXRRemotingInterface>())
    {
      if (pXRRemotingInterface->Deinitialize().Failed())
      {
        plLog::Error("Failed to deinitialize plXRRemotingInterface, make sure all actors are destroyed and plXRInterface deinitialized.");
      }
    }

    m_pDummyXR = nullptr;
  }

  plRenderWorld::DeleteView(m_hMainView);
}

void plGameState::ScheduleRendering()
{
  plRenderWorld::AddMainView(m_hMainView);
}

plUniquePtr<plActor> plGameState::CreateXRActor()
{
  PL_LOG_BLOCK("CreateXRActor");
  // Init XR
  const plXRConfig* pConfig = plGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<plXRConfig>();
  if (!pConfig)
    return nullptr;

  if (!pConfig->m_bEnableXR)
    return nullptr;

  plXRInterface* pXRInterface = plSingletonRegistry::GetSingletonInstance<plXRInterface>();
  if (!pXRInterface)
  {
    plLog::Warning("No plXRInterface interface found. Please load a XR plugin to enable XR. Loading dummyXR interface.");
    m_pDummyXR = PL_DEFAULT_NEW(plDummyXR);
    pXRInterface = plSingletonRegistry::GetSingletonInstance<plXRInterface>();
    PL_ASSERT_DEV(pXRInterface, "Creating dummyXR did not register the plXRInterface.");
  }

  plXRRemotingInterface* pXRRemotingInterface = plSingletonRegistry::GetSingletonInstance<plXRRemotingInterface>();
  if (plXRRemotingInterface::cvar_XrRemoting)
  {
    if (pXRRemotingInterface)
    {
      if (pXRRemotingInterface->Initialize().Failed())
      {
        plLog::Error("plXRRemotingInterface could not be initialized. See log for details.");
      }
      else
      {
        m_bXRRemotingEnabled = true;
      }
    }
    else
    {
      plLog::Error("No plXRRemotingInterface interface found. Please load a XR remoting plugin to enable XR Remoting.");
    }
  }

  if (pXRInterface->Initialize().Failed())
  {
    plLog::Error("plXRInterface could not be initialized. See log for details.");
    return nullptr;
  }
  m_bXREnabled = true;

  plUniquePtr<plWindow> pMainWindow;
  plUniquePtr<plWindowOutputTargetGAL> pOutput;

  if (pXRInterface->SupportsCompanionView())
  {
    // XR Window with added companion window (allows keyboard / mouse input).
    pMainWindow = CreateMainWindow();
    PL_ASSERT_DEV(pMainWindow != nullptr, "To change the main window creation behavior, override plGameState::CreateActors().");
    pOutput = CreateMainOutputTarget(pMainWindow.Borrow());
    ConfigureMainWindowInputDevices(pMainWindow.Borrow());
    CreateMainView();
    SetupMainView(pOutput->m_hSwapChain, pMainWindow->GetClientAreaSize());
  }
  else
  {
    // XR Window (no companion window)
    CreateMainView();
    SetupMainView({}, {});
  }

  if (m_bXRRemotingEnabled)
  {
    if (pXRRemotingInterface->Connect(plXRRemotingInterface::cvar_XrRemotingHostName.GetValue().GetData()).Failed())
    {
      plLog::Error("Failed to connect XR Remoting.");
    }
  }

  plView* pView = nullptr;
  PL_VERIFY(plRenderWorld::TryGetView(m_hMainView, pView), "");
  plUniquePtr<plActor> pXRActor = pXRInterface->CreateActor(pView, plGALMSAASampleCount::Default, std::move(pMainWindow), std::move(pOutput));
  return std::move(pXRActor);
}

void plGameState::CreateActors()
{
  PL_LOG_BLOCK("CreateActors");
  plUniquePtr<plActor> pXRActor = CreateXRActor();
  if (pXRActor != nullptr)
  {
    plActorManager::GetSingleton()->AddActor(std::move(pXRActor));
    return;
  }

  plUniquePtr<plWindow> pMainWindow = CreateMainWindow();
  PL_ASSERT_DEV(pMainWindow != nullptr, "To change the main window creation behavior, override plGameState::CreateActors().");
  plUniquePtr<plWindowOutputTargetGAL> pOutput = CreateMainOutputTarget(pMainWindow.Borrow());
  ConfigureMainWindowInputDevices(pMainWindow.Borrow());
  CreateMainView();
  SetupMainView(pOutput->m_hSwapChain, pMainWindow->GetClientAreaSize());

  {
    // Default flat window
    plUniquePtr<plActorPluginWindowOwner> pWindowPlugin = PL_DEFAULT_NEW(plActorPluginWindowOwner);
    pWindowPlugin->m_pWindow = std::move(pMainWindow);
    pWindowPlugin->m_pWindowOutputTarget = std::move(pOutput);
    plUniquePtr<plActor> pActor = PL_DEFAULT_NEW(plActor, "Main Window", this);
    pActor->AddPlugin(std::move(pWindowPlugin));
    plActorManager::GetSingleton()->AddActor(std::move(pActor));
  }
}

void plGameState::ConfigureMainWindowInputDevices(plWindow* pWindow) {}

void plGameState::ConfigureInputActions() {}

void plGameState::SetupMainView(plGALSwapChainHandle hSwapChain, plSizeU32 viewportSize)
{
  plView* pView = nullptr;
  if (!plRenderWorld::TryGetView(m_hMainView, pView))
  {
    plLog::Error("Main view is invalid, SetupMainView canceled.");
    return;
  }

  if (m_bXREnabled)
  {
    const plXRConfig* pConfig = plGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<plXRConfig>();

    auto renderPipeline = plResourceManager::LoadResource<plRenderPipelineResource>(pConfig->m_sXRRenderPipeline);
    pView->SetRenderPipelineResource(renderPipeline);
    // Render target setup is done by plXRInterface::CreateActor
  }
  else
  {
    // Render target setup
    {
      const auto* pConfig = plGameApplicationBase::GetGameApplicationBaseInstance()->GetPlatformProfile().GetTypeConfig<plRenderPipelineProfileConfig>();
      auto renderPipeline = plResourceManager::LoadResource<plRenderPipelineResource>(pConfig->m_sMainRenderPipeline);
      pView->SetRenderPipelineResource(renderPipeline);
      pView->SetSwapChain(hSwapChain);
      pView->SetViewport(plRectFloat(0.0f, 0.0f, (float)viewportSize.width, (float)viewportSize.height));
      pView->ForceUpdate();
    }
  }
}

plView* plGameState::CreateMainView()
{
  PL_ASSERT_DEV(m_hMainView.IsInvalidated(), "CreateMainView was already called.");

  PL_LOG_BLOCK("CreateMainView");
  plView* pView = nullptr;
  m_hMainView = plRenderWorld::CreateView("MainView", pView);
  pView->SetCameraUsageHint(plCameraUsageHint::MainView);
  pView->SetWorld(m_pMainWorld);
  pView->SetCamera(&m_MainCamera);
  plRenderWorld::AddMainView(m_hMainView);

  const plTag& tagEditor = plTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
  // exclude all editor objects from rendering in proper game views
  pView->m_ExcludeTags.Set(tagEditor);
  return pView;
}

plResult plGameState::SpawnPlayer(const plTransform* pStartPosition)
{
  if (m_pMainWorld == nullptr)
    return PL_FAILURE;

  PL_LOCK(m_pMainWorld->GetWriteMarker());

  plPlayerStartPointComponentManager* pMan = m_pMainWorld->GetComponentManager<plPlayerStartPointComponentManager>();
  if (pMan == nullptr)
    return PL_FAILURE;

  for (auto it = pMan->GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActive() && it->GetPlayerPrefab().IsValid())
    {
      plResourceLock<plPrefabResource> pPrefab(it->GetPlayerPrefab(), plResourceAcquireMode::BlockTillLoaded);

      if (pPrefab.GetAcquireResult() == plResourceAcquireResult::Final)
      {
        const plUInt16 uiTeamID = it->GetOwner()->GetTeamID();
        plTransform startPos = it->GetOwner()->GetGlobalTransform();

        if (pStartPosition)
        {
          startPos = *pStartPosition;
          startPos.m_vScale.Set(1.0f);
          startPos.m_vPosition.z += 1.0f; // do not spawn player prefabs on the ground, they may not have their origin there
        }

        plPrefabInstantiationOptions options;
        options.m_pOverrideTeamID = &uiTeamID;

        pPrefab->InstantiatePrefab(*m_pMainWorld, startPos, options, &(it->m_Parameters));

        return PL_SUCCESS;
      }
    }
  }

  return PL_FAILURE;
}

void plGameState::ChangeMainWorld(plWorld* pNewMainWorld)
{
  m_pMainWorld = pNewMainWorld;

  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hMainView, pView))
  {
    pView->SetWorld(m_pMainWorld);
  }
}

void plGameState::ConfigureMainCamera()
{
  plVec3 vCameraPos = plVec3(0.0f, 0.0f, 0.0f);

  plCoordinateSystem coordSys;

  if (m_pMainWorld)
  {
    m_pMainWorld->GetCoordinateSystem(vCameraPos, coordSys);
  }
  else
  {
    coordSys.m_vForwardDir.Set(1, 0, 0);
    coordSys.m_vRightDir.Set(0, 1, 0);
    coordSys.m_vUpDir.Set(0, 0, 1);
  }

  // if the camera is already set to be in 'Stereo' mode, its parameters are set from the outside
  if (m_MainCamera.GetCameraMode() != plCameraMode::Stereo)
  {
    m_MainCamera.LookAt(vCameraPos, vCameraPos + coordSys.m_vForwardDir, coordSys.m_vUpDir);
    m_MainCamera.SetCameraMode(plCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 1000.0f);
  }
}

plCommandLineOptionPath opt_Window("GameState", "-wnd", "Path to the window configuration file to use.", "");

plUniquePtr<plWindow> plGameState::CreateMainWindow()
{
  if (false)
  {
    plHybridArray<plScreenInfo, 2> screens;
    plScreen::EnumerateScreens(screens).IgnoreResult();
    plScreen::PrintScreenInfo(screens);
  }

  plStringBuilder sWndCfg = opt_Window.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified);

  if (!sWndCfg.IsEmpty() && !plFileSystem::ExistsFile(sWndCfg))
  {
    plLog::Dev("Window Config file does not exist: '{0}'", sWndCfg);
    sWndCfg.Clear();
  }

  if (sWndCfg.IsEmpty())
  {
#if PL_ENABLED(PL_MIGRATE_RUNTIMECONFIGS)
    const plStringView sCfgAppData = plFileSystem::MigrateFileLocation(":appdata/Window.ddl", ":appdata/RuntimeConfigs/Window.ddl");
    const plStringView sCfgProject = plFileSystem::MigrateFileLocation(":project/Window.ddl", ":project/RuntimeConfigs/Window.ddl");
#else
    const plStringView sCfgAppData = ":appdata/RuntimeConfigs/Window.ddl";
    const plStringView sCfgProject = ":project/RuntimeConfigs/Window.ddl";
#endif

    if (plFileSystem::ExistsFile(sCfgAppData))
      sWndCfg = sCfgAppData;
    else
      sWndCfg = sCfgProject;
  }

  plWindowCreationDesc wndDesc;
  wndDesc.LoadFromDDL(sWndCfg).IgnoreResult();

  plUniquePtr<plGameStateWindow> pWindow = PL_DEFAULT_NEW(plGameStateWindow, wndDesc, [] {});
  pWindow->ResetOnClickClose([this]()
    { this->RequestQuit(); });
  if (pWindow->GetInputDevice())
    pWindow->GetInputDevice()->SetMouseSpeed(plVec2(0.002f));

  return pWindow;
}

plUniquePtr<plWindowOutputTargetGAL> plGameState::CreateMainOutputTarget(plWindow* pMainWindow)
{
  plUniquePtr<plWindowOutputTargetGAL> pOutput = PL_DEFAULT_NEW(plWindowOutputTargetGAL, [this](plGALSwapChainHandle hSwapChain, plSizeU32 size)
    { SetupMainView(hSwapChain, size); });

  plGALWindowSwapChainCreationDescription desc;
  desc.m_pWindow = pMainWindow;
  desc.m_BackBufferFormat = plGALResourceFormat::RGBAUByteNormalizedsRGB;
  desc.m_bAllowScreenshots = true;

  pOutput->CreateSwapchain(desc);

  return pOutput;
}
