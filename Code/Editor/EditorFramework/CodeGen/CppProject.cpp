#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/System/Process.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#  include <Shlobj.h>
#endif

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plIDE, 1)
  PLASMA_ENUM_CONSTANT(plIDE::VisualStudioCode),
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  PLASMA_ENUM_CONSTANT(plIDE::VisualStudio),
#endif
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plCompiler, 1)
  PLASMA_ENUM_CONSTANT(plCompiler::Clang),
#if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
  PLASMA_ENUM_CONSTANT(plCompiler::Gcc),
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  PLASMA_ENUM_CONSTANT(plCompiler::Vs2022),
#endif
PLASMA_END_STATIC_REFLECTED_ENUM;

#if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
#define CPP_COMPILER_DEFAULT "g++"
#define C_COMPILER_DEFAULT "gcc"
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#define CPP_COMPILER_DEFAULT ""
#define C_COMPILER_DEFAULT ""
#else
#error Platform not implemented
#endif

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plCompilerPreferences, plNoBase, 1, plRTTIDefaultAllocator<plCompilerPreferences>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("Compiler", plCompiler, m_Compiler)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("CustomCompiler", m_bCustomCompiler)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("CppCompiler", m_sCppCompiler)->AddAttributes(new plDefaultValueAttribute(CPP_COMPILER_DEFAULT)),
    PLASMA_MEMBER_PROPERTY("CCompiler", m_sCCompiler)->AddAttributes(new plDefaultValueAttribute(C_COMPILER_DEFAULT)),
    PLASMA_MEMBER_PROPERTY("RcCompiler", m_sRcCompiler),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCppProject, 1, plRTTIDefaultAllocator<plCppProject>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("CppIDE", plIDE, m_Ide),
    PLASMA_MEMBER_PROPERTY("CompilerPreferences", m_CompilerPreferences),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plEvent<const plCppSettings&> plCppProject::s_ChangeEvents;

plDynamicArray<plCppProject::MachineSpecificCompilerPaths> plCppProject::s_MachineSpecificCompilers;

namespace
{
  static constexpr plUInt32 minGccVersion = 10;
  static constexpr plUInt32 maxGccVersion = 20;
  static constexpr plUInt32 minClangVersion = 10;
  static constexpr plUInt32 maxClangVersion = 20;

  plResult TestCompilerExecutable(plStringView sName, plString* out_pVersion = nullptr)
  {
    plStringBuilder sStdout;
    plProcessOptions po;
    po.AddArgument("--version");
    po.m_sProcess = sName;
    po.m_onStdOut = [&sStdout](plStringView out) { sStdout.Append(out); };

    if (plProcess::Execute(po).Failed())
      return PLASMA_FAILURE;

    plHybridArray<plStringView, 8> lines;
    sStdout.Split(false, lines, "\r", "\n");
    if (lines.IsEmpty())
      return PLASMA_FAILURE;

    plHybridArray<plStringView, 4> splitResult;
    lines[0].Split(false, splitResult, " ");

    if (splitResult.IsEmpty())
      return PLASMA_FAILURE;

    plStringView version = splitResult.PeekBack();
    splitResult.Clear();
    version.Split(false, splitResult, ".");
    if (splitResult.GetCount() < 3)
    {
      return PLASMA_FAILURE;
    }

    if (out_pVersion)
    {
      *out_pVersion = version;
    }

    return PLASMA_SUCCESS;
  }

  void AddCompilerVersions(plDynamicArray<plCppProject::MachineSpecificCompilerPaths>& inout_compilers, plCompiler::Enum compiler, plStringView sRequiredMajorVersion)
  {
    plStringView compilerBaseName;
    plStringView compilerBaseNameCpp;
    switch (compiler)
    {
      case plCompiler::Clang:
        compilerBaseName = "clang";
        compilerBaseNameCpp = "clang++";
        break;
#if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
      case plCompiler::Gcc:
        compilerBaseName = "gcc";
        compilerBaseNameCpp = "g++";
        break;
#endif
      default:
        PLASMA_ASSERT_NOT_IMPLEMENTED
    }


    plString compilerVersion;
    plStringBuilder requiredVersion = sRequiredMajorVersion;
    requiredVersion.Append('.');
    plStringBuilder fmt;
    if (TestCompilerExecutable(compilerBaseName, &compilerVersion).Succeeded() && TestCompilerExecutable(compilerBaseNameCpp).Succeeded() && compilerVersion.StartsWith(requiredVersion))
    {
      fmt.Format("{} (system default = {})", compilerBaseName, compilerVersion);
      inout_compilers.PushBack({fmt.GetView(), compiler, compilerBaseName, compilerBaseNameCpp, false});
    }

    plStringBuilder compilerExecutable;
    plStringBuilder compilerExecutableCpp;
    compilerExecutable.Format("{}-{}", compilerBaseName, sRequiredMajorVersion);
    compilerExecutableCpp.Format("{}-{}", compilerBaseNameCpp, sRequiredMajorVersion);
    if (TestCompilerExecutable(compilerExecutable, &compilerVersion).Succeeded() && TestCompilerExecutable(compilerExecutableCpp).Succeeded() && compilerVersion.StartsWith(requiredVersion))
    {
      fmt.Format("{} (version {})", compilerBaseName, compilerVersion);
      inout_compilers.PushBack({fmt.GetView(), compiler, compilerExecutable, compilerExecutableCpp, false});
    }
  }
} // namespace

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
  const plCppProject* preferences = plPreferences::QueryPreferences<plCppProject>();

  switch (preferences->m_CompilerPreferences.m_Compiler.GetValue())
  {
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    case plCompiler::Vs2022:
      return "Vs2022x64";
#endif
    case plCompiler::Clang:
      return "Clangx64";
#if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
    case plCompiler::Gcc:
      return "Gccx64";
#endif
  }
  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return "";
}

plString plCppProject::GetCMakeGeneratorName(const plCppSettings& cfg)
{

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  const plCppProject* preferences = plPreferences::QueryPreferences<plCppProject>();

  switch (preferences->m_CompilerPreferences.m_Compiler.GetValue())
  {
    case plCompiler::Vs2022:
      return "Visual Studio 17 2022";
    case plCompiler::Clang:
      return "Ninja";
  }
#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
  return "Ninja";
#else
#  error Platform not implemented
#endif
  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return "";
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

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  const plCppProject* preferences = plPreferences::QueryPreferences<plCppProject>();
  if (preferences->m_CompilerPreferences.m_Compiler == plCompiler::Vs2022)
  {
    sSolutionFile.AppendPath(cfg.m_sPluginName);
    sSolutionFile.Append(".sln");
    return sSolutionFile;
  }
#endif

  sSolutionFile.AppendPath("build.ninja");
  return sSolutionFile;
}

plStatus plCppProject::OpenSolution(const plCppSettings& cfg)
{
  const plCppProject* preferences = plPreferences::QueryPreferences<plCppProject>();

  switch (preferences->m_Ide.GetValue())
  {
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    case plIDE::VisualStudio:
      if (!plQtUiServices::OpenFileInDefaultProgram(plCppProject::GetSolutionPath(cfg)))
      {
        return plStatus("Opening the solution in Visual Studio failed.");
      }
      break;
#endif
    case plIDE::VisualStudioCode:
    {
      auto solutionPath = plCppProject::GetTargetSourceDir();
      QStringList args;
      args.push_back(QString::fromUtf8(solutionPath.GetData(), solutionPath.GetElementCount()));
      if (plStatus status = plQtUiServices::OpenInVsCode(args); status.Failed())
      {
        return plStatus(plFmt("Opening Visual Studio Code failed: {}", status.m_sMessage));
      }
    }
    break;
  }

  return plStatus(PLASMA_SUCCESS);
}

plStringView plCppProject::CompilerToString(plCompiler::Enum compiler)
{
  switch (compiler)
  {
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    case plCompiler::Vs2022:
      return "Vs2022";
#endif
    case plCompiler::Clang:
      return "Clang";
#if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
    case plCompiler::Gcc:
      return "Gcc";
#endif
    default:
      break;
  }
  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return "<not implemented>";
}

plCompiler::Enum plCppProject::GetSdkCompiler()
{
#if PLASMA_ENABLED(PLASMA_COMPILER_CLANG)
  return plCompiler::Clang;
#elif PLASMA_ENABLED(PLASMA_COMPILER_GCC)
  return plCompiler::Gcc;
#elif PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
  return plCompiler::Vs2022;
#else
#  error Unknown compiler
#endif
}

plString plCppProject::GetSdkCompilerMajorVersion()
{
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
  plStringBuilder fmt;
  fmt.Format("{}.{}", _MSC_VER / 100, _MSC_VER % 100);
  return fmt;
#elif PLASMA_ENABLED(PLASMA_COMPILER_CLANG)
  return PLASMA_PP_STRINGIFY(__clang_major__);
#elif PLASMA_ENABLED(PLASMA_COMPILER_GCC)
  return PLASMA_PP_STRINGIFY(__GNUC__);
#else
#  error Unsupported compiler
#endif
}

plStatus plCppProject::TestCompiler()
{
  const plCppProject* preferences = plPreferences::QueryPreferences<plCppProject>();
  if (preferences->m_CompilerPreferences.m_Compiler != GetSdkCompiler())
  {
    return plStatus(plFmt("The currently configured compiler is incompatible with this SDK. The SDK was built with '{}' but the currently configured compiler is '{}'.",
      CompilerToString(GetSdkCompiler()),
      CompilerToString(preferences->m_CompilerPreferences.m_Compiler)));
  }

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  // As CMake is selecting the compiler it is hard to do a version check, for now just assume they are compatible.
  if (GetSdkCompiler() == plCompiler::Vs2022)
  {
    return plStatus(PLASMA_SUCCESS);
  }

  if (GetSdkCompiler() == plCompiler::Clang)
  {
    if (!plOSFile::ExistsFile(preferences->m_CompilerPreferences.m_sRcCompiler))
    {
      return plStatus(plFmt("The selected RC compiler '{}' does not exist on disk.", preferences->m_CompilerPreferences.m_sRcCompiler));
    }
  }
#endif

  plString cCompilerVersion, cppCompilerVersion;
  if (TestCompilerExecutable(preferences->m_CompilerPreferences.m_sCCompiler, &cCompilerVersion).Failed())
  {
    return plStatus("The selected C Compiler doesn't work or doesn't exist.");
  }
  if (TestCompilerExecutable(preferences->m_CompilerPreferences.m_sCppCompiler, &cppCompilerVersion).Failed())
  {
    return plStatus("The selected C++ Compiler doesn't work or doesn't exist.");
  }

  plStringBuilder sdkCompilerMajorVersion = GetSdkCompilerMajorVersion();
  sdkCompilerMajorVersion.Append('.');
  if (!cCompilerVersion.StartsWith(sdkCompilerMajorVersion))
  {
    return plStatus(plFmt("The selected C Compiler has an incompatible version. The SDK was built with version {} but the compiler has version {}.", GetSdkCompilerMajorVersion(), cCompilerVersion));
  }
  if (!cppCompilerVersion.StartsWith(sdkCompilerMajorVersion))
  {
    return plStatus(plFmt("The selected C++ Compiler has an incompatible version. The SDK was built with version {} but the compiler has version {}.", GetSdkCompilerMajorVersion(), cppCompilerVersion));
  }

  return plStatus(PLASMA_SUCCESS);
}

const char* plCppProject::GetCMakePath()
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  return "cmake/bin/cmake";
#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
  return "cmake";
#else
#  error Platform not implemented
#endif
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

plCppProject::ModifyResult plCppProject::CheckCMakeUserPresets(const plCppSettings& cfg, bool bWriteResult)
{
  plStringBuilder configureJsonPath = plCppProject::GetPluginSourceDir(cfg).GetFileDirectory();
  configureJsonPath.AppendPath("CMakeUserPresets.json");

  if (plOSFile::ExistsFile(configureJsonPath))
  {
    plFileReader fileReader;
    if (fileReader.Open(configureJsonPath).Failed())
    {
      plLog::Error("Failed to open '{}' for reading", configureJsonPath);
      return ModifyResult::FAILURE;
    }
    plJSONReader reader;
    if (reader.Parse(fileReader).Failed())
    {
      plLog::Error("Failed to parse JSON of '{}'", configureJsonPath);
      return ModifyResult::FAILURE;
    }
    fileReader.Close();

    // if (reader.GetTopLevelElementType() != plJSONReader::ElementType::Dictionary)
    // {
    //   plLog::Error("Top level element of '{}' is expected to be a dictionary. Please manually fix, rename or delete the file.", configureJsonPath);
    //   return ModifyResult::FAILURE;
    // }

    plVariantDictionary json = reader.GetTopLevelObject();
    auto modifyResult = ModifyCMakeUserPresetsJson(cfg, json);
    if (modifyResult == ModifyResult::FAILURE)
    {
      plLog::Error("Failed to modify '{}' in place. Please manually fix, rename or delete the file.", configureJsonPath);
      return ModifyResult::FAILURE;
    }

    if (bWriteResult && modifyResult == ModifyResult::MODIFIED)
    {
      plStandardJSONWriter jsonWriter;
      plDeferredFileWriter fileWriter;
      fileWriter.SetOutput(configureJsonPath);
      jsonWriter.SetOutputStream(&fileWriter);

      jsonWriter.WriteVariant(plVariant(json));
      if (fileWriter.Close().Failed())
      {
        plLog::Error("Failed to write CMakeUserPresets.json to '{}'", configureJsonPath);
        return ModifyResult::FAILURE;
      }
    }

    return modifyResult;
  }
  else
  {
    if (bWriteResult)
    {
      plStandardJSONWriter jsonWriter;
      plDeferredFileWriter fileWriter;
      fileWriter.SetOutput(configureJsonPath);
      jsonWriter.SetOutputStream(&fileWriter);

      jsonWriter.WriteVariant(plVariant(CreateEmptyCMakeUserPresetsJson(cfg)));
      if (fileWriter.Close().Failed())
      {
        plLog::Error("Failed to write CMakeUserPresets.json to '{}'", configureJsonPath);
        return ModifyResult::FAILURE;
      }
    }
  }

  return ModifyResult::MODIFIED;
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

  if (auto compilerWorking = TestCompiler(); compilerWorking.Failed())
  {
    compilerWorking.LogFailure();
    return PLASMA_FAILURE;
  }

  if (CheckCMakeUserPresets(cfg, true) == ModifyResult::FAILURE)
  {
    return PLASMA_FAILURE;
  }

  plStringBuilder tmp;

  QStringList args;
  args << "--preset";
  args << "plEngine";

  plLogSystemToBuffer log;


  const plString sTargetSourceDir = plCppProject::GetTargetSourceDir();

  plStatus res = plQtEditorApp::GetSingleton()->ExecuteTool(GetCMakePath(), args, 120, &log, plLogMsgType::InfoMsg, sTargetSourceDir);

  if (res.Failed())
  {
    plLog::Error("CMake generation failed:\n\n{}\n{}\n", log.m_sBuffer, res.m_sMessage);
    return PLASMA_FAILURE;
  }

  if (!ExistsSolution(cfg))
  {
    plLog::Error("CMake did not generate the expected output. Did you attempt to rename it? If so, you may need to delete the top-level CMakeLists.txt file and set up the C++ project again.");
    return PLASMA_FAILURE;
  }

  plLog::Success("CMake generation successful.\n\n{}\n", log.m_sBuffer);
  s_ChangeEvents.Broadcast(cfg);
  return PLASMA_SUCCESS;
}

plResult plCppProject::RunCMakeIfNecessary(const plCppSettings& cfg)
{
  if (!plCppProject::ExistsProjectCMakeListsTxt())
    return PLASMA_SUCCESS;

  auto userPresetResult = CheckCMakeUserPresets(cfg, false);
  if (userPresetResult == ModifyResult::FAILURE)
  {
    return PLASMA_FAILURE;
  }

  if (plCppProject::ExistsSolution(cfg) && plCppProject::CheckCMakeCache(cfg).Succeeded() && userPresetResult == ModifyResult::NOT_MODIFIED)
    return PLASMA_SUCCESS;

  return plCppProject::RunCMake(cfg);
}

plResult plCppProject::CompileSolution(const plCppSettings& cfg)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  PLASMA_SCOPE_EXIT(QApplication::restoreOverrideCursor());

  PLASMA_LOG_BLOCK("Compile C++ Plugin");

  plHybridArray<plString, 32> errors;
  plInt32 iReturnCode = 0;
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  if (plSystemInformation::IsDebuggerAttached())
  {
    plQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, MSBuild usually fails to compile the project.\n\nDetach the debugger now, then press OK to continue.");
  }
#endif

  plProcessOptions po;

  plString cmakePath = plQtEditorApp::GetSingleton()->FindToolApplication(plCppProject::GetCMakePath());

  po.m_sProcess = cmakePath;
  po.AddArgument("--build");
  po.AddArgument(GetBuildDir(cfg));
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  const plCppProject* preferences = plPreferences::QueryPreferences<plCppProject>();

  if (preferences->m_CompilerPreferences.m_Compiler == plCompiler::Vs2022)
  {
    po.AddArgument("--config");
    po.AddArgument(BUILDSYSTEM_BUILDTYPE);
  }
#endif
  po.m_sWorkingDirectory = GetBuildDir(cfg);
  po.m_bHideConsoleWindow = true;
  po.m_onStdOut = [&](plStringView sText) {
    if (sText.FindSubString_NoCase("error") != nullptr)
      errors.PushBack(sText);
  };
  po.m_onStdError = [&](plStringView sText) {
    if (sText.FindSubString_NoCase("error") != nullptr)
      errors.PushBack(sText);
  };

  plStringBuilder sCMakeBuildCmd;
  po.BuildCommandLineString(sCMakeBuildCmd);
  plLog::Dev("Running {} {}", cmakePath, sCMakeBuildCmd);
  if (plProcess::Execute(po, &iReturnCode).Failed())
  {
    plLog::Error("Failed to start CMake.");
    return PLASMA_FAILURE;
  }

  if (iReturnCode == 0)
  {
    plLog::Success("Compiled C++ code.");
    return PLASMA_SUCCESS;
  }

  plLog::Error("CMake --build failed with return code {}", iReturnCode);

  for (const auto& err : errors)
  {
    plLog::Error(err);
  }

  return PLASMA_FAILURE;
}

plResult plCppProject::BuildCodeIfNecessary(const plCppSettings& cfg)
{
  if (!plCppProject::ExistsProjectCMakeListsTxt())
    return PLASMA_SUCCESS;

  if (!plCppProject::ExistsSolution(cfg) || plCppProject::CheckCMakeCache(cfg).Failed())
  {
    PLASMA_SUCCEED_OR_RETURN(plCppProject::RunCMake(cfg));
  }

  return CompileSolution(cfg);
}

plVariantDictionary plCppProject::CreateEmptyCMakeUserPresetsJson(const plCppSettings& cfg)
{
  plVariantDictionary json;
  json.Insert("version", 3);

  {
    plVariantDictionary cmakeMinimumRequired;
    cmakeMinimumRequired.Insert("major", 3);
    cmakeMinimumRequired.Insert("minor", 21);
    cmakeMinimumRequired.Insert("patch", 0);

    json.Insert("cmakeMinimumRequired", std::move(cmakeMinimumRequired));
  }

  {
    plVariantArray configurePresets;
    plVariantDictionary plEnginePreset;
    plEnginePreset.Insert("name", "plEngine");
    plEnginePreset.Insert("displayName", "Build the plEngine Plugin");

    {
      plVariantDictionary cacheVariables;
      plEnginePreset.Insert("cacheVariables", std::move(cacheVariables));
    }

    configurePresets.PushBack(std::move(plEnginePreset));
    json.Insert("configurePresets", std::move(configurePresets));
  }

  {
    plVariantArray buildPresets;
    {
      plVariantDictionary plEngineBuildPreset;
      plEngineBuildPreset.Insert("name", "plEngine");
      plEngineBuildPreset.Insert("configurePreset", "plEngine");
      buildPresets.PushBack(std::move(plEngineBuildPreset));
    }
    json.Insert("buildPresets", std::move(buildPresets));
  }

  PLASMA_VERIFY(ModifyCMakeUserPresetsJson(cfg, json) == ModifyResult::MODIFIED, "Freshly created user presets file should always be modified");

  return json;
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

namespace
{
  template <typename T>
  T* Expect(plVariantDictionary& inout_json, plStringView sName)
  {
    plVariant* var = nullptr;
    if (inout_json.TryGetValue(sName, var) && var->IsA<T>())
    {
      return &var->GetWritable<T>();
    }
    return nullptr;
  }

  void Modify(plVariantDictionary& inout_json, plStringView sName, plStringView sValue, plCppProject::ModifyResult& inout_modified)
  {
    plVariant* currentValue = nullptr;
    if (inout_json.TryGetValue(sName, currentValue) && currentValue->IsA<plString>() && currentValue->Get<plString>() == sValue)
      return;

    inout_json[sName] = sValue;
    inout_modified = plCppProject::ModifyResult::MODIFIED;
  }
} // namespace

plCppProject::ModifyResult plCppProject::ModifyCMakeUserPresetsJson(const plCppSettings& cfg, plVariantDictionary& inout_json)
{
  auto result = ModifyResult::NOT_MODIFIED;
  auto configurePresets = Expect<plVariantArray>(inout_json, "configurePresets");
  if (!configurePresets)
    return ModifyResult::FAILURE;

  const plCppProject* preferences = plPreferences::QueryPreferences<plCppProject>();

  for (auto& preset : *configurePresets)
  {
    if (!preset.IsA<plVariantDictionary>())
      continue;

    auto& presetDict = preset.GetWritable<plVariantDictionary>();

    auto name = Expect<plString>(presetDict, "name");
    if (!name || *name != "plEngine")
    {
      continue;
    }

    auto cacheVariables = Expect<plVariantDictionary>(presetDict, "cacheVariables");
    if (!cacheVariables)
      return ModifyResult::FAILURE;

    Modify(*cacheVariables, "PLASMA_SDK_DIR", plFileSystem::GetSdkRootDirectory(), result);
    Modify(*cacheVariables, "PLASMA_BUILDTYPE_ONLY", BUILDSYSTEM_BUILDTYPE, result);
    Modify(*cacheVariables, "CMAKE_BUILD_TYPE", BUILDSYSTEM_BUILDTYPE, result);

    bool needsCompilerPaths = true;
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    if (preferences->m_CompilerPreferences.m_Compiler == plCompiler::Vs2022)
    {
      needsCompilerPaths = false;
    }
#endif

    if (needsCompilerPaths)
    {
      Modify(*cacheVariables, "CMAKE_C_COMPILER", preferences->m_CompilerPreferences.m_sCCompiler, result);
      Modify(*cacheVariables, "CMAKE_CXX_COMPILER", preferences->m_CompilerPreferences.m_sCppCompiler, result);
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
      Modify(*cacheVariables, "CMAKE_RC_COMPILER", preferences->m_CompilerPreferences.m_sRcCompiler, result);
      Modify(*cacheVariables, "CMAKE_RC_COMPILER_INIT", "rc", result);
#endif
    }
    else
    {
      cacheVariables->Remove("CMAKE_C_COMPILER");
      cacheVariables->Remove("CMAKE_CXX_COMPILER");
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
      cacheVariables->Remove("CMAKE_RC_COMPILER");
      cacheVariables->Remove("CMAKE_RC_COMPILER_INIT");
#endif
    }

    Modify(presetDict, "generator", GetCMakeGeneratorName(cfg), result);
    Modify(presetDict, "binaryDir", GetBuildDir(cfg), result);
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    if (preferences->m_CompilerPreferences.m_Compiler == plCompiler::Vs2022)
    {
      Modify(presetDict, "architecture", "x64", result);
    }
    else
    {
      presetDict.Remove("architecture");
    }
#endif
  }

  return result;
}

plCppProject::plCppProject()
  : plPreferences(plPreferences::Domain::Application, "C++ Projects")
{
}
void plCppProject::LoadPreferences()
{
  PLASMA_PROFILE_SCOPE("Preferences");
  auto preferences = plPreferences::QueryPreferences<plCppProject>();

  plCompiler::Enum sdkCompiler = GetSdkCompiler();

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  if (sdkCompiler == plCompiler::Vs2022)
  {
    s_MachineSpecificCompilers.PushBack({"Visual Studio 2022 (system default)", plCompiler::Vs2022, "", "", false});
  }

#  if PLASMA_ENABLED(PLASMA_COMPILER_CLANG)
  // if the rcCompiler path is empty or points to a non existant file, try to autodetect it
  if ((preferences->m_CompilerPreferences.m_sRcCompiler.IsEmpty() || !plOSFile::ExistsFile(preferences->m_CompilerPreferences.m_sRcCompiler)))
  {
    plStringBuilder rcPath;
    HKEY hInstalledRoots = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots", 0, KEY_READ, &hInstalledRoots) == ERROR_SUCCESS)
    {
      PLASMA_SCOPE_EXIT(RegCloseKey(hInstalledRoots));
      DWORD pathLengthInBytes = 0;
      plDynamicArray<wchar_t> path;
      if (RegGetValueW(hInstalledRoots, nullptr, L"KitsRoot10", RRF_RT_REG_SZ, nullptr, nullptr, &pathLengthInBytes) == ERROR_SUCCESS)
      {
        path.SetCount(pathLengthInBytes / sizeof(wchar_t));
        if (RegGetValueW(hInstalledRoots, nullptr, L"KitsRoot10", RRF_RT_REG_SZ, nullptr, path.GetData(), &pathLengthInBytes) == ERROR_SUCCESS)
        {
          plStringBuilder windowsSdkBinPath;
          windowsSdkBinPath = plStringWChar(path.GetData());
          windowsSdkBinPath.MakeCleanPath();
          windowsSdkBinPath.AppendPath("bin");

          plDynamicArray<plFileStats> folders;
          plOSFile::GatherAllItemsInFolder(folders, windowsSdkBinPath, plFileSystemIteratorFlags::ReportFolders);

          folders.Sort([](const plFileStats& a, const plFileStats& b) { return a.m_sName > b.m_sName; });

          for (const plFileStats& folder : folders)
          {
            if (!folder.m_sName.StartsWith("10."))
            {
              continue;
            }
            rcPath = windowsSdkBinPath;
            rcPath.AppendPath(folder.m_sName);
            rcPath.AppendPath("x64/rc.exe");
            if (plOSFile::ExistsFile(rcPath))
            {
              break;
            }
            rcPath.Clear();
          }
        }
      }
    }
    if (!rcPath.IsEmpty())
    {
      preferences->m_CompilerPreferences.m_sRcCompiler = rcPath;
    }
  }

  plString clangVersion;

  wchar_t* pProgramFiles = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, nullptr, &pProgramFiles)))
  {
    plStringBuilder clangDefaultPath;
    clangDefaultPath = plStringWChar(pProgramFiles);
    CoTaskMemFree(pProgramFiles);
    pProgramFiles = nullptr;

    clangDefaultPath.AppendPath("LLVM/bin/clang.exe");
    clangDefaultPath.MakeCleanPath();
    plStringBuilder clangCppDefaultPath = clangDefaultPath;
    clangCppDefaultPath.ReplaceLast(".exe", "++.exe");

    plStringView clangMajorSdkVersion = PLASMA_PP_STRINGIFY(__clang_major__) ".";
    if (TestCompilerExecutable(clangDefaultPath, &clangVersion).Succeeded() && TestCompilerExecutable(clangCppDefaultPath).Succeeded() && clangVersion.StartsWith(clangMajorSdkVersion))
    {
      plStringBuilder clangNiceName;
      clangNiceName.Format("Clang (system default = {})", clangVersion);
      s_MachineSpecificCompilers.PushBack({clangNiceName, plCompiler::Clang, clangDefaultPath, clangCppDefaultPath, false});
    }
  }
#  endif
#endif


#if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
  AddCompilerVersions(s_MachineSpecificCompilers, plCppProject::GetSdkCompiler(), plCppProject::GetSdkCompilerMajorVersion());
#endif

#if PLASMA_ENABLED(PLASMA_COMPILER_CLANG)
  s_MachineSpecificCompilers.PushBack({"Clang (Custom)", plCompiler::Clang, "", "", true});
#endif

#if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) && PLASMA_ENABLED(PLASMA_COMPILER_GCC)
  s_MachineSpecificCompilers.PushBack({"Gcc (Custom)", plCompiler::Gcc, "", "", true});
#endif

  if (preferences->m_CompilerPreferences.m_Compiler != sdkCompiler)
  {
    plStringBuilder incompatibleCompilerName = u8"⚠ ";
    incompatibleCompilerName.Format(u8"⚠ {} (incompatible)", plCppProject::CompilerToString(preferences->m_CompilerPreferences.m_Compiler));
    s_MachineSpecificCompilers.PushBack(
      {incompatibleCompilerName,
        preferences->m_CompilerPreferences.m_Compiler,
        preferences->m_CompilerPreferences.m_sCCompiler,
        preferences->m_CompilerPreferences.m_sCppCompiler,
        preferences->m_CompilerPreferences.m_bCustomCompiler});
  }
}

plResult plCppProject::ForceSdkCompatibleCompiler()
{
  plCppProject* preferences = plPreferences::QueryPreferences<plCppProject>();

  plCompiler::Enum sdkCompiler = GetSdkCompiler();
  for (auto& compiler : s_MachineSpecificCompilers)
  {
    if (!compiler.m_bIsCustom && compiler.m_Compiler == sdkCompiler)
    {
      preferences->m_CompilerPreferences.m_Compiler = sdkCompiler;
      preferences->m_CompilerPreferences.m_sCCompiler = compiler.m_sCCompiler;
      preferences->m_CompilerPreferences.m_sCppCompiler = compiler.m_sCppCompiler;
      preferences->m_CompilerPreferences.m_bCustomCompiler = compiler.m_bIsCustom;

      return PLASMA_SUCCESS;
    }
  }

  return PLASMA_FAILURE;
}

plCppProject::~plCppProject() = default;