#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Core/Collection/CollectionResource.h>
#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plFallbackGameState, 1, plRTTIDefaultAllocator<plFallbackGameState>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plFallbackGameState::plFallbackGameState()
{
  m_iActiveCameraComponentIndex = -3;
}

void plFallbackGameState::EnableSceneSelectionMenu(bool bEnable)
{
  m_bEnableSceneSelectionMenu = bEnable;
}

void plFallbackGameState::EnableFreeCameras(bool bEnable)
{
  m_bEnableFreeCameras = bEnable;
}

void plFallbackGameState::EnableAutoSwitchToLoadedScene(bool bEnable)
{
  m_bAutoSwitchToLoadedScene = bEnable;
}

plGameStatePriority plFallbackGameState::DeterminePriority(plWorld* pWorld) const
{
  return plGameStatePriority::Fallback;
}

void plFallbackGameState::OnActivation(plWorld* pWorld, const plTransform* pStartPosition)
{
  SUPER::OnActivation(pWorld, pStartPosition);

  // if we already have a scene (editor use case), just use that and don't create any other world
  if (pWorld != nullptr)
    return;

  // otherwise we need to load a scene

  SwitchToLoadingScreen();

  if (!plFileSystem::ExistsFile(":project/plProject"))
  {
    m_bShowMenu = true;

    if (plCommandLineUtils::GetGlobalInstance()->HasOption("-project"))
      m_State = State::BadProject;
    else
      m_State = State::NoProject;
  }
  else
  {
    plStringBuilder sScenePath = GetStartupSceneFile();
    sScenePath.MakeCleanPath();

    if (sScenePath.IsEmpty())
    {
      m_bShowMenu = true;
      m_State = State::NoScene;
    }
    else if (StartSceneLoading(sScenePath, {}).Failed())
    {
      m_bShowMenu = true;
      m_State = State::BadScene;
    }
  }
}

void plFallbackGameState::OnDeactivation()
{
  CancelSceneLoading();

  SUPER::OnDeactivation();
}

plString plFallbackGameState::GetStartupSceneFile()
{
  return plCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");
}

void plFallbackGameState::SwitchToLoadingScreen()
{
  m_sTitleOfActiveScene = "Loading Screen";
  m_bIsInLoadingScreen = true;

  m_pActiveWorld = std::move(CreateLoadingScreenWorld());
  ChangeMainWorld(m_pActiveWorld.Borrow());
}

plUniquePtr<plWorld> plFallbackGameState::CreateLoadingScreenWorld()
{
  plWorldDesc desc("LoadingScreen");

  return PL_DEFAULT_NEW(plWorld, desc);
}

plResult plFallbackGameState::StartSceneLoading(plStringView sSceneFile, plStringView sPreloadCollection)
{
  if (m_pSceneToLoad != nullptr && m_sTitleOfLoadingScene == sSceneFile)
  {
    // already being loaded
    return PL_SUCCESS;
  }

  m_sTitleOfLoadingScene = sSceneFile;

  m_pSceneToLoad = PL_DEFAULT_NEW(plSceneLoadUtility);
  m_pSceneToLoad->StartSceneLoading(sSceneFile, sPreloadCollection);

  if (m_pSceneToLoad->GetLoadingState() == plSceneLoadUtility::LoadingState::Failed)
  {
    plLog::Error("Scene loading failed: {}", m_pSceneToLoad->GetLoadingFailureReason());
    CancelSceneLoading();
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

void plFallbackGameState::CancelSceneLoading()
{
  m_sTitleOfLoadingScene.Clear();
  m_pSceneToLoad.Clear();
}

bool plFallbackGameState::IsLoadingScene() const
{
  return m_pSceneToLoad != nullptr;
}

void plFallbackGameState::SwitchToLoadedScene()
{
  PL_ASSERT_DEV(IsLoadingScene(), "Can't switch to loaded scene, if no scene is currently being loaded.");
  PL_ASSERT_DEV(m_pSceneToLoad->GetLoadingState() == plSceneLoadUtility::LoadingState::FinishedSuccessfully, "Can't switch to loaded scene before it has finished loading.");

  m_State = State::Ok;
  m_sTitleOfActiveScene = m_sTitleOfLoadingScene;
  m_pActiveWorld = m_pSceneToLoad->RetrieveLoadedScene();
  ChangeMainWorld(m_pActiveWorld.Borrow());
  SpawnPlayer(nullptr).IgnoreResult();

  CancelSceneLoading();

  m_bIsInLoadingScreen = false;
}

plResult plFallbackGameState::SpawnPlayer(const plTransform* pStartPosition)
{
  if (SUPER::SpawnPlayer(pStartPosition).Succeeded())
    return PL_SUCCESS;

  if (m_pMainWorld && pStartPosition)
  {
    m_iActiveCameraComponentIndex = -1; // set free camera
    m_MainCamera.LookAt(pStartPosition->m_vPosition, pStartPosition->m_vPosition + pStartPosition->m_qRotation * plVec3(1, 0, 0),
      pStartPosition->m_qRotation * plVec3(0, 0, 1));
  }

  return PL_FAILURE;
}

static plHybridArray<plGameAppInputConfig, 16> g_AllInput;

static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  plGameAppInputConfig& gacfg = g_AllInput.ExpandAndGetRef();
  gacfg.m_sInputSet = szInputSet;
  gacfg.m_sInputAction = szInputAction;
  gacfg.m_sInputSlotTrigger[0] = szKey1;
  gacfg.m_sInputSlotTrigger[1] = szKey2;
  gacfg.m_sInputSlotTrigger[2] = szKey3;
  gacfg.m_bApplyTimeScaling = true;

  plInputActionConfig cfg;

  cfg = plInputManager::GetInputActionConfig(szInputSet, szInputAction);
  cfg.m_bApplyTimeScaling = true;

  if (szKey1 != nullptr)
    cfg.m_sInputSlotTrigger[0] = szKey1;
  if (szKey2 != nullptr)
    cfg.m_sInputSlotTrigger[1] = szKey2;
  if (szKey3 != nullptr)
    cfg.m_sInputSlotTrigger[2] = szKey3;

  plInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void plFallbackGameState::ConfigureInputActions()
{
  g_AllInput.Clear();

  RegisterInputAction("Game", "MoveForwards", plInputSlot_KeyW);
  RegisterInputAction("Game", "MoveBackwards", plInputSlot_KeyS);
  RegisterInputAction("Game", "MoveLeft", plInputSlot_KeyA);
  RegisterInputAction("Game", "MoveRight", plInputSlot_KeyD);
  RegisterInputAction("Game", "MoveUp", plInputSlot_KeyE);
  RegisterInputAction("Game", "MoveDown", plInputSlot_KeyQ);
  RegisterInputAction("Game", "Run", plInputSlot_KeyLeftShift);

  RegisterInputAction("Game", "TurnLeft", plInputSlot_MouseMoveNegX, plInputSlot_KeyLeft);
  RegisterInputAction("Game", "TurnRight", plInputSlot_MouseMovePosX, plInputSlot_KeyRight);
  RegisterInputAction("Game", "TurnUp", plInputSlot_MouseMoveNegY, plInputSlot_KeyUp);
  RegisterInputAction("Game", "TurnDown", plInputSlot_MouseMovePosY, plInputSlot_KeyDown);

  RegisterInputAction("Game", "NextCamera", plInputSlot_KeyPageDown);
  RegisterInputAction("Game", "PrevCamera", plInputSlot_KeyPageUp);
}

const plCameraComponent* plFallbackGameState::FindActiveCameraComponent()
{
  if (m_iActiveCameraComponentIndex == -1)
    return nullptr;

  const plWorld* pWorld = m_pMainWorld;
  const plCameraComponentManager* pManager = pWorld->GetComponentManager<plCameraComponentManager>();
  if (pManager == nullptr)
    return nullptr;

  auto itComp = pManager->GetComponents();

  plHybridArray<const plCameraComponent*, 32> Cameras[plCameraUsageHint::ENUM_COUNT];

  // first find all cameras and sort them by usage type
  while (itComp.IsValid())
  {
    const plCameraComponent* pComp = itComp;

    if (pComp->IsActive())
    {
      Cameras[pComp->GetUsageHint().GetValue()].PushBack(pComp);
    }

    itComp.Next();
  }

  Cameras[plCameraUsageHint::None].Clear();
  Cameras[plCameraUsageHint::RenderTarget].Clear();
  Cameras[plCameraUsageHint::Culling].Clear();
  Cameras[plCameraUsageHint::Shadow].Clear();
  Cameras[plCameraUsageHint::Thumbnail].Clear();

  if (m_iActiveCameraComponentIndex == -3)
  {
    // take first camera of a good usage type
    m_iActiveCameraComponentIndex = 0;
  }

  // take last camera (wrap around)
  if (m_iActiveCameraComponentIndex == -2)
  {
    m_iActiveCameraComponentIndex = 0;
    for (plUInt32 i = 0; i < plCameraUsageHint::ENUM_COUNT; ++i)
    {
      m_iActiveCameraComponentIndex += Cameras[i].GetCount();
    }

    --m_iActiveCameraComponentIndex;
  }

  if (m_iActiveCameraComponentIndex >= 0)
  {
    plInt32 offset = m_iActiveCameraComponentIndex;

    // now find the camera by that index
    for (plUInt32 i = 0; i < plCameraUsageHint::ENUM_COUNT; ++i)
    {
      if (offset < (plInt32)Cameras[i].GetCount())
        return Cameras[i][offset];

      offset -= Cameras[i].GetCount();
    }
  }

  // on overflow, reset to free camera
  m_iActiveCameraComponentIndex = -1;
  return nullptr;
}

void plFallbackGameState::ProcessInput()
{
  if (IsLoadingScene())
  {
    m_pSceneToLoad->TickSceneLoading();

    switch (m_pSceneToLoad->GetLoadingState())
    {
      case plSceneLoadUtility::LoadingState::FinishedSuccessfully:
        if (m_bAutoSwitchToLoadedScene)
        {
          SwitchToLoadedScene();
        }
        break;

      case plSceneLoadUtility::LoadingState::Failed:
        plLog::Error("Scene loading failed: {}", m_pSceneToLoad->GetLoadingFailureReason());
        CancelSceneLoading();
        break;

      default:
        break;
    }
  }

  if (m_bEnableSceneSelectionMenu)
  {
    if (plInputManager::GetExclusiveInputSet().IsEmpty() || plInputManager::GetExclusiveInputSet() == "plPlayer")
    {
      if (DisplayMenu())
      {
        // prevents the currently active scene from getting any input
        plInputManager::SetExclusiveInputSet("plPlayer");
      }
      else
      {
        // allows the active scene to retrieve input again
        plInputManager::SetExclusiveInputSet("");
      }
    }
  }

  if (m_bEnableFreeCameras)
  {
    PL_LOCK(m_pMainWorld->GetReadMarker());

    if (plInputManager::GetInputActionState("Game", "NextCamera") == plKeyState::Pressed)
      ++m_iActiveCameraComponentIndex;
    if (plInputManager::GetInputActionState("Game", "PrevCamera") == plKeyState::Pressed)
      --m_iActiveCameraComponentIndex;

    const plCameraComponent* pCamComp = FindActiveCameraComponent();
    if (pCamComp)
    {
      return;
    }

    float fRotateSpeed = 180.0f;
    float fMoveSpeed = 10.0f;
    float fInput = 0.0f;

    if (plInputManager::GetInputActionState("Game", "Run", &fInput) != plKeyState::Up)
      fMoveSpeed *= 10.0f;

    if (plInputManager::GetInputActionState("Game", "MoveForwards", &fInput) != plKeyState::Up)
      m_MainCamera.MoveLocally(fInput * fMoveSpeed, 0, 0);
    if (plInputManager::GetInputActionState("Game", "MoveBackwards", &fInput) != plKeyState::Up)
      m_MainCamera.MoveLocally(-fInput * fMoveSpeed, 0, 0);
    if (plInputManager::GetInputActionState("Game", "MoveLeft", &fInput) != plKeyState::Up)
      m_MainCamera.MoveLocally(0, -fInput * fMoveSpeed, 0);
    if (plInputManager::GetInputActionState("Game", "MoveRight", &fInput) != plKeyState::Up)
      m_MainCamera.MoveLocally(0, fInput * fMoveSpeed, 0);

    if (plInputManager::GetInputActionState("Game", "MoveUp", &fInput) != plKeyState::Up)
      m_MainCamera.MoveGlobally(0, 0, fInput * fMoveSpeed);
    if (plInputManager::GetInputActionState("Game", "MoveDown", &fInput) != plKeyState::Up)
      m_MainCamera.MoveGlobally(0, 0, -fInput * fMoveSpeed);

    if (plInputManager::GetInputActionState("Game", "TurnLeft", &fInput) != plKeyState::Up)
      m_MainCamera.RotateGlobally(plAngle(), plAngle(), plAngle::MakeFromDegree(-fRotateSpeed * fInput));
    if (plInputManager::GetInputActionState("Game", "TurnRight", &fInput) != plKeyState::Up)
      m_MainCamera.RotateGlobally(plAngle(), plAngle(), plAngle::MakeFromDegree(fRotateSpeed * fInput));
    if (plInputManager::GetInputActionState("Game", "TurnUp", &fInput) != plKeyState::Up)
      m_MainCamera.RotateLocally(plAngle(), plAngle::MakeFromDegree(fRotateSpeed * fInput), plAngle());
    if (plInputManager::GetInputActionState("Game", "TurnDown", &fInput) != plKeyState::Up)
      m_MainCamera.RotateLocally(plAngle(), plAngle::MakeFromDegree(-fRotateSpeed * fInput), plAngle());
  }
}

void plFallbackGameState::AfterWorldUpdate()
{
  PL_LOCK(m_pMainWorld->GetReadMarker());

  // Update the camera transform after world update so the owner node has its final position for this frame.
  // Setting the camera transform in ProcessInput introduces one frame delay.
  if (const plCameraComponent* pCamComp = FindActiveCameraComponent())
  {
    if (pCamComp->GetCameraMode() != plCameraMode::Stereo && m_MainCamera.GetCameraMode() != plCameraMode::Stereo)
    {
      const plGameObject* pOwner = pCamComp->GetOwner();
      plVec3 vPosition = pOwner->GetGlobalPosition();
      plVec3 vForward = pOwner->GetGlobalDirForwards();
      plVec3 vUp = pOwner->GetGlobalDirUp();

      m_MainCamera.LookAt(vPosition, vPosition + vForward, vUp);
    }
  }
}

void plFallbackGameState::FindAvailableScenes()
{
  if (m_bCheckedForScenes)
    return;

  m_bCheckedForScenes = true;

  if (!plFileSystem::ExistsFile(":project/plProject"))
    return;

#if PL_ENABLED(PL_SUPPORTS_FILE_ITERATORS)
  plFileSystemIterator fsit;
  plStringBuilder sScenePath;

  for (plFileSystem::StartSearch(fsit, "", plFileSystemIteratorFlags::ReportFilesRecursive);
       fsit.IsValid(); fsit.Next())
  {
    fsit.GetStats().GetFullPath(sScenePath);

    if (!sScenePath.HasExtension(".plScene"))
      continue;

    sScenePath.MakeRelativeTo(fsit.GetCurrentSearchTerm()).AssertSuccess();

    m_AvailableScenes.PushBack(sScenePath);
  }
#endif
}

bool plFallbackGameState::DisplayMenu()
{
  if (IsLoadingScene() || m_pMainWorld == nullptr)
    return false;

  auto pWorld = m_pMainWorld;

  if (m_State == State::NoProject)
  {
    plDebugRenderer::DrawInfoText(pWorld, plDebugTextPlacement::TopCenter, "_Player", "No project path provided.\n\nUse the command-line argument\n-project \"Path/To/plProject\"\nto tell plPlayer which project to load.\n\nWith the argument\n-scene \"Path/To/Scene.plScene\"\nyou can also directly load a specific scene.\n\nPress ESC to quit.", plColor::Red);

    return false;
  }

  if (m_State == State::BadProject)
  {
    plDebugRenderer::DrawInfoText(pWorld, plDebugTextPlacement::TopCenter, "_Player", plFmt("Invalid project path provided.\nThe given project directory does not exist:\n\n{}\n\nPress ESC to quit.", plGameApplication::GetGameApplicationInstance()->GetAppProjectPath()), plColor::Red);

    return false;
  }

  if (plInputManager::GetInputSlotState(plInputSlot_KeyLeftWin) == plKeyState::Pressed || plInputManager::GetInputSlotState(plInputSlot_KeyRightWin) == plKeyState::Pressed)
  {
    m_bShowMenu = !m_bShowMenu;
  }

  if (m_State == State::Ok && !m_bShowMenu)
    return false;

  plDebugRenderer::DrawInfoText(pWorld, plDebugTextPlacement::TopCenter, "_Player", plFmt("Project: '{}'", plGameApplication::GetGameApplicationInstance()->GetAppProjectPath()), plColor::White);

  if (m_State == State::NoScene)
  {
    plDebugRenderer::DrawInfoText(pWorld, plDebugTextPlacement::TopCenter, "_Player", "No scene path provided.\n\nUse the command-line argument\n-scene \"Path/To/Scene.plScene\"\nto directly load a specific scene.", plColor::Orange);
  }
  else if (m_State == State::BadScene)
  {
    plDebugRenderer::DrawInfoText(pWorld, plDebugTextPlacement::TopCenter, "_Player", plFmt("Failed to load scene: '{}'", m_sTitleOfLoadingScene), plColor::Red);
  }
  else
  {
    plDebugRenderer::DrawInfoText(pWorld, plDebugTextPlacement::TopCenter, "_Player", plFmt("Scene: '{}'", m_sTitleOfActiveScene), plColor::White);
  }

  if (m_bShowMenu)
  {
    FindAvailableScenes();

    plDebugRenderer::DrawInfoText(pWorld, plDebugTextPlacement::TopCenter, "_Player", "\nSelect scene:\n", plColor::White);

    for (plUInt32 i = 0; i < m_AvailableScenes.GetCount(); ++i)
    {
      const auto& file = m_AvailableScenes[i];

      if (i == m_uiSelectedScene)
      {
        plDebugRenderer::DrawInfoText(pWorld, plDebugTextPlacement::TopCenter, "_Player", plFmt("> {} <", file), plColor::Gold);
      }
      else
      {
        plDebugRenderer::DrawInfoText(pWorld, plDebugTextPlacement::TopCenter, "_Player", plFmt("  {}  ", file), plColor::GhostWhite);
      }
    }

    plDebugRenderer::DrawInfoText(pWorld, plDebugTextPlacement::TopCenter, "_Player", "\nPress 'Return' to load scene.\nPress the 'Windows' key to toggle this menu.", plColor::White);

    if (plInputManager::GetInputSlotState(plInputSlot_KeyEscape) == plKeyState::Pressed)
    {
      m_bShowMenu = false;
    }
    else if (!m_AvailableScenes.IsEmpty())
    {
      if (plInputManager::GetInputSlotState(plInputSlot_KeyUp) == plKeyState::Pressed)
      {
        if (m_uiSelectedScene == 0)
          m_uiSelectedScene = m_AvailableScenes.GetCount() - 1;
        else
          --m_uiSelectedScene;
      }

      if (plInputManager::GetInputSlotState(plInputSlot_KeyDown) == plKeyState::Pressed)
      {
        if (m_uiSelectedScene == m_AvailableScenes.GetCount() - 1)
          m_uiSelectedScene = 0;
        else
          ++m_uiSelectedScene;
      }

      if (plInputManager::GetInputSlotState(plInputSlot_KeyReturn) == plKeyState::Pressed || plInputManager::GetInputSlotState(plInputSlot_KeyNumpadEnter) == plKeyState::Pressed)
      {
        if (StartSceneLoading(m_AvailableScenes[m_uiSelectedScene], {}).Succeeded())
        {
          m_bShowMenu = false;
        }
        else
        {
          m_bShowMenu = true;
          m_State = State::BadScene;
        }
      }

      return true;
    }
  }

  return false;
}

bool plFallbackGameState::IsInLoadingScreen() const
{
  return m_bIsInLoadingScreen;
}

PL_STATICLINK_FILE(GameEngine, GameEngine_GameState_Implementation_FallbackGameState);
