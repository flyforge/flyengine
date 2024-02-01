#pragma once

#include <Core/GameState/GameStateBase.h>
#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class plWindow;
class plWindowOutputTargetBase;
class plView;
struct plActorEvent;
class plWindowOutputTargetGAL;
class plActor;
class plDummyXR;

using plRenderPipelineResourceHandle = plTypedResourceHandle<class plRenderPipelineResource>;

/// \brief plGameState is the base class to build custom game logic upon. It works closely together with plGameApplication.
///
/// In a typical game there is always exactly one instance of an plGameState derived class active.
/// The game state handles custom game logic, which must be handled outside plWorld, custom components and scripts.
///
/// For example a custom implementation of plGameState may handle how to show a menu, when to switch to
/// another level, how multi-player works, or which player information is transitioned from one level to the next.
/// It's main purpose is to implement high-level game logic.
///
/// plGameApplication will loop through all available plGameState implementations and ask each available one
/// whether it can handle a certain level. Each game state returns a 'score' how well it can handle the game.
///
/// In a typical game you only have one game state linked into the binary, so in that case there is no reason for
/// such a system. However, in an editor you might have multiple game states available through plugins, but
/// only one can take control.
/// In such a case, each game state may inspect the given world and check whether it is e.g. a single-player
/// or multi-player level, or whether it uses it's own game specific components, and then decide whether
/// it is the best fit for that level.
///
/// \note Do not forget to reflect your derived class, otherwise plGameApplication may not find it.
class PL_GAMEENGINE_DLL plGameState : public plGameStateBase
{
  PL_ADD_DYNAMIC_REFLECTION(plGameState, plGameStateBase)

protected:
  /// \brief This class cannot be instantiated directly.
  plGameState();

public:
  virtual ~plGameState();

  /// \brief When a game state was chosen, it gets activated through this function.
  ///
  /// \param pWorld
  /// The game state is supposed to operate on the given world.
  /// In a stand-alone application pWorld will always be nullptr and the game state is expected
  /// to create worlds itself.
  /// When run inside the editor, pWorld will already exist and the game state is expected to work on it.
  ///
  /// \param pStartPosition
  /// An optional transform for the 'player object' to start at.
  /// Usually nullptr, but may be set by the editor to relocate or create the player object at the given destination.
  virtual void OnActivation(plWorld* pWorld, const plTransform* pStartPosition) override;

  /// \brief Called when the game state is being shut down.
  virtual void OnDeactivation() override;

  /// \brief Has to call plRenderLoop::AddMainView for all views that need to be rendered
  virtual void ScheduleRendering() override;

  /// \brief Gives access to the game state's main camera object.
  plCamera* GetMainCamera() { return &m_MainCamera; }

protected:
  /// \brief Creates an actor with a default window (plGameStateWindow) adds it to the application
  ///
  /// The base implementation calls CreateMainWindow(), CreateMainOutputTarget() and SetupMainView() to configure the main window.
  virtual void CreateActors();

  /// \brief Adds custom input actions, if necessary.
  /// Unless overridden OnActivation() will call this.
  virtual void ConfigureInputActions();

  /// \brief Overridable function that may create a player object.
  ///
  /// By default called by OnActivation().
  /// The default implementation will search the world for plPlayerStartComponent's and instantiate the given player prefab at one of those
  /// locations. If pStartPosition is not nullptr, it will be used as the spawn position for the player prefab, otherwise the location of
  /// the plPlayerStartComponent will be used.
  ///
  /// Returns PL_SUCCESS if a prefab was spawned, PL_FAILURE if nothing was done.
  virtual plResult SpawnPlayer(const plTransform* pStartPosition);

  /// \brief Creates an XR Actor if XR is configured and available for the project.
  plUniquePtr<plActor> CreateXRActor();

  /// \brief Creates a default main view.
  plView* CreateMainView();

  /// \brief Sets m_pMainWorld and updates m_pMainView to use that new world for rendering
  void ChangeMainWorld(plWorld* pNewMainWorld);

  /// \brief Sets up m_MainCamera for first use
  virtual void ConfigureMainCamera();

  /// \brief Override this to modify the default window creation behavior. Called by CreateActors().
  virtual plUniquePtr<plWindow> CreateMainWindow();

  /// \brief Override this to modify the default output target creation behavior. Called by CreateActors().
  virtual plUniquePtr<plWindowOutputTargetGAL> CreateMainOutputTarget(plWindow* pMainWindow);

  /// \brief Creates a default render view. Unless overridden, OnActivation() will do this for the main window.
  virtual void SetupMainView(plGALSwapChainHandle hSwapChain, plSizeU32 viewportSize);

  /// \brief Configures available input devices, e.g. sets mouse speed, cursor clipping, etc.
  /// Called by CreateActors() with the result of CreateMainWindow().
  virtual void ConfigureMainWindowInputDevices(plWindow* pWindow);

  plViewHandle m_hMainView;

  plWorld* m_pMainWorld = nullptr;

  plCamera m_MainCamera;
  bool m_bStateWantsToQuit = false;
  bool m_bXREnabled = false;
  bool m_bXRRemotingEnabled = false;
  plUniquePtr<plDummyXR> m_pDummyXR;
};
