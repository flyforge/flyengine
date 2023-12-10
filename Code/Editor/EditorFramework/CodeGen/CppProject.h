#pragma once

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Communication/Event.h>

struct PLASMA_EDITORFRAMEWORK_DLL plCppProject
{
  static plString GetTargetSourceDir(plStringView sProjectDirectory = {});

  static plString GetGeneratorFolderName(const plCppSettings& cfg);

  static plString GetCMakeGeneratorName(const plCppSettings& cfg);

  static plString GetPluginSourceDir(const plCppSettings& cfg, plStringView sProjectDirectory = {});

  static plString GetBuildDir(const plCppSettings& cfg);

  static plString GetSolutionPath(const plCppSettings& cfg);

  static plResult CheckCMakeCache(const plCppSettings& cfg);

  static bool ExistsSolution(const plCppSettings& cfg);

  static bool ExistsProjectCMakeListsTxt();

  static plResult PopulateWithDefaultSources(const plCppSettings& cfg);

  static plResult CleanBuildDir(const plCppSettings& cfg);

  static plResult RunCMake(const plCppSettings& cfg);

  static plResult RunCMakeIfNecessary(const plCppSettings& cfg);

  static plResult CompileSolution(const plCppSettings& cfg);

  static plResult BuildCodeIfNecessary(const plCppSettings& cfg);

  static plResult FindMsBuild(const plCppSettings& cfg);

  static void UpdatePluginConfig(const plCppSettings& cfg);

  static plResult EnsureCppPluginReady();

  static bool IsBuildRequired();

  /// \brief Fired when a notable change has been made.
  static plEvent<const plCppSettings&> s_ChangeEvents;
};
