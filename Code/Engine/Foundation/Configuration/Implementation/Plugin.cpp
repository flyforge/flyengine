#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
#  include <Foundation/Configuration/Implementation/Win/Plugin_Win.h>
#elif PL_ENABLED(PL_PLATFORM_OSX) || PL_ENABLED(PL_PLATFORM_LINUX)
#  include <Foundation/Configuration/Implementation/Posix/Plugin_Posix.h>
#elif PL_ENABLED(PL_PLATFORM_ANDROID)
#  include <Foundation/Configuration/Implementation/Android/Plugin_Android.h>
#else
#  error "Plugins not implemented on this Platform."
#endif

plResult UnloadPluginModule(plPluginModule& ref_pModule, plStringView sPluginFile);
plResult LoadPluginModule(plStringView sFileToLoad, plPluginModule& ref_pModule, plStringView sPluginFile);

struct ModuleData
{
  plPluginModule m_hModule = 0;
  plUInt8 m_uiFileNumber = 0;
  bool m_bCalledOnLoad = false;
  plHybridArray<plPluginInitCallback, 2> m_OnLoadCB;
  plHybridArray<plPluginInitCallback, 2> m_OnUnloadCB;
  plHybridArray<plString, 2> m_sPluginDependencies;
  plBitflags<plPluginLoadFlags> m_LoadFlags;

  void Initialize();
  void Uninitialize();
};

static ModuleData g_StaticModule;
static ModuleData* g_pCurrentlyLoadingModule = nullptr;
static plMap<plString, ModuleData> g_LoadedModules;
static plDynamicArray<plString> s_PluginLoadOrder;
static plUInt32 s_uiMaxParallelInstances = 32;
static plInt32 s_iPluginChangeRecursionCounter = 0;

plCopyOnBroadcastEvent<const plPluginEvent&> s_PluginEvents;

void plPlugin::SetMaxParallelInstances(plUInt32 uiMaxParallelInstances)
{
  s_uiMaxParallelInstances = plMath::Max(1u, uiMaxParallelInstances);
}

void plPlugin::InitializeStaticallyLinkedPlugins()
{
  g_StaticModule.Initialize();
}

void plPlugin::GetAllPluginInfos(plDynamicArray<PluginInfo>& ref_infos)
{
  ref_infos.Clear();

  ref_infos.Reserve(g_LoadedModules.GetCount());

  for (auto mod : g_LoadedModules)
  {
    auto& pi = ref_infos.ExpandAndGetRef();
    pi.m_sName = mod.Key();
    pi.m_sDependencies = mod.Value().m_sPluginDependencies;
    pi.m_LoadFlags = mod.Value().m_LoadFlags;
  }
}

void ModuleData::Initialize()
{
  if (m_bCalledOnLoad)
    return;

  m_bCalledOnLoad = true;

  for (const auto& dep : m_sPluginDependencies)
  {
    // TODO: ignore ??
    plPlugin::LoadPlugin(dep).IgnoreResult();
  }

  for (auto cb : m_OnLoadCB)
  {
    cb();
  }
}

void ModuleData::Uninitialize()
{
  if (!m_bCalledOnLoad)
    return;

  for (plUInt32 i = m_OnUnloadCB.GetCount(); i > 0; --i)
  {
    m_OnUnloadCB[i - 1]();
  }

  m_bCalledOnLoad = false;
}

void plPlugin::BeginPluginChanges()
{
  if (s_iPluginChangeRecursionCounter == 0)
  {
    plPluginEvent e;
    e.m_EventType = plPluginEvent::BeforePluginChanges;
    s_PluginEvents.Broadcast(e);
  }

  ++s_iPluginChangeRecursionCounter;
}

void plPlugin::EndPluginChanges()
{
  --s_iPluginChangeRecursionCounter;

  if (s_iPluginChangeRecursionCounter == 0)
  {
    plPluginEvent e;
    e.m_EventType = plPluginEvent::AfterPluginChanges;
    s_PluginEvents.Broadcast(e);
  }
}

static plResult UnloadPluginInternal(plStringView sPluginFile)
{
  auto thisMod = g_LoadedModules.Find(sPluginFile);

  if (!thisMod.IsValid())
    return PL_SUCCESS;

  plLog::Debug("Plugin to unload: \"{0}\"", sPluginFile);

  plPlugin::BeginPluginChanges();
  PL_SCOPE_EXIT(plPlugin::EndPluginChanges());

  // Broadcast event: Before unloading plugin
  {
    plPluginEvent e;
    e.m_EventType = plPluginEvent::BeforeUnloading;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // Broadcast event: Startup Shutdown
  {
    plPluginEvent e;
    e.m_EventType = plPluginEvent::StartupShutdown;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  // Broadcast event: After Startup Shutdown
  {
    plPluginEvent e;
    e.m_EventType = plPluginEvent::AfterStartupShutdown;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  thisMod.Value().Uninitialize();

  // unload the plugin module
  if (UnloadPluginModule(thisMod.Value().m_hModule, sPluginFile) == PL_FAILURE)
  {
    plLog::Error("Unloading plugin module '{}' failed.", sPluginFile);
    return PL_FAILURE;
  }

  // delete the plugin copy that we had loaded
  if(plPlugin::PlatformNeedsPluginCopy())
  {
    plStringBuilder sOriginalFile, sCopiedFile;
    plPlugin::GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, g_LoadedModules[sPluginFile].m_uiFileNumber);

    plOSFile::DeleteFile(sCopiedFile).IgnoreResult();
  }

  // Broadcast event: After unloading plugin
  {
    plPluginEvent e;
    e.m_EventType = plPluginEvent::AfterUnloading;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  plLog::Success("Plugin '{0}' is unloaded.", sPluginFile);
  g_LoadedModules.Remove(thisMod);

  return PL_SUCCESS;
}

static plResult LoadPluginInternal(plStringView sPluginFile, plBitflags<plPluginLoadFlags> flags)
{
  plUInt8 uiFileNumber = 0;

  plStringBuilder sOriginalFile, sCopiedFile;
  plPlugin::GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, uiFileNumber);

  if (!plOSFile::ExistsFile(sOriginalFile))
  {
    plLog::Error("The plugin '{0}' does not exist.", sPluginFile);
    return PL_FAILURE;
  }

  if (plPlugin::PlatformNeedsPluginCopy() && flags.IsSet(plPluginLoadFlags::LoadCopy))
  {
    // create a copy of the original plugin file
    const plUInt8 uiMaxParallelInstances = static_cast<plUInt8>(s_uiMaxParallelInstances);
    for (uiFileNumber = 0; uiFileNumber < uiMaxParallelInstances; ++uiFileNumber)
    {
      plPlugin::GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, uiFileNumber);
      if (plOSFile::CopyFile(sOriginalFile, sCopiedFile) == PL_SUCCESS)
        goto success;
    }

    plLog::Error("Could not copy the plugin file '{0}' to '{1}' (and all previous file numbers). Plugin MaxParallelInstances is set to {2}.", sOriginalFile, sCopiedFile, s_uiMaxParallelInstances);

    g_LoadedModules.Remove(sCopiedFile);
    return PL_FAILURE;
  }
  else
  {
    sCopiedFile = sOriginalFile;
  }

success:

  auto& thisMod = g_LoadedModules[sPluginFile];
  thisMod.m_uiFileNumber = uiFileNumber;
  thisMod.m_LoadFlags = flags;

  plPlugin::BeginPluginChanges();
  PL_SCOPE_EXIT(plPlugin::EndPluginChanges());

  // Broadcast Event: Before loading plugin
  {
    plPluginEvent e;
    e.m_EventType = plPluginEvent::BeforeLoading;
    e.m_sPluginBinary = sPluginFile;
    s_PluginEvents.Broadcast(e);
  }

  g_pCurrentlyLoadingModule = &thisMod;

  if (LoadPluginModule(sCopiedFile, g_pCurrentlyLoadingModule->m_hModule, sPluginFile) == PL_FAILURE)
  {
    // loaded, but failed
    g_pCurrentlyLoadingModule = nullptr;
    thisMod.m_hModule = 0;

    return PL_FAILURE;
  }

  g_pCurrentlyLoadingModule = nullptr;

  {
    // Broadcast Event: After loading plugin, before init
    {
      plPluginEvent e;
      e.m_EventType = plPluginEvent::AfterLoadingBeforeInit;
      e.m_sPluginBinary = sPluginFile;
      s_PluginEvents.Broadcast(e);
    }

    thisMod.Initialize();

    // Broadcast Event: After loading plugin
    {
      plPluginEvent e;
      e.m_EventType = plPluginEvent::AfterLoading;
      e.m_sPluginBinary = sPluginFile;
      s_PluginEvents.Broadcast(e);
    }
  }

  plLog::Success("Plugin '{0}' is loaded.", sPluginFile);
  return PL_SUCCESS;
}

bool plPlugin::ExistsPluginFile(plStringView sPluginFile)
{
  plStringBuilder sOriginalFile, sCopiedFile;
  GetPluginPaths(sPluginFile, sOriginalFile, sCopiedFile, 0);

  return plOSFile::ExistsFile(sOriginalFile);
}

plResult plPlugin::LoadPlugin(plStringView sPluginFile, plBitflags<plPluginLoadFlags> flags /*= plPluginLoadFlags::Default*/)
{
  if (flags.IsSet(plPluginLoadFlags::PluginIsOptional))
  {
    // early out without logging an error

    if (!ExistsPluginFile(sPluginFile))
      return PL_FAILURE;
  }

  PL_LOG_BLOCK("Loading Plugin", sPluginFile);

  if (g_LoadedModules.Find(sPluginFile).IsValid())
  {
    plLog::Debug("Plugin '{0}' already loaded.", sPluginFile);
    return PL_SUCCESS;
  }

  // make sure this is done first
  InitializeStaticallyLinkedPlugins();

  plLog::Debug("Plugin to load: \"{0}\"", sPluginFile);

  // make sure to use a static string pointer from now on, that stays where it is
  sPluginFile = g_LoadedModules.FindOrAdd(sPluginFile).Key();

  plResult res = LoadPluginInternal(sPluginFile, flags);

  if (res.Succeeded())
  {
    s_PluginLoadOrder.PushBack(sPluginFile);
  }
  else
  {
    // If we failed to load the plugin, it shouldn't be in the loaded modules list
    g_LoadedModules.Remove(sPluginFile);
  }

  return res;
}

void plPlugin::UnloadAllPlugins()
{
  BeginPluginChanges();
  PL_SCOPE_EXIT(EndPluginChanges());

  for (plUInt32 i = s_PluginLoadOrder.GetCount(); i > 0; --i)
  {
    if (UnloadPluginInternal(s_PluginLoadOrder[i - 1]).Failed())
    {
      // not sure what to do
    }
  }

  PL_ASSERT_DEBUG(g_LoadedModules.IsEmpty(), "Not all plugins were unloaded somehow.");

  for (auto mod : g_LoadedModules)
  {
    mod.Value().Uninitialize();
  }

  // also shut down all plugin objects that are statically linked
  g_StaticModule.Uninitialize();

  s_PluginLoadOrder.Clear();
  g_LoadedModules.Clear();
}

const plCopyOnBroadcastEvent<const plPluginEvent&>& plPlugin::Events()
{
  return s_PluginEvents;
}

plPlugin::Init::Init(plPluginInitCallback onLoadOrUnloadCB, bool bOnLoad)
{
  ModuleData* pMD = g_pCurrentlyLoadingModule ? g_pCurrentlyLoadingModule : &g_StaticModule;

  if (bOnLoad)
    pMD->m_OnLoadCB.PushBack(onLoadOrUnloadCB);
  else
    pMD->m_OnUnloadCB.PushBack(onLoadOrUnloadCB);
}

plPlugin::Init::Init(const char* szAddPluginDependency)
{
  ModuleData* pMD = g_pCurrentlyLoadingModule ? g_pCurrentlyLoadingModule : &g_StaticModule;

  pMD->m_sPluginDependencies.PushBack(szAddPluginDependency);
}


