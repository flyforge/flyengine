#pragma once

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/Containers/Deque.h>

#define PL_GLOBALEVENT_STARTUP_CORESYSTEMS_BEGIN "plStartup_StartupCoreSystems_Begin"
#define PL_GLOBALEVENT_STARTUP_CORESYSTEMS_END "plStartup_StartupCoreSystems_End"
#define PL_GLOBALEVENT_SHUTDOWN_CORESYSTEMS_BEGIN "plStartup_ShutdownCoreSystems_Begin"
#define PL_GLOBALEVENT_SHUTDOWN_CORESYSTEMS_END "plStartup_ShutdownCoreSystems_End"

#define PL_GLOBALEVENT_STARTUP_HIGHLEVELSYSTEMS_BEGIN "plStartup_StartupHighLevelSystems_Begin"
#define PL_GLOBALEVENT_STARTUP_HIGHLEVELSYSTEMS_END "plStartup_StartupHighLevelSystems_End"
#define PL_GLOBALEVENT_SHUTDOWN_HIGHLEVELSYSTEMS_BEGIN "plStartup_ShutdownHighLevelSystems_Begin"
#define PL_GLOBALEVENT_SHUTDOWN_HIGHLEVELSYSTEMS_END "plStartup_ShutdownHighLevelSystems_End"

#define PL_GLOBALEVENT_UNLOAD_PLUGIN_BEGIN "plStartup_UnloadPlugin_Begin"
#define PL_GLOBALEVENT_UNLOAD_PLUGIN_END "plStartup_UnloadPlugin_End"

/// \brief The startup system makes sure to initialize and shut down all known subsystems in the proper order.
///
/// Each subsystem can define on which other subsystems (or entire group) it is dependent (i.e. which other code it needs in an initialized
/// state, before it can run itself). The startup system will sort all subsystems by their dependencies and then initialize
/// them in the proper order.
/// The startup and shutdown sequence consists of two steps. First the 'core' functionality is initialized. This is usually
/// all the functionality that does not depend on a working rendering context.
/// After all systems have had their 'core' functionality initialized, the 'high level' functionality can be initialized.
/// In between these steps the rendering context should be created.
/// Tools that might not create a window or do not want to actually load GPU resources, might get away with only doing
/// the 'core' initialization. Thus a subsystem should do all the initialization that is independent from a window or rendering
/// context in 'core' startup, and it should be able to work (with some features disabled), even when 'high level' startup is not done.
///
/// A subsystem startup configuration for a static subsystem needs to be put in some cpp file of the subsystem and looks like this:
///
/// // clang-format off
/// PL_BEGIN_SUBSYSTEM_DECLARATION(ExampleGroup, ExampleSubSystem)
///
///   BEGIN_SUBSYSTEM_DEPENDENCIES
///     "SomeOtherSubSystem",
///     "SomeOtherSubSystem2",
///     "SomeGroup"
///   END_SUBSYSTEM_DEPENDENCIES
///
///   ON_CORESYSTEMS_STARTUP
///   {
///     plExampleSubSystem::BasicStartup();
///   }
///
///   ON_CORESYSTEMS_SHUTDOWN
///   {
///     plExampleSubSystem::BasicShutdown();
///   }
///
///   ON_HIGHLEVELSYSTEMS_STARTUP
///   {
///     plExampleSubSystem::EngineStartup();
///   }
///
///   ON_HIGHLEVELSYSTEMS_SHUTDOWN
///   {
///     plExampleSubSystem::EngineShutdown();
///   }
///
/// PL_END_SUBSYSTEM_DECLARATION;
/// // clang-format on
///
/// This will automatically register the subsystem, once the code is being loaded (can be dynamically loaded from a DLL).
/// The next time any of the plStartup functions are called (StartupCoreSystems, StartupHighLevelSystems) the subsystem will be initialized.
///
/// If however your subsystem is implemented as a normal class, you need to derive from the base class 'plSubSystem' and
/// override the virtual functions. Then when you have an instance of that class and call plStartup::StartupCore etc., that
/// instance will be properly initialized as well. However, you must ensure that the subsystem is properly shut down, before
/// its instance is destroyed. Also you should never have two instances of the same subsystem.
///
/// All startup / shutdown procedures broadcast global events before and after they execute.
class PL_FOUNDATION_DLL plStartup
{
public:
  // 'Base Startup' happens even before 'Core Startup', but only really low level stuff should  be done there
  // and those subsystems should not have any dependencies on each other.
  // 'Base Startup' is automatically done right before 'Core Startup'
  // There is actually no 'Base Shutdown', everything that is initialized in 'Base Startup' should not require
  // any explicit shutdown.

  /// \brief Stores the const char* as a tag. Does not copy the string, so this must be a string embedded in the application code.
  ///
  /// Before executing the startup routines an application should set tags that allow plugins to identify the context in which they are running.
  /// This makes it possible for the startup functions to conditionally configure things.
  ///
  /// Strings that should be used for common things:
  /// 'runtime' : For all applications that run the full engine, automatically added by plGameApplication. Be aware that some tool applications have
  /// this set, even though they don't use graphical output. 'editor' : for all applications that run the editor framework, set on the Editor and the
  /// EditorProcessor 'testframework' : for applications that execute the plTestFramework 'tool' : for all stand-alone tool applications, set by the
  /// editor, editorprocessor, fileserve, etc.
  static void AddApplicationTag(const char* szTag);

  /// \brief Query whether a tag was added with AddApplicationTag()
  static bool HasApplicationTag(const char* szTag);

  /// \brief Runs the 'base' startup sequence of all subsystems in the proper order.
  ///
  /// Run this, if you only require very low level systems to be initialized. Otherwise prefer StartupCore.
  /// There is NO ShutdownBaseSystems, everything that gets initialized during the 'Base Startup' should not need any deinitialization.
  /// This function is automatically called by StartupCore, if it hasn't been called before already.
  static void StartupBaseSystems() { Startup(plStartupStage::BaseSystems); }

  /// \brief Runs the 'core' startup sequence of all subsystems in the proper order.
  ///
  /// Run this BEFORE any window and graphics context have been created.
  /// Broadcasts the global event PL_GLOBALEVENT_STARTUP_CORESYSTEMS_BEGIN and PL_GLOBALEVENT_STARTUP_CORESYSTEMS_END
  static void StartupCoreSystems() { Startup(plStartupStage::CoreSystems); }

  /// \brief Runs the 'core' shutdown sequence of all subsystems in the proper order (reversed startup order).
  ///
  /// Call this AFTER window and graphics context have been destroyed already, shortly before application exit.
  /// Makes sure that the 'high level' shutdown has been run first.
  /// Broadcasts the global event PL_GLOBALEVENT_SHUTDOWN_CORESYSTEMS_BEGIN and PL_GLOBALEVENT_SHUTDOWN_CORESYSTEMS_END
  static void ShutdownCoreSystems() { Shutdown(plStartupStage::CoreSystems); }

  /// \brief Runs the 'high level' startup sequence of all subsystems in the proper order.
  ///
  /// Run this AFTER a window and graphics context have been created, such that anything that depends on that
  /// can now do its initialization.
  /// Makes sure that the 'core' initialization has been run first.
  /// Broadcasts the global event PL_GLOBALEVENT_STARTUP_HIGHLEVELSYSTEMS_BEGIN and PL_GLOBALEVENT_STARTUP_HIGHLEVELSYSTEMS_END
  static void StartupHighLevelSystems() { Startup(plStartupStage::HighLevelSystems); }

  /// \brief Runs the 'high level' shutdown sequence of all subsystems in the proper order (reversed startup order).
  ///
  /// Run this BEFORE the window and graphics context have been destroyed, such that code that requires those
  /// can do its deinitialization first.
  /// Broadcasts the global event PL_GLOBALEVENT_SHUTDOWN_HIGHLEVELSYSTEMS_BEGIN and PL_GLOBALEVENT_SHUTDOWN_HIGHLEVELSYSTEMS_END
  static void ShutdownHighLevelSystems() { Shutdown(plStartupStage::HighLevelSystems); }

  /// \brief Output info about all known subsystems via the logging system (can change when DLLs are loaded dynamically).
  static void PrintAllSubsystems();

  /// \brief Calls StartupBaseSystems(), StartupCoreSystems() or StartupHighLevelSystems() again, depending on what was done last.
  ///
  /// This can be used to first unload plugins and reload them, and then reinit the engine to the state that it was in again.
  static void ReinitToCurrentState();

private:
  /// \brief Unloads all subsystems from the given plugin AND all subsystems that directly or indirectly depend on them.
  ///
  /// This can be used to shutdown all systems from certain DLLs before that DLL is unloaded (and possibly reloaded).
  /// Broadcasts the global event PL_GLOBALEVENT_UNLOAD_PLUGIN_BEGIN and PL_GLOBALEVENT_UNLOAD_PLUGIN_END and passes szPluginName in the first event
  /// parameter.
  static void UnloadPluginSubSystems(plStringView sPluginName);

  static void PluginEventHandler(const plPluginEvent& EventData);
  static void AssignSubSystemPlugin(plStringView sPluginName);

  static void ComputeOrder(plDeque<plSubSystem*>& Order);
  static bool HasDependencyOnPlugin(plSubSystem* pSubSystem, plStringView sModule);

  static void Startup(plStartupStage::Enum stage);
  static void Shutdown(plStartupStage::Enum stage);

  static bool s_bPrintAllSubSystems;
  static plStartupStage::Enum s_CurrentState;
  static plDynamicArray<const char*> s_ApplicationTags;
};
