#pragma once

/// \file

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

/// \brief The data that is broadcast whenever a plugin is (un-) loaded.
struct plPluginEvent
{
  enum Type
  {
    BeforeLoading,          ///< Sent shortly before a new plugin is loaded.
    AfterLoadingBeforeInit, ///< Sent immediately after a new plugin has been loaded, even before it is initialized (which might trigger loading of other plugins).
    AfterLoading,           ///< Sent after a new plugin has been loaded and initialized.
    BeforeUnloading,        ///< Sent before a plugin is going to be unloaded.
    StartupShutdown,        ///< Used by the startup system for automatic shutdown.
    AfterStartupShutdown,   ///< Used by the ResourceManager to unload now unreferenced resources after the startup system shutdown is through.
    AfterUnloading,         ///< Sent after a plugin has been unloaded.
    BeforePluginChanges,    ///< Sent (once) before any (group) plugin changes (load/unload) are done.
    AfterPluginChanges,     ///< Sent (once) after all (group) plugin changes (unload/load) are finished.
  };

  Type m_EventType;                       ///< Which type of event this is.
  plStringView m_sPluginBinary;           ///< The file name of the affected plugin.
};

/// \brief Flags for loading a plugin.
struct plPluginLoadFlags
{
  using StorageType = plUInt8;

  enum Enum
  {
    LoadCopy = PL_BIT(0),         ///< Don't load a DLL directly, but create a copy of the file and load that instead. This allows to continue working on (and compiling) the DLL in parallel.
    PluginIsOptional = PL_BIT(1), ///< When an optional plugin can't be loaded (missing file usually), no error is logged. LoadPlugin() will still return PL_FAILURE though.
    CustomDependency = PL_BIT(2), ///< The plugin is an injected dependency (usually for the editor), and thus might get treated differently (this is just a tag)

    Default = 0,
  };

  struct Bits
  {
    StorageType LoadCopy : 1;
    StorageType PluginIsOptional : 1;
  };
};

using plPluginInitCallback = void (*)();

/// \brief plPlugin manages all dynamically loadable plugins.
///
/// To load a plugin, call plPlugin::LoadPlugin() with the filename of the plugin (without a path).
/// The plugin DLL has to be located next to the application binary.
///
/// It is not possible to unload individual plugins, but you can unload all plugins. Make sure to only do this when no code and data
/// from any of the plugins is still referenced somewhere.
///
/// When a plugin has a dependency on another plugin, it should contain a call to PL_PLUGIN_DEPENDENCY() in one of its cpp files.
/// This instructs the system to load that plugin as well, and makes sure to delay initialization until all (transitive) dependencies are loaded.
///
/// A plugin may contain one or multiple PL_PLUGIN_ON_LOADED() and PL_PLUGIN_ON_UNLOADED() functions.
/// These are called automatically once a plugin and all its dependencies are loaded. This can be used to make sure basic things get set up.
/// For anything more complicated, use plStartup instead. Once a plugin is loaded, the startup system will initialize new startup code properly.
class PL_FOUNDATION_DLL plPlugin
{
public:
  /// \brief Code that needs to be execute whenever a plugin is loaded or unloaded can register itself here to be notified of such events.
  static const plCopyOnBroadcastEvent<const plPluginEvent&>& Events(); // [tested]

  /// \brief Calls the PL_PLUGIN_ON_LOADED() functions for all code that is already linked into the executable at startup.
  ///
  /// If code that was meant to be loaded dynamically ends up being statically linked (e.g. on platforms where only static linking is used),
  /// the PL_PLUGIN_ON_LOADED() functions should still be called. The application can decide when the best time is.
  /// Usually a good point in time is right before the app would load the first dynamic plugin.
  /// If this function is never called manually, but plPlugin::LoadPlugin() is called, this function will be called automatically before loading the first actual plugin.
  static void InitializeStaticallyLinkedPlugins(); // [tested]

  /// \brief Call this before loading / unloading several plugins in a row, to prevent unnecessary re-initializations.
  static void BeginPluginChanges();

  /// \brief Must be called to finish what BeginPluginChanges started.
  static void EndPluginChanges();

  /// \brief Checks whether a plugin with the given name exists. Does not guarantee that the plugin could be loaded successfully.
  static bool ExistsPluginFile(plStringView sPluginFile);

  /// \brief Tries to load a DLL dynamically into the program.
  ///
  /// PL_SUCCESS is returned when the DLL is either successfully loaded or has already been loaded before.
  /// PL_FAILURE is returned if the DLL cannot be located or it could not be loaded properly.
  ///
  /// See plPluginLoadFlags for additional options.
  static plResult LoadPlugin(plStringView sPluginFile, plBitflags<plPluginLoadFlags> flags = plPluginLoadFlags::Default); // [tested]

  /// \brief Unloads all previously loaded plugins in the reverse order in which they were loaded.
  ///
  /// Also calls PL_PLUGIN_ON_UNLOADED() of all statically linked code.
  static void UnloadAllPlugins(); // [tested]

  /// \brief Sets how many tries the system will do to find a free plugin file name.
  ///
  /// During plugin loading the system may create copies of the plugin DLLs. This only works if the system can find a
  /// file to write to. If too many instances of the engine are running, no such free file name might be found and plugin loading fails.
  /// This value specifies how often the system tries to find a free file. The default is 32.
  static void SetMaxParallelInstances(plUInt32 uiMaxParallelInstances);

  /// \internal struct used by plPlugin macros
  struct PL_FOUNDATION_DLL Init
  {
    Init(plPluginInitCallback onLoadOrUnloadCB, bool bOnLoad);
    Init(const char* szAddPluginDependency);
  };

  /// \brief Contains basic information about a loaded plugin.
  struct PL_FOUNDATION_DLL PluginInfo
  {
    plString m_sName;
    plHybridArray<plString, 2> m_sDependencies;
    plBitflags<plPluginLoadFlags> m_LoadFlags;
  };

  /// \brief Returns information about all currently loaded plugins.
  static void GetAllPluginInfos(plDynamicArray<PluginInfo>& ref_infos);

  /// \internal Determines the plugin paths.
  static void GetPluginPaths(plStringView sPluginName, plStringBuilder& ref_sOriginalFile, plStringBuilder& ref_sCopiedFile, plUInt8 uiFileCopyNumber);

  /// \internal determines if a plugin copy is required for hot reloading for plugin code
  static bool PlatformNeedsPluginCopy();

private:
  plPlugin() = delete;
};

/// \brief Adds a dependency on another plugin to the plugin in which this call is located.
///
/// If Plugin2 requires Plugin1 to be loaded when Plugin2 is used, insert this into a CPP file of Plugin2:\n
/// PL_PLUGIN_DEPENDENCY(Plugin1);
///
/// That instructs the plPlugin system to make sure that Plugin1 gets loaded and initialized before Plugin2 is initialized.
#define PL_PLUGIN_DEPENDENCY(PluginName) \
  plPlugin::Init PL_CONCAT(PL_CONCAT(plugin_dep_, PluginName), PL_SOURCE_LINE)(PL_PP_STRINGIFY(PluginName))

/// \brief Creates a function that is executed when the plugin gets loaded.
///
/// Just insert PL_PLUGIN_ON_LOADED() { /* function body */ } into a CPP file of a plugin to add a function that is called
/// right after the plugin got loaded.
/// If the plugin has depenencies (set via PL_PLUGIN_DEPENDENCY()), it is guaranteed that all ON_LOADED functions of the
/// dependencies are called first.
/// If there are multiple such functions defined within the same DLL, there is no guarantee in which order they are called.
#define PL_PLUGIN_ON_LOADED()                                \
  static void plugin_OnLoaded();                             \
  plPlugin::Init plugin_OnLoadedInit(plugin_OnLoaded, true); \
  static void plugin_OnLoaded()

/// \brief Creates a function that is executed when the plugin gets unloaded.
///
/// This is typically the case when the application shuts down.
///
/// Just insert PL_PLUGIN_ON_UNLOADED() { /* function body */ } into a CPP file of a plugin to add a function that is called
/// right before the plugin gets unloaded.
/// ON_UNLOADED function calls across DLLs are done in reverse order to the ON_LOADED function calls.
/// If there are multiple such functions defined within the same DLL, there is no guarantee in which order they are called.
#define PL_PLUGIN_ON_UNLOADED()                                   \
  static void plugin_OnUnloaded();                                \
  plPlugin::Init plugin_OnUnloadedInit(plugin_OnUnloaded, false); \
  static void plugin_OnUnloaded()
