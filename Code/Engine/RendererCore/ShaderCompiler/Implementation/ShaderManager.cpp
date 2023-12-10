#include <RendererCore/RendererCorePCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

bool plShaderManager::s_bEnableRuntimeCompilation = false;
plString plShaderManager::s_sPlatform;
plString plShaderManager::s_sPermVarSubDir;
plString plShaderManager::s_sShaderCacheDirectory;

namespace
{
  struct PermutationVarConfig
  {
    plHashedString m_sName;
    plVariant m_DefaultValue;
    plDynamicArray<plShaderParser::EnumValue, plStaticAllocatorWrapper> m_EnumValues;
  };

  static plDeque<PermutationVarConfig, plStaticAllocatorWrapper> s_PermutationVarConfigsStorage;
  static plHashTable<plHashedString, PermutationVarConfig*> s_PermutationVarConfigs;
  static plMutex s_PermutationVarConfigsMutex;

  const PermutationVarConfig* FindConfig(const char* szName, const plTempHashedString& sHashedName)
  {
    PLASMA_LOCK(s_PermutationVarConfigsMutex);

    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigs.TryGetValue(sHashedName, pConfig))
    {
      plShaderManager::ReloadPermutationVarConfig(szName, sHashedName);
      s_PermutationVarConfigs.TryGetValue(sHashedName, pConfig);
    }

    return pConfig;
  }

  const PermutationVarConfig* FindConfig(const plHashedString& sName)
  {
    PLASMA_LOCK(s_PermutationVarConfigsMutex);

    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigs.TryGetValue(sName, pConfig))
    {
      plShaderManager::ReloadPermutationVarConfig(sName.GetData(), sName);
      s_PermutationVarConfigs.TryGetValue(sName, pConfig);
    }

    return pConfig;
  }

  static plHashedString s_sTrue = plMakeHashedString("TRUE");
  static plHashedString s_sFalse = plMakeHashedString("FALSE");

  bool IsValueAllowed(const PermutationVarConfig& config, const plTempHashedString& sValue, plHashedString& out_sValue)
  {
    if (config.m_DefaultValue.IsA<bool>())
    {
      if (sValue == s_sTrue)
      {
        out_sValue = s_sTrue;
        return true;
      }

      if (sValue == s_sFalse)
      {
        out_sValue = s_sFalse;
        return true;
      }
    }
    else
    {
      for (auto& enumValue : config.m_EnumValues)
      {
        if (enumValue.m_sValueName == sValue)
        {
          out_sValue = enumValue.m_sValueName;
          return true;
        }
      }
    }

    return false;
  }

  bool IsValueAllowed(const PermutationVarConfig& config, const plTempHashedString& sValue)
  {
    if (config.m_DefaultValue.IsA<bool>())
    {
      return sValue == s_sTrue || sValue == s_sFalse;
    }
    else
    {
      for (auto& enumValue : config.m_EnumValues)
      {
        if (enumValue.m_sValueName == sValue)
          return true;
      }
    }

    return false;
  }

  static plHashTable<plUInt64, plString> s_PermutationPaths;
} // namespace

//////////////////////////////////////////////////////////////////////////

void plShaderManager::Configure(const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory, const char* szPermVarSubDirectory)
{
  s_sShaderCacheDirectory = szShaderCacheDirectory;
  s_sPermVarSubDir = szPermVarSubDirectory;

  plStringBuilder s = szActivePlatform;
  s.ToUpper();

  s_bEnableRuntimeCompilation = bEnableRuntimeCompilation;
  s_sPlatform = s;
}

void plShaderManager::ReloadPermutationVarConfig(const char* szName, const plTempHashedString& sHashedName)
{
  // clear earlier data
  {
    PLASMA_LOCK(s_PermutationVarConfigsMutex);

    s_PermutationVarConfigs.Remove(sHashedName);
  }

  plStringBuilder sPath;
  sPath.Format("{0}/{1}.plPermVar", s_sPermVarSubDir, szName);

  plStringBuilder sTemp = s_sPlatform;
  sTemp.Append(" 1");

  plPreprocessor pp;
  pp.SetLogInterface(plLog::GetThreadLocalLogSystem());
  pp.SetPassThroughLine(false);
  pp.SetPassThroughPragma(false);
  pp.AddCustomDefine(sTemp.GetData()).IgnoreResult();

  if (pp.Process(sPath, sTemp, false).Failed())
  {
    plLog::Error("Could not read shader permutation variable '{0}' from file '{1}'", szName, sPath);
  }

  plVariant defaultValue;
  plShaderParser::EnumDefinition enumDef;

  plShaderParser::ParsePermutationVarConfig(sTemp, defaultValue, enumDef);
  if (defaultValue.IsValid())
  {
    PLASMA_LOCK(s_PermutationVarConfigsMutex);

    auto pConfig = &s_PermutationVarConfigsStorage.ExpandAndGetRef();
    pConfig->m_sName.Assign(szName);
    pConfig->m_DefaultValue = defaultValue;
    pConfig->m_EnumValues = enumDef.m_Values;

    s_PermutationVarConfigs.Insert(pConfig->m_sName, pConfig);
  }
}

bool plShaderManager::IsPermutationValueAllowed(const char* szName, const plTempHashedString& sHashedName, const plTempHashedString& sValue, plHashedString& out_sName, plHashedString& out_sValue)
{
  const PermutationVarConfig* pConfig = FindConfig(szName, sHashedName);
  if (pConfig == nullptr)
  {
    plLog::Error("Permutation variable '{0}' does not exist", szName);
    return false;
  }

  out_sName = pConfig->m_sName;

  if (!IsValueAllowed(*pConfig, sValue, out_sValue))
  {
    if (!s_bEnableRuntimeCompilation)
    {
      return false;
    }

    plLog::Debug("Invalid Shader Permutation: '{0}' cannot be set to value '{1}' -> reloading config for variable", szName, sValue.GetHash());
    ReloadPermutationVarConfig(szName, sHashedName);

    if (!IsValueAllowed(*pConfig, sValue, out_sValue))
    {
      plLog::Error("Invalid Shader Permutation: '{0}' cannot be set to value '{1}'", szName, sValue.GetHash());
      return false;
    }
  }

  return true;
}

bool plShaderManager::IsPermutationValueAllowed(const plHashedString& sName, const plHashedString& sValue)
{
  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig == nullptr)
  {
    plLog::Error("Permutation variable '{0}' does not exist", sName);
    return false;
  }

  if (!IsValueAllowed(*pConfig, sValue))
  {
    if (!s_bEnableRuntimeCompilation)
    {
      return false;
    }

    plLog::Debug("Invalid Shader Permutation: '{0}' cannot be set to value '{1}' -> reloading config for variable", sName, sValue);
    ReloadPermutationVarConfig(sName, sName);

    if (!IsValueAllowed(*pConfig, sValue))
    {
      plLog::Error("Invalid Shader Permutation: '{0}' cannot be set to value '{1}'", sName, sValue);
      return false;
    }
  }

  return true;
}

void plShaderManager::GetPermutationValues(const plHashedString& sName, plDynamicArray<plHashedString>& out_values)
{
  out_values.Clear();

  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig == nullptr)
    return;

  if (pConfig->m_DefaultValue.IsA<bool>())
  {
    out_values.PushBack(s_sTrue);
    out_values.PushBack(s_sFalse);
  }
  else
  {
    for (const auto& val : pConfig->m_EnumValues)
    {
      out_values.PushBack(val.m_sValueName);
    }
  }
}

plArrayPtr<const plShaderParser::EnumValue> plShaderManager::GetPermutationEnumValues(const plHashedString& sName)
{
  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig != nullptr)
  {
    return pConfig->m_EnumValues;
  }

  return {};
}

void plShaderManager::PreloadPermutations(plShaderResourceHandle hShader, const plHashTable<plHashedString, plHashedString>& permVars, plTime shouldBeAvailableIn)
{
  PLASMA_ASSERT_NOT_IMPLEMENTED;
#if 0
  plResourceLock<plShaderResource> pShader(hShader, plResourceAcquireMode::BlockTillLoaded);

  if (!pShader->IsShaderValid())
    return;

  /*plUInt32 uiPermutationHash = */ FilterPermutationVars(pShader->GetUsedPermutationVars(), permVars);

  generator.RemoveUnusedPermutations(pShader->GetUsedPermutationVars());

  plHybridArray<plPermutationVar, 16> usedPermVars;

  const plUInt32 uiPermutationCount = generator.GetPermutationCount();
  for (plUInt32 uiPermutation = 0; uiPermutation < uiPermutationCount; ++uiPermutation)
  {
    generator.GetPermutation(uiPermutation, usedPermVars);

    PreloadSingleShaderPermutation(hShader, usedPermVars, tShouldBeAvailableIn);
  }
#endif
}

plShaderPermutationResourceHandle plShaderManager::PreloadSinglePermutation(plShaderResourceHandle hShader, const plHashTable<plHashedString, plHashedString>& permVars, bool bAllowFallback)
{
  plResourceLock<plShaderResource> pShader(hShader, bAllowFallback ? plResourceAcquireMode::AllowLoadingFallback : plResourceAcquireMode::BlockTillLoaded);

  if (!pShader->IsShaderValid())
    return plShaderPermutationResourceHandle();

  plHybridArray<plPermutationVar, 64> filteredPermutationVariables(plFrameAllocator::GetCurrentAllocator());
  plUInt32 uiPermutationHash = FilterPermutationVars(pShader->GetUsedPermutationVars(), permVars, filteredPermutationVariables);

  return PreloadSinglePermutationInternal(pShader->GetResourceID(), pShader->GetResourceIDHash(), uiPermutationHash, filteredPermutationVariables);
}


plUInt32 plShaderManager::FilterPermutationVars(plArrayPtr<const plHashedString> usedVars, const plHashTable<plHashedString, plHashedString>& permVars, plDynamicArray<plPermutationVar>& out_FilteredPermutationVariables)
{
  for (auto& sName : usedVars)
  {
    auto& var = out_FilteredPermutationVariables.ExpandAndGetRef();
    var.m_sName = sName;

    if (!permVars.TryGetValue(sName, var.m_sValue))
    {
      const PermutationVarConfig* pConfig = FindConfig(sName);
      if (pConfig == nullptr)
        continue;

      const plVariant& defaultValue = pConfig->m_DefaultValue;
      if (defaultValue.IsA<bool>())
      {
        var.m_sValue = defaultValue.Get<bool>() ? s_sTrue : s_sFalse;
      }
      else
      {
        plUInt32 uiDefaultValue = defaultValue.Get<plUInt32>();
        var.m_sValue = pConfig->m_EnumValues[uiDefaultValue].m_sValueName;
      }
    }
  }

  return plShaderHelper::CalculateHash(out_FilteredPermutationVariables);
}



plShaderPermutationResourceHandle plShaderManager::PreloadSinglePermutationInternal(const char* szResourceId, plUInt64 uiResourceIdHash, plUInt32 uiPermutationHash, plArrayPtr<plPermutationVar> filteredPermutationVariables)
{
  const plUInt64 uiPermutationKey = (plUInt64)plHashingUtils::StringHashTo32(uiResourceIdHash) << 32 | uiPermutationHash;

  plString* pPermutationPath = &s_PermutationPaths[uiPermutationKey];
  if (pPermutationPath->IsEmpty())
  {
    plStringBuilder sShaderFile = GetCacheDirectory();
    sShaderFile.AppendPath(GetActivePlatform().GetData());
    sShaderFile.AppendPath(szResourceId);
    sShaderFile.ChangeFileExtension("");
    if (sShaderFile.EndsWith("."))
      sShaderFile.Shrink(0, 1);
    sShaderFile.AppendFormat("_{0}.plPermutation", plArgU(uiPermutationHash, 8, true, 16, true));

    *pPermutationPath = sShaderFile;
  }

  plShaderPermutationResourceHandle hShaderPermutation = plResourceManager::LoadResource<plShaderPermutationResource>(pPermutationPath->GetData());

  {
    plResourceLock<plShaderPermutationResource> pShaderPermutation(hShaderPermutation, plResourceAcquireMode::PointerOnly);
    if (!pShaderPermutation->IsShaderValid())
    {
      pShaderPermutation->m_PermutationVars = filteredPermutationVariables;
    }
  }

  plResourceManager::PreloadResource(hShaderPermutation);

  return hShaderPermutation;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderManager);
