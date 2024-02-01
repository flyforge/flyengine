#include <Core/CorePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/WorldModuleConfig.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Utilities/CommandLineOptions.h>

plCommandLineOptionBool opt_DisableConsoleOutput("app", "-disableConsoleOutput", "Disables logging to the standard console window.", false);
plCommandLineOptionInt opt_TelemetryPort("app", "-TelemetryPort", "The network port over which telemetry is sent.", plTelemetry::s_uiPort);
plCommandLineOptionString opt_Profile("app", "-profile", "The platform profile to use.", "PC");

plString plGameApplicationBase::GetBaseDataDirectoryPath() const
{
  return ">sdk/Data/Base";
}

plString plGameApplicationBase::GetProjectDataDirectoryPath() const
{
  return ">project/";
}

void plGameApplicationBase::ExecuteInitFunctions()
{
  Init_PlatformProfile_SetPreferred();
  Init_ConfigureTelemetry();
  Init_FileSystem_SetSpecialDirs();
  Init_LoadRequiredPlugins();
  Init_ConfigureAssetManagement();
  Init_FileSystem_ConfigureDataDirs();
  Init_LoadWorldModuleConfig();
  Init_LoadProjectPlugins();
  Init_PlatformProfile_LoadForRuntime();
  Init_ConfigureInput();
  Init_ConfigureTags();
  Init_ConfigureCVars();
  Init_SetupGraphicsDevice();
  Init_SetupDefaultResources();
}

void plGameApplicationBase::Init_PlatformProfile_SetPreferred()
{
  m_PlatformProfile.SetConfigName(opt_Profile.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified));
  m_PlatformProfile.AddMissingConfigs();
}

void plGameApplicationBase::BaseInit_ConfigureLogging()
{
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  plGlobalLog::RemoveLogWriter(m_LogToConsoleID);
  plGlobalLog::RemoveLogWriter(m_LogToVsID);

  if (!opt_DisableConsoleOutput.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    m_LogToConsoleID = plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
  }

  m_LogToVsID = plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
#endif
}

void plGameApplicationBase::Init_ConfigureTelemetry()
{
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  plTelemetry::s_uiPort = static_cast<plUInt16>(opt_TelemetryPort.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified));
  plTelemetry::SetServerName(GetApplicationName());
  plTelemetry::CreateServer();
#endif
}

void plGameApplicationBase::Init_FileSystem_SetSpecialDirs()
{
  plFileSystem::SetSpecialDirectory("project", FindProjectDirectory());
}

void plGameApplicationBase::Init_ConfigureAssetManagement() {}

void plGameApplicationBase::Init_LoadRequiredPlugins()
{
  plPlugin::InitializeStaticallyLinkedPlugins();

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
  plPlugin::LoadPlugin("XBoxControllerPlugin", plPluginLoadFlags::PluginIsOptional).IgnoreResult();
#endif
}

void plGameApplicationBase::Init_FileSystem_ConfigureDataDirs()
{
  // ">appdir/" and ">user/" are built-in special directories
  // see plFileSystem::ResolveSpecialDirectory

  const plStringBuilder sUserDataPath(">user/", GetApplicationName());

  plFileSystem::CreateDirectoryStructure(sUserDataPath).IgnoreResult();

  plString writableBinRoot = ">appdir/";
  plString shaderCacheRoot = ">appdir/";

#if PL_DISABLED(PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // On platforms where this is disabled, one can usually only write to the user directory
  // e.g. on UWP and mobile platforms
  writableBinRoot = sUserDataPath;
  shaderCacheRoot = sUserDataPath;
#endif

  // for absolute paths, read-only
  plFileSystem::AddDataDirectory("", "GameApplicationBase", ":", plFileSystem::ReadOnly).IgnoreResult();

  // ":bin/" : writing to the binary directory
  plFileSystem::AddDataDirectory(writableBinRoot, "GameApplicationBase", "bin", plFileSystem::AllowWrites).IgnoreResult();

  // ":shadercache/" for reading and writing shader files
  plFileSystem::AddDataDirectory(shaderCacheRoot, "GameApplicationBase", "shadercache", plFileSystem::AllowWrites).IgnoreResult();

  // ":appdata/" for reading and writing app user data
  plFileSystem::AddDataDirectory(sUserDataPath, "GameApplicationBase", "appdata", plFileSystem::AllowWrites).IgnoreResult();

  // ":base/" for reading the core engine files
  plFileSystem::AddDataDirectory(GetBaseDataDirectoryPath(), "GameApplicationBase", "base", plFileSystem::DataDirUsage::ReadOnly).IgnoreResult();

  // ":project/" for reading the project specific files
  plFileSystem::AddDataDirectory(GetProjectDataDirectoryPath(), "GameApplicationBase", "project", plFileSystem::DataDirUsage::ReadOnly).IgnoreResult();

  // ":plugins/" for plugin specific data (optional, if it exists)
  {
    plStringBuilder dir;
    plFileSystem::ResolveSpecialDirectory(">sdk/Data/Plugins", dir).IgnoreResult();
    if (plOSFile::ExistsDirectory(dir))
    {
      plFileSystem::AddDataDirectory(">sdk/Data/Plugins", "GameApplicationBase", "plugins", plFileSystem::DataDirUsage::ReadOnly).IgnoreResult();
    }
  }

  {
    plApplicationFileSystemConfig appFileSystemConfig;
    appFileSystemConfig.Load();

    // get rid of duplicates that we already hard-coded above
    for (plUInt32 i = appFileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
    {
      const plString name = appFileSystemConfig.m_DataDirs[i - 1].m_sRootName;
      if (name.IsEqual_NoCase(":") || name.IsEqual_NoCase("bin") || name.IsEqual_NoCase("shadercache") || name.IsEqual_NoCase("appdata") || name.IsEqual_NoCase("base") || name.IsEqual_NoCase("project") || name.IsEqual_NoCase("plugins"))
      {
        appFileSystemConfig.m_DataDirs.RemoveAtAndCopy(i - 1);
      }
    }

    appFileSystemConfig.Apply();
  }
}

void plGameApplicationBase::Init_LoadWorldModuleConfig()
{
  plWorldModuleConfig worldModuleConfig;
  worldModuleConfig.Load();
  worldModuleConfig.Apply();
}

void plGameApplicationBase::Init_LoadProjectPlugins()
{
  plApplicationPluginConfig appPluginConfig;
  appPluginConfig.Load();
  appPluginConfig.Apply();
}

void plGameApplicationBase::Init_PlatformProfile_LoadForRuntime()
{
  const plStringBuilder sRuntimeProfileFile(":project/RuntimeConfigs/", m_PlatformProfile.GetConfigName(), ".plProfile");
  m_PlatformProfile.AddMissingConfigs();
  m_PlatformProfile.LoadForRuntime(sRuntimeProfileFile).IgnoreResult();
}

void plGameApplicationBase::Init_ConfigureInput() {}

void plGameApplicationBase::Init_ConfigureTags()
{
  PL_LOG_BLOCK("Reading Tags", "Tags.ddl");

  plStringView sFile = ":project/RuntimeConfigs/Tags.ddl";

#if PL_ENABLED(PL_MIGRATE_RUNTIMECONFIGS)
  sFile = plFileSystem::MigrateFileLocation(":project/Tags.ddl", sFile);
#endif

  plFileReader file;
  if (file.Open(sFile).Failed())
  {
    plLog::Dev("'{}' does not exist", sFile);
    return;
  }

  plStringBuilder tmp;

  plOpenDdlReader reader;
  if (reader.ParseDocument(file).Failed())
  {
    plLog::Error("Failed to parse DDL data in tags file");
    return;
  }

  const plOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const plOpenDdlReaderElement* pTags = pRoot->GetFirstChild(); pTags != nullptr; pTags = pTags->GetSibling())
  {
    if (!pTags->IsCustomType("Tag"))
      continue;

    const plOpenDdlReaderElement* pName = pTags->FindChildOfType(plOpenDdlPrimitiveType::String, "Name");

    if (!pName)
    {
      plLog::Error("Incomplete tag declaration!");
      continue;
    }

    tmp = pName->GetPrimitivesString()[0];
    plTagRegistry::GetGlobalRegistry().RegisterTag(tmp);
  }
}

void plGameApplicationBase::Init_ConfigureCVars()
{
  plCVar::SetStorageFolder(":appdata/CVars");
  plCVar::LoadCVars();
}

void plGameApplicationBase::Init_SetupDefaultResources()
{
  // continuously unload resources that are not in use anymore
  plResourceManager::SetAutoFreeUnused(plTime::MakeFromMicroseconds(100), plTime::MakeFromSeconds(10.0f));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void plGameApplicationBase::Deinit_UnloadPlugins()
{
  plPlugin::UnloadAllPlugins();
}

void plGameApplicationBase::Deinit_ShutdownLogging()
{
#if PL_DISABLED(PL_COMPILE_FOR_DEVELOPMENT)
  // during development, keep these loggers active
  plGlobalLog::RemoveLogWriter(m_LogToConsoleID);
  plGlobalLog::RemoveLogWriter(m_LogToVsID);
#endif
}
