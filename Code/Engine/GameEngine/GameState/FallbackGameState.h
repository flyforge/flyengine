#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameState/GameState.h>
#include <GameEngine/Utils/SceneLoadUtil.h>

class plCameraComponent;

using plCollectionResourceHandle = plTypedResourceHandle<class plCollectionResource>;

/// \brief plFallbackGameState is an plGameState that can handle existing worlds when no other game state is available.
///
/// This game state returns a priority of 'Fallback' in DeterminePriority() and therefore only takes over when
/// no other game state is available.
/// It implements a simple first person camera to fly around a scene.
///
/// This game state cannot be used in stand-alone applications that require the game state to create
/// a new world. It is mainly for plEditor and plPlayer which make sure that a world already exists.
class PL_GAMEENGINE_DLL plFallbackGameState : public plGameState
{
  PL_ADD_DYNAMIC_REFLECTION(plFallbackGameState, plGameState)

public:
  plFallbackGameState();

  /// \brief If disabled, pressing the Windows key won't show an onscreen menu to switch to a different scene.
  void EnableSceneSelectionMenu(bool bEnable);

  /// \brief If disabled, moving around a scene with a free camera won't be possible.
  ///
  /// Also switching between scene cameras with Page Up/Down will be disabled.
  void EnableFreeCameras(bool bEnable);

  /// \brief If disabled, the game state will not automatically switch to a scene that is being loaded in the background,
  /// once it is finished loading. Instead overriding code has to call SwitchToLoadedScene() itself.
  void EnableAutoSwitchToLoadedScene(bool bEnable);

  virtual void ProcessInput() override;
  virtual void AfterWorldUpdate() override;

  /// \brief Returns plGameStatePriority::Fallback.
  virtual plGameStatePriority DeterminePriority(plWorld* pWorld) const override;

  virtual void OnActivation(plWorld* pWorld, const plTransform* pStartPosition) override;
  virtual void OnDeactivation() override;

  /// \brief Returns the path to the scene file to load at startup. By default this is taken from the command line '-scene' option.
  virtual plString GetStartupSceneFile();

  /// \brief Creates a new world that's used as a temporary loading screen while waiting for loading of another world to finish.
  ///
  /// Usually this world would be set up in code and would be very quick to create. By default an entirely empty world is created.
  void SwitchToLoadingScreen();

  /// \brief Starts loading a scene in the background.
  ///
  /// If available, a collection can be provided. Resources referenced in the collection will be fully preloaded first and then
  /// the scene is loaded. This is the only way to get a proper estimation of loading progress and is necessary to get a smooth
  /// start, otherwise the engine will have to load resources on-demand, many of which will be needed during the first frame.
  plResult StartSceneLoading(plStringView sSceneFile, plStringView sPreloadCollection);

  /// \brief If a scene is currently being loaded in the background, cancel the loading.
  void CancelSceneLoading();

  /// \brief Whether a scene is currently being loaded.
  bool IsLoadingScene() const;

  /// \brief Whether the game state currently displays a loading screen. This usually implies that a scene is being loaded as well.
  bool IsInLoadingScreen() const;

  /// \brief Once scene loading has finished successfully, this can be called to switch to that scene.
  void SwitchToLoadedScene();

  /// \brief Returns the name of the plWorld that is currently active.
  plStringView GetActiveSceneName() const { return m_sTitleOfActiveScene; }

  /// \brief Returns the name of the plWorld that is currently being loaded.
  plStringView GetLoadingSceneName() const { return m_sTitleOfLoadingScene; }

protected:
  /// \brief Called by SwitchToLoadingScreen() to setup a new world that acts as the loading screen while waiting for another scene to finish loading.
  virtual plUniquePtr<plWorld> CreateLoadingScreenWorld();
  virtual void ConfigureInputActions() override;
  virtual plResult SpawnPlayer(const plTransform* pStartPosition) override;

  virtual const plCameraComponent* FindActiveCameraComponent();

  plInt32 m_iActiveCameraComponentIndex;

  plUniquePtr<plWorld> m_pActiveWorld;

  plUniquePtr<plSceneLoadUtility> m_pSceneToLoad;

  //////////////////////////////////////////////////////////////////////////

  enum class State
  {
    Ok,
    NoProject,
    BadProject,
    NoScene,
    BadScene,
  };

  State m_State = State::Ok;
  bool m_bShowMenu = false;
  bool m_bEnableSceneSelectionMenu = true;
  bool m_bEnableFreeCameras = true;
  bool m_bIsInLoadingScreen = false;
  bool m_bAutoSwitchToLoadedScene = true;

  void FindAvailableScenes();
  bool DisplayMenu();

  bool m_bCheckedForScenes = false;
  plDynamicArray<plString> m_AvailableScenes;
  plUInt32 m_uiSelectedScene = 0;
  plString m_sTitleOfLoadingScene;
  plString m_sTitleOfActiveScene;
};
