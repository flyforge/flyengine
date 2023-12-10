#pragma once

#include <GameEngine/GameState/GameState.h>

#include <Core/Console/ConsoleFunction.h>
#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/UniquePtr.h>

class plQuakeConsole;

// TODO: update comments below

/// \brief The base class for all typical game applications made with PlasmaEngine
///
/// While plApplication is an abstraction for the operating system entry point,
/// plGameApplication extends this to implement startup and tear down functionality
/// of a typical game that uses the standard functionality of PlasmaEngine.
///
/// plGameApplication implements a lot of functionality needed by most games,
/// such as setting up data directories, loading plugins, configuring the input system, etc.
///
/// For every such step a virtual function is called, allowing to override steps in custom applications.
///
/// The default implementation tries to do as much of this in a data-driven way. E.g. plugin and data
/// directory configurations are read from DDL files. These can be configured by hand or using PlasmaEditor.
///
/// You are NOT supposed to implement game functionality by deriving from plGameApplication.
/// Instead see plGameState.
///
/// plGameApplication will create exactly one plGameState by looping over all available plGameState types
/// (through reflection) and picking the one whose DeterminePriority function returns the highest priority.
/// That game state will live throughout the entire application life-time and will be stepped every frame.
class PLASMA_GAMEENGINE_DLL plGameApplication : public plGameApplicationBase
{
public:
  static plCVarBool cvar_AppVSync;
  static plCVarBool cvar_AppShowFPS;
  static plCVarBool cvar_AppShowInfo;

public:
  typedef plGameApplicationBase SUPER;

  /// szProjectPath may be nullptr, if FindProjectDirectory() is overridden.
  plGameApplication(const char* szAppName, const char* szProjectPath);
  ~plGameApplication();

  /// \brief Returns the plGameApplication singleton
  static plGameApplication* GetGameApplicationInstance() { return s_pGameApplicationInstance; }

  /// \brief Returns the active renderer of the current app. Either the default or overridden via -render command line flag.
  static plStringView GetActiveRenderer();

  /// \brief When the graphics device is created, by default the game application will pick a platform specific implementation. This
  /// function allows to override that by setting a custom function that creates a graphics device.
  static void SetOverrideDefaultDeviceCreator(plDelegate<plGALDevice*(const plGALDeviceCreationDescription&)> creator);

  /// \brief Implementation of plGameApplicationBase::FindProjectDirectory to define the 'project' special data directory.
  ///
  /// The default implementation will try to resolve m_sAppProjectPath to an absolute path. m_sAppProjectPath can be absolute itself,
  /// relative to ">sdk/" or relative to plOSFile::GetApplicationDirectory().
  /// m_sAppProjectPath must be set either via the plGameApplication constructor or manually set before project.
  ///
  /// Alternatively, plGameApplication::FindProjectDirectory() must be overwritten.
  virtual plString FindProjectDirectory() const override;

  /// \brief Used at runtime (by the editor) to reload input maps. Forwards to Init_ConfigureInput()
  void ReinitializeInputConfig();

  /// \brief Returns the project path that was given to the constructor (or modified by an overridden implementation).
  plStringView GetAppProjectPath() const { return m_sAppProjectPath; }

protected:
  virtual void Init_ConfigureInput() override;
  virtual void Init_ConfigureAssetManagement() override;
  virtual void Init_LoadRequiredPlugins() override;
  virtual void Init_SetupDefaultResources() override;
  virtual void Init_SetupGraphicsDevice() override;
  virtual void Deinit_ShutdownGraphicsDevice() override;

  virtual bool IsGameUpdateEnabled() const override;

  virtual bool Run_ProcessApplicationInput() override;
  virtual void Run_WorldUpdateAndRender() override;
  virtual void Run_Present() override;
  virtual void Run_FinishFrame() override;

  /// \brief Stores what is given to the constructor
  plString m_sAppProjectPath;
  bool m_bIgnoreErrors = false;

protected:
  static plGameApplication* s_pGameApplicationInstance;

  void RenderFps();
  void RenderConsole();

  void UpdateWorldsAndExtractViews();
  plSharedPtr<plDelegateTask<void>> m_pUpdateTask;

  static plDelegate<plGALDevice*(const plGALDeviceCreationDescription&)> s_DefaultDeviceCreator;

  bool m_bShowConsole = false;
  plUniquePtr<plQuakeConsole> m_pConsole;
};
