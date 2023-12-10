#pragma once

#include <Foundation/Containers/HashTable.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

class PLASMA_RENDERERCORE_DLL plShaderManager
{
public:
  static void Configure(const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory = ":shadercache/ShaderCache",
    const char* szPermVarSubDirectory = "Shaders/PermutationVars");
  static const plString& GetPermutationVarSubDirectory() { return s_sPermVarSubDir; }
  static const plString& GetActivePlatform() { return s_sPlatform; }
  static const plString& GetCacheDirectory() { return s_sShaderCacheDirectory; }
  static bool IsRuntimeCompilationEnabled() { return s_bEnableRuntimeCompilation; }

  static void ReloadPermutationVarConfig(const char* szName, const plTempHashedString& sHashedName);
  static bool IsPermutationValueAllowed(const char* szName, const plTempHashedString& sHashedName, const plTempHashedString& sValue,
    plHashedString& out_sName, plHashedString& out_sValue);
  static bool IsPermutationValueAllowed(const plHashedString& sName, const plHashedString& sValue);

  /// \brief If the given permutation variable is an enum variable, this returns the possible values.
  /// Returns an empty array for other types of permutation variables.
  static plArrayPtr<const plShaderParser::EnumValue> GetPermutationEnumValues(const plHashedString& sName);

  /// \brief Same as GetPermutationEnumValues() but also returns values for other types of variables.
  /// E.g. returns TRUE and FALSE for boolean variables.
  static void GetPermutationValues(const plHashedString& sName, plDynamicArray<plHashedString>& out_values);

  static void PreloadPermutations(
    plShaderResourceHandle hShader, const plHashTable<plHashedString, plHashedString>& permVars, plTime shouldBeAvailableIn);
  static plShaderPermutationResourceHandle PreloadSinglePermutation(
    plShaderResourceHandle hShader, const plHashTable<plHashedString, plHashedString>& permVars, bool bAllowFallback);

private:
  static plUInt32 FilterPermutationVars(plArrayPtr<const plHashedString> usedVars, const plHashTable<plHashedString, plHashedString>& permVars,
    plDynamicArray<plPermutationVar>& out_FilteredPermutationVariables);
  static plShaderPermutationResourceHandle PreloadSinglePermutationInternal(
    const char* szResourceId, plUInt64 uiResourceIdHash, plUInt32 uiPermutationHash, plArrayPtr<plPermutationVar> filteredPermutationVariables);

  static bool s_bEnableRuntimeCompilation;
  static plString s_sPlatform;
  static plString s_sPermVarSubDir;
  static plString s_sShaderCacheDirectory;
};
