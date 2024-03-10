#pragma once

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Types/VariantType.h>
#include <Foundation/Types/Status.h>

// Only saved in editor preferences, does not have to work cross-platform
struct PL_EDITORFRAMEWORK_DLL plIDE
{
  using StorageType = plUInt8;

  enum Enum
  {
    Clion,
    Rider,
    _10x,
    VisualStudioCode,
#if PL_ENABLED(PL_PLATFORM_WINDOWS)
    VisualStudio,
#endif

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
    Default = VisualStudio
#else
    Default = VisualStudioCode
#endif
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_EDITORFRAMEWORK_DLL, plIDE);

// Only saved in editor preferences, does not have to work cross-platform
struct PL_EDITORFRAMEWORK_DLL plCompiler
{
  using StorageType = plUInt8;

  enum Enum
  {
    Clang,
#if PL_ENABLED(PL_PLATFORM_LINUX)
    Gcc,
#elif PL_ENABLED(PL_PLATFORM_WINDOWS)
    Vs2022,
#endif

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
    Default = Vs2022
#else
    Default = Gcc
#endif
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_EDITORFRAMEWORK_DLL, plCompiler);

struct PL_EDITORFRAMEWORK_DLL plCompilerPreferences
{
  plEnum<plCompiler> m_Compiler;
  bool m_bCustomCompiler;
  plString m_sCppCompiler;
  plString m_sCCompiler;
  plString m_sRcCompiler;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_EDITORFRAMEWORK_DLL, plCompilerPreferences);

struct PL_EDITORFRAMEWORK_DLL plCppProject : public plPreferences
{
  PL_ADD_DYNAMIC_REFLECTION(plCppProject, plPreferences);

  struct MachineSpecificCompilerPaths
  {
    plString m_sNiceName;
    plEnum<plCompiler> m_Compiler;
    plString m_sCCompiler;
    plString m_sCppCompiler;
    bool m_bIsCustom;
  };

  enum class ModifyResult
  {
    FAILURE,
    NOT_MODIFIED,
    MODIFIED
  };


  plCppProject();
  ~plCppProject();

  static plString GetTargetSourceDir(plStringView sProjectDirectory = {});

  static plString GetGeneratorFolderName(const plCppSettings& cfg);

  static plString GetCMakeGeneratorName(const plCppSettings& cfg);

  static plString GetPluginSourceDir(const plCppSettings& cfg, plStringView sProjectDirectory = {});

  static plString GetBuildDir(const plCppSettings& cfg);

  static plString GetSolutionPath(const plCppSettings& cfg);

  static plStatus OpenSolution(const plCppSettings& cfg);

  static plStringView CompilerToString(plCompiler::Enum compiler);

  static plCompiler::Enum GetSdkCompiler();

  static plString GetSdkCompilerMajorVersion();

  static plStatus TestCompiler();

  static const char* GetCMakePath();

  static plResult CheckCMakeCache(const plCppSettings& cfg);

  static ModifyResult CheckCMakeUserPresets(const plCppSettings& cfg, bool bWriteResult);

  static bool ExistsSolution(const plCppSettings& cfg);

  static bool ExistsProjectCMakeListsTxt();

  static plResult PopulateWithDefaultSources(const plCppSettings& cfg);

  static plResult CleanBuildDir(const plCppSettings& cfg);

  static plResult RunCMake(const plCppSettings& cfg);

  static plResult RunCMakeIfNecessary(const plCppSettings& cfg);

  static plResult CompileSolution(const plCppSettings& cfg);

  static plResult BuildCodeIfNecessary(const plCppSettings& cfg);

  static plVariantDictionary CreateEmptyCMakeUserPresetsJson(const plCppSettings& cfg);

  static ModifyResult ModifyCMakeUserPresetsJson(const plCppSettings& cfg, plVariantDictionary& inout_json);

  static void UpdatePluginConfig(const plCppSettings& cfg);

  static plResult EnsureCppPluginReady();

  static bool IsBuildRequired();

  /// \brief Fired when a notable change has been made.
  static plEvent<const plCppSettings&> s_ChangeEvents;

  static void LoadPreferences();

  static plArrayPtr<const MachineSpecificCompilerPaths> GetMachineSpecificCompilers() { return s_MachineSpecificCompilers.GetArrayPtr(); }

  // Change the current preferences to point to a SDK compatible compiler
  static plResult ForceSdkCompatibleCompiler();

private:
  plEnum<plIDE> m_Ide;
  plCompilerPreferences m_CompilerPreferences;


  static plDynamicArray<MachineSpecificCompilerPaths> s_MachineSpecificCompilers;
};

