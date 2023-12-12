#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#  include <Shlobj.h>
#endif

plEvent<const plCppSettings&> plCppProject::s_ChangeEvents;

plString plCppProject::GetTargetSourceDir(plStringView sProjectDirectory /*= {}*/)
{
  plStringBuilder sTargetDir = sProjectDirectory;

  if (sTargetDir.IsEmpty())
  {
    sTargetDir = plToolsProject::GetSingleton()->GetProjectDirectory();
  }

  sTargetDir.AppendPath("CppSource");
  return sTargetDir;
}

plString plCppProject::GetGeneratorFolderName(const plCppSettings& cfg)
{
  switch (cfg.m_Compiler)
  {
    case plCppSettings::Compiler::None:
      return "";

    case plCppSettings::Compiler::Vs2019:
      return "Vs2019x64";

    case plCppSettings::Compiler::Vs2022:
      return "Vs2022x64";

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return {};
}

plString plCppProject::GetCMakeGeneratorName(const plCppSettings& cfg)
{
  switch (cfg.m_Compiler)
  {
    case plCppSettings::Compiler::None:
      return "";

    case plCppSettings::Compiler::Vs2019:
      return "Visual Studio 16 2019";

    case plCppSettings::Compiler::Vs2022:
      return "Visual Studio 17 2022";

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return {};
}

plString plCppProject::GetPluginSourceDir(const plCppSettings& cfg, plStringView sProjectDirectory /*= {}*/)
{
  plStringBuilder sDir = GetTargetSourceDir(sProjectDirectory);
  sDir.AppendPath(cfg.m_sPluginName);
  sDir.Append("Plugin");
  return sDir;
}

plString plCppProject::GetBuildDir(const plCppSettings& cfg)
{
  plStringBuilder sBuildDir;
  sBuildDir.Format("{}/Build/{}", GetTargetSourceDir(), GetGeneratorFolderName(cfg));
  return sBuildDir;
}

plString plCppProject::GetSolutionPath(const plCppSettings& cfg)
{
  plStringBuilder sSolutionFile;
  sSolutionFile = GetBuildDir(cfg);
  sSolutionFile.AppendPath(cfg.m_sPluginName);
  sSolutionFile.Append(".sln");
  return sSolutionFile;
}

plResult plCppProject::CheckCMakeCache(const plCppSettings& cfg)
{
  plStringBuilder sCacheFile;
  sCacheFile = GetBuildDir(cfg);
  sCacheFile.AppendPath("CMakeCache.txt");

  plFileReader file;
  PLASMA_SUCCEED_OR_RETURN(file.Open(sCacheFile));

  plStringBuilder content;
  content.ReadAll(file);

  const plStringView sSearchFor = "CMAKE_CONFIGURATION_TYPES:STRING="_plsv;

  const char* pConfig = content.FindSubString(sSearchFor);
  if (pConfig == nullptr)
    return PLASMA_FAILURE;

  pConfig += sSearchFor.GetElementCount();

  const char* pEndConfig = content.FindSubString("\n", pConfig);
  if (pEndConfig == nullptr)
    return PLASMA_FAILURE;

  plStringBuilder sUsedCfg;
  sUsedCfg.SetSubString_FromTo(pConfig, pEndConfig);
  sUsedCfg.Trim("\t\n\r ");

  if (sUsedCfg != BUILDSYSTEM_BUILDTYPE)
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}

bool plCppProject::ExistsSolution(const plCppSettings& cfg)
{
  return plOSFile::ExistsFile(GetSolutionPath(cfg));
}

bool plCppProject::ExistsProjectCMakeListsTxt()
{
  if (!plToolsProject::IsProjectOpen())
    return false;

  plStringBuilder sPath = GetTargetSourceDir();
  sPath.AppendPath("CMakeLists.txt");
  return plOSFile::ExistsFile(sPath);
}

plResult plCppProject::PopulateWithDefaultSources(const plCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  PLASMA_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  const plString sProjectName = cfg.m_sPluginName;

  plStringBuilder sProjectNameUpper = cfg.m_sPluginName;
  sProjectNameUpper.ToUpper();

  const plStringBuilder sTargetDir = plToolsProject::GetSingleton()->GetProjectDirectory();

  plStringBuilder sSourceDir = plApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSourceDir.AppendPath("CppProject");

  plDynamicArray<plFileStats> items;
  plOSFile::GatherAllItemsInFolder(items, sSourceDir, plFileSystemIteratorFlags::ReportFilesRecursive);

  struct FileToCopy
  {
    plString m_sSource;
    plString m_sDestination;
  };

  plHybridArray<FileToCopy, 32> filesCopied;

  // gather files
  {
    for (const auto& item : items)
    {
      plStringBuilder srcPath, dstPath;
      item.GetFullPath(srcPath);

      dstPath = srcPath;
      dstPath.MakeRelativeTo(sSourceDir).IgnoreResult();

      dstPath.ReplaceAll("CppProject", sProjectName);
      dstPath.Prepend(sTargetDir, "/");
      dstPath.MakeCleanPath();

      // don't copy files over that already exist (and may have edits)
      if (plOSFile::ExistsFile(dstPath))
      {
        // if any file already exists, don't copy non-existing (user might have deleted unwanted sample files)
        filesCopied.Clear();
        break;
      }

      auto& ftc = filesCopied.ExpandAndGetRef();
      ftc.m_sSource = srcPath;
      ftc.m_sDestination = dstPath;
    }
  }

  // Copy files
  {
    for (const auto& ftc : filesCopied)
    {
      if (plOSFile::CopyFile(ftc.m_sSource, ftc.m_sDestination).Failed())
      {
        plLog::Error("Failed to copy a file.\nSource: '{}'\nDestination: '{}'\n", ftc.m_sSource, ftc.m_sDestination);
        return PLASMA_FAILURE;
      }
    }
  }

  // Modify sources
  {
    for (const auto& filePath : filesCopied)
    {
      plStringBuilder content;

      {
        plFileReader file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          plLog::Error("Failed to open C++ project file for reading.\nSource: '{}'\n", filePath.m_sDestination);
          return PLASMA_FAILURE;
        }

        content.ReadAll(file);
      }

      content.ReplaceAll("CppProject", sProjectName);
      content.ReplaceAll("CPPPROJECT", sProjectNameUpper);

      {
        plFileWriter file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          plLog::Error("Failed to open C++ project file for writing.\nSource: '{}'\n", filePath.m_sDestination);
          return PLASMA_FAILURE;
        }

        file.WriteBytes(content.GetData(), content.GetElementCount()).IgnoreResult();
      }
    }
  }

  s_ChangeEvents.Broadcast(cfg);
  return PLASMA_SUCCESS;
}

plResult plCppProject::CleanBuildDir(const plCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  PLASMA_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  const plString sBuildDir = GetBuildDir(cfg);

  if (!plOSFile::ExistsDirectory(sBuildDir))
    return PLASMA_SUCCESS;

  return plOSFile::DeleteFolder(sBuildDir);
}

plResult plCppProject::RunCMake(const plCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  PLASMA_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  if (!ExistsProjectCMakeListsTxt())
  {
    plLog::Error("No CMakeLists.txt exists in target source directory '{}'", GetTargetSourceDir());
    return PLASMA_FAILURE;
  }

  const plString sSdkDir = plFileSystem::GetSdkRootDirectory();
  const plString sBuildDir = plCppProject::GetBuildDir(cfg);
  const plString sSolutionFile = plCppProject::GetSolutionPath(cfg);

  plStringBuilder tmp;

  QStringList args;
  args << "-S";
  args << plCppProject::GetTargetSourceDir().GetData();

  tmp.Format("-DPLASMA_SDK_DIR:PATH={}", sSdkDir);
  args << tmp.GetData();

  tmp.Format("-DPLASMA_BUILDTYPE_ONLY:STRING={}", BUILDSYSTEM_BUILDTYPE);
  args << tmp.GetData();

  args << "-G";
  args << plCppProject::GetCMakeGeneratorName(cfg).GetData();

  args << "-B";
  args << sBuildDir.GetData();

  args << "-A";
  args << "x64";

  plLogSystemToBuffer log;

  plStatus res = plQtEditorApp::GetSingleton()->ExecuteTool("cmake/bin/cmake", args, 120, &log, plLogMsgType::InfoMsg);

  if (res.Failed())
  {
    plLog::Error("Solution generation failed:\n\n{}\n{}\n", log.m_sBuffer, res.m_sMessage);
    return PLASMA_FAILURE;
  }

  if (!ExistsSolution(cfg))
  {
    plLog::Error("CMake did not generate the expected solution. Did you attempt to rename it? If so, you may need to delete the top-level CMakeLists.txt file and set up the C++ project again.");
    return PLASMA_FAILURE;
  }

  plLog::Success("Solution generated.\n\n{}\n", log.m_sBuffer);
  s_ChangeEvents.Broadcast(cfg);
  return PLASMA_SUCCESS;
}

plResult plCppProject::RunCMakeIfNecessary(const plCppSettings& cfg)
{
  if (!plCppProject::ExistsProjectCMakeListsTxt())
    return PLASMA_SUCCESS;

  if (plCppProject::ExistsSolution(cfg) && plCppProject::CheckCMakeCache(cfg).Succeeded())
    return PLASMA_SUCCESS;

  return plCppProject::RunCMake(cfg);
}

plResult plCppProject::CompileSolution(const plCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  PLASMA_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  PLASMA_LOG_BLOCK("Compile Solution");

  if (cfg.m_sMsBuildPath.IsEmpty())
  {
    FindMsBuild(cfg).IgnoreResult();

    if (cfg.m_sMsBuildPath.IsEmpty())
    {
      plLog::Error("MSBuild path is not available.");

      return PLASMA_FAILURE;
    }
  }

  if (plSystemInformation::IsDebuggerAttached())
  {
    plQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, MSBuild usually fails to compile the project.\n\nDetach the debugger now, then press OK to continue.");
  }

  plHybridArray<plString, 32> errors;

  plProcessOptions po;
  po.m_sProcess = cfg.m_sMsBuildPath;
  po.m_bHideConsoleWindow = true;
  po.m_onStdOut = [&](plStringView sText) {
    if (sText.FindSubString_NoCase("error") != nullptr)
      errors.PushBack(sText);
  };

  po.AddArgument(plCppProject::GetSolutionPath(cfg));
  po.AddArgument("/m"); // multi-threaded compilation
  po.AddArgument("/nr:false");
  po.AddArgument("/t:Build");
  po.AddArgument("/p:Configuration={}", BUILDSYSTEM_BUILDTYPE);
  po.AddArgument("/p:Platform=x64");

  plStringBuilder sMsBuildCmd;
  po.BuildCommandLineString(sMsBuildCmd);
  plLog::Dev("Running MSBuild: {}", sMsBuildCmd);

  plInt32 iReturnCode = 0;
  if (plProcess::Execute(po, &iReturnCode).Failed())
  {
    plLog::Error("MSBuild failed to run.");
    return PLASMA_FAILURE;
  }

  if (iReturnCode == 0)
  {
    plLog::Success("Compiled C++ solution.");
    return PLASMA_SUCCESS;
  }

  plLog::Error("MSBuild failed with return code {}", iReturnCode);

  for (const auto& err : errors)
  {
    plLog::Error(err);
  }

  return PLASMA_FAILURE;
}

plResult plCppProject::BuildCodeIfNecessary(const plCppSettings& cfg)
{
  PLASMA_SUCCEED_OR_RETURN(plCppProject::FindMsBuild(cfg));

  if (!plCppProject::ExistsProjectCMakeListsTxt())
    return PLASMA_SUCCESS;

  if (!plCppProject::ExistsSolution(cfg) || plCppProject::CheckCMakeCache(cfg).Failed())
  {
    PLASMA_SUCCEED_OR_RETURN(plCppProject::RunCMake(cfg));
  }

  return CompileSolution(cfg);
}

plResult plCppProject::FindMsBuild(const plCppSettings& cfg)
{
  if (!cfg.m_sMsBuildPath.IsEmpty())
    return PLASMA_SUCCESS;

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  plStringBuilder sVsWhere;

  wchar_t* pPath = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, KF_FLAG_DEFAULT, nullptr, &pPath)))
  {
    sVsWhere = plStringWChar(pPath);
    sVsWhere.AppendPath("Microsoft Visual Studio/Installer/vswhere.exe");

    CoTaskMemFree(pPath);
  }
  else
  {
    plLog::Error("Could not find the 'Program Files (x86)' folder.");
    return PLASMA_FAILURE;
  }

  plStringBuilder sStdOut;
  plProcessOptions po;
  po.m_sProcess = sVsWhere;
  po.m_bHideConsoleWindow = true;
  po.m_onStdOut = [&](plStringView sText) { sStdOut.Append(sText); };

  // TODO: search for VS2022 or VS2019 depending on cfg
  po.AddCommandLine("-latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe");

  if (plProcess::Execute(po).Failed())
  {
    plLog::Error("Executing vsWhere.exe failed. Do you have the correct version of Visual Studio installed?");
    return PLASMA_FAILURE;
  }

  sStdOut.Trim("\n\r");
  sStdOut.MakeCleanPath();

  cfg.m_sMsBuildPath = sStdOut;
  return PLASMA_SUCCESS;
#else
  return PLASMA_FAILURE;
#endif
}

void plCppProject::UpdatePluginConfig(const plCppSettings& cfg)
{
  const plStringBuilder sPluginName(cfg.m_sPluginName, "Plugin");

  plPluginBundleSet& bundles = plQtEditorApp::GetSingleton()->GetPluginBundles();

  plStringBuilder txt;
  bundles.m_Plugins.Remove(sPluginName);
  plPluginBundle& plugin = bundles.m_Plugins[sPluginName];
  plugin.m_bLoadCopy = true;
  plugin.m_bSelected = true;
  plugin.m_bMissing = true;
  plugin.m_LastModificationTime.Invalidate();
  plugin.m_ExclusiveFeatures.PushBack("ProjectPlugin");
  txt.Set("'", cfg.m_sPluginName, "' project plugin");
  plugin.m_sDisplayName = txt;
  txt.Set("C++ code for the '", cfg.m_sPluginName, "' project.");
  plugin.m_sDescription = txt;
  plugin.m_RuntimePlugins.PushBack(sPluginName);

  plQtEditorApp::GetSingleton()->WritePluginSelectionStateDDL();
}

plResult plCppProject::EnsureCppPluginReady()
{
  if (!ExistsProjectCMakeListsTxt())
    return PLASMA_SUCCESS;

  plCppSettings cppSettings;
  if (cppSettings.Load().Failed())
  {
    plQtUiServices::GetSingleton()->MessageBoxWarning(plFmt("Failed to load the C++ plugin settings."));
    return PLASMA_FAILURE;
  }

  if (plCppProject::BuildCodeIfNecessary(cppSettings).Failed())
  {
    plQtUiServices::GetSingleton()->MessageBoxWarning(plFmt("Failed to build the C++ code. See log for details."));
    return PLASMA_FAILURE;
  }

  plQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
  return PLASMA_SUCCESS;
}

bool plCppProject::IsBuildRequired()
{
  if (!ExistsProjectCMakeListsTxt())
    return false;

  plCppSettings cfg;
  if (cfg.Load().Failed())
    return false;

  if (!plCppProject::ExistsSolution(cfg))
    return true;

  if (plCppProject::CheckCMakeCache(cfg).Failed())
    return true;

  plStringBuilder sPath = plOSFile::GetApplicationDirectory();
  sPath.AppendPath(cfg.m_sPluginName);
  sPath.Append("Plugin");

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  sPath.Append(".dll");
#else
  sPath.Append(".so");
#endif

  if (!plOSFile::ExistsFile(sPath))
    return true;

  return false;
}