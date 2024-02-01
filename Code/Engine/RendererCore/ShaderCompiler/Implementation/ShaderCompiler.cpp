#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plShaderProgramCompiler, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  static bool PlatformEnabled(const plString& sPlatforms, const char* szPlatform)
  {
    plStringBuilder sTemp;
    sTemp = szPlatform;

    sTemp.Prepend("!");

    // if it contains '!platform'
    if (sPlatforms.FindWholeWord_NoCase(sTemp, plStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return false;

    sTemp = szPlatform;

    // if it contains 'platform'
    if (sPlatforms.FindWholeWord_NoCase(sTemp, plStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return true;

    // do not enable this when ALL is specified
    if (plStringUtils::IsEqual(szPlatform, "DEBUG"))
      return false;

    // if it contains 'ALL'
    if (sPlatforms.FindWholeWord_NoCase("ALL", plStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return true;

    return false;
  }

  static void GenerateDefines(const char* szPlatform, const plArrayPtr<plPermutationVar>& permutationVars, plHybridArray<plString, 32>& out_defines)
  {
    plStringBuilder sTemp;

    if (out_defines.IsEmpty())
    {
      out_defines.PushBack("TRUE 1");
      out_defines.PushBack("FALSE 0");

      sTemp = szPlatform;
      sTemp.ToUpper();

      out_defines.PushBack(sTemp);
    }

    for (const plPermutationVar& var : permutationVars)
    {
      const char* szValue = var.m_sValue;
      const bool isBoolVar = plStringUtils::IsEqual(szValue, "TRUE") || plStringUtils::IsEqual(szValue, "FALSE");

      if (isBoolVar)
      {
        sTemp.Set(var.m_sName, " ", var.m_sValue);
        out_defines.PushBack(sTemp);
      }
      else
      {
        const char* szName = var.m_sName;
        auto enumValues = plShaderManager::GetPermutationEnumValues(var.m_sName);

        for (const auto& ev : enumValues)
        {
          sTemp.SetFormat("{1} {2}", szName, ev.m_sValueName, ev.m_iValueValue);
          out_defines.PushBack(sTemp);
        }

        if (plStringUtils::StartsWith(szValue, szName))
        {
          sTemp.Set(szName, " ", szValue);
        }
        else
        {
          sTemp.Set(szName, " ", szName, "_", szValue);
        }
        out_defines.PushBack(sTemp);
      }
    }
  }

  static const char* s_szStageDefines[plGALShaderStage::ENUM_COUNT] = {"VERTEX_SHADER", "HULL_SHADER", "DOMAIN_SHADER", "GEOMETRY_SHADER", "PIXEL_SHADER", "COMPUTE_SHADER"};
} // namespace

plResult plShaderCompiler::FileOpen(plStringView sAbsoluteFile, plDynamicArray<plUInt8>& FileContent, plTimestamp& out_FileModification)
{
  if (sAbsoluteFile == "ShaderRenderState")
  {
    const plString& sData = m_ShaderData.m_StateSource;
    const plUInt32 uiCount = sData.GetElementCount();
    plStringView sString = sData;

    FileContent.SetCountUninitialized(uiCount);

    if (uiCount > 0)
    {
      plMemoryUtils::Copy<plUInt8>(FileContent.GetData(), (const plUInt8*)sString.GetStartPointer(), uiCount);
    }

    return PL_SUCCESS;
  }

  for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_StageSourceFile[stage] == sAbsoluteFile)
    {
      const plString& sData = m_ShaderData.m_ShaderStageSource[stage];
      const plUInt32 uiCount = sData.GetElementCount();
      const char* szString = sData;

      FileContent.SetCountUninitialized(uiCount);

      if (uiCount > 0)
      {
        plMemoryUtils::Copy<plUInt8>(FileContent.GetData(), (const plUInt8*)szString, uiCount);
      }

      return PL_SUCCESS;
    }
  }

  m_IncludeFiles.Insert(sAbsoluteFile);

  plFileReader r;
  if (r.Open(sAbsoluteFile).Failed())
  {
    plLog::Error("Could not find include file '{0}'", sAbsoluteFile);
    return PL_FAILURE;
  }

#if PL_ENABLED(PL_SUPPORTS_FILE_STATS)
  plFileStats stats;
  if (plFileSystem::GetFileStats(sAbsoluteFile, stats).Succeeded())
  {
    out_FileModification = stats.m_LastModificationTime;
  }
#endif

  plUInt8 Temp[4096];

  while (plUInt64 uiRead = r.ReadBytes(Temp, 4096))
  {
    FileContent.PushBackRange(plArrayPtr<plUInt8>(Temp, (plUInt32)uiRead));
  }

  return PL_SUCCESS;
}

plResult plShaderCompiler::CompileShaderPermutationForPlatforms(plStringView sFile, const plArrayPtr<const plPermutationVar>& permutationVars, plLogInterface* pLog, plStringView sPlatform)
{
  plStringBuilder sFileContent, sTemp;

  {
    plFileReader File;
    if (File.Open(sFile).Failed())
      return PL_FAILURE;

    sFileContent.ReadAll(File);
  }

  plShaderHelper::plTextSectionizer Sections;
  plShaderHelper::GetShaderSections(sFileContent, Sections);

  plUInt32 uiFirstLine = 0;
  sTemp = Sections.GetSectionContent(plShaderHelper::plShaderSections::PLATFORMS, uiFirstLine);
  sTemp.ToUpper();

  m_ShaderData.m_Platforms = sTemp;

  plHybridArray<plHashedString, 16> usedPermutations;
  plShaderParser::ParsePermutationSection(Sections.GetSectionContent(plShaderHelper::plShaderSections::PERMUTATIONS, uiFirstLine), usedPermutations, m_ShaderData.m_FixedPermVars);

  for (const plHashedString& usedPermutationVar : usedPermutations)
  {
    plUInt32 uiIndex = plInvalidIndex;
    for (plUInt32 i = 0; i < permutationVars.GetCount(); ++i)
    {
      if (permutationVars[i].m_sName == usedPermutationVar)
      {
        uiIndex = i;
        break;
      }
    }

    if (uiIndex != plInvalidIndex)
    {
      m_ShaderData.m_Permutations.PushBack(permutationVars[uiIndex]);
    }
    else
    {
      plLog::Error("No value given for permutation var '{0}'. Assuming default value of zero.", usedPermutationVar);

      plPermutationVar& finalVar = m_ShaderData.m_Permutations.ExpandAndGetRef();
      finalVar.m_sName = usedPermutationVar;
      finalVar.m_sValue.Assign("0");
    }
  }

  m_ShaderData.m_StateSource = Sections.GetSectionContent(plShaderHelper::plShaderSections::RENDERSTATE, uiFirstLine);

  plUInt32 uiFirstShaderLine = 0;
  plStringView sShaderSource = Sections.GetSectionContent(plShaderHelper::plShaderSections::SHADER, uiFirstShaderLine);

  for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    plStringView sStageSource = Sections.GetSectionContent(plShaderHelper::plShaderSections::VERTEXSHADER + stage, uiFirstLine);

    // later code checks whether the string is empty, to see whether we have any shader source, so this has to be kept empty
    if (!sStageSource.IsEmpty())
    {
      sTemp.Clear();

      // prepend common shader section if there is any
      if (!sShaderSource.IsEmpty())
      {
        sTemp.AppendFormat("#line {0}\n{1}", uiFirstShaderLine, sShaderSource);
      }

      sTemp.AppendFormat("#line {0}\n{1}", uiFirstLine, sStageSource);

      m_ShaderData.m_ShaderStageSource[stage] = sTemp;
    }
    else
    {
      m_ShaderData.m_ShaderStageSource[stage].Clear();
    }
  }

  plStringBuilder tmp = sFile;
  tmp.MakeCleanPath();

  m_StageSourceFile[plGALShaderStage::VertexShader] = tmp;
  m_StageSourceFile[plGALShaderStage::VertexShader].ChangeFileExtension("vs");

  m_StageSourceFile[plGALShaderStage::HullShader] = tmp;
  m_StageSourceFile[plGALShaderStage::HullShader].ChangeFileExtension("hs");

  m_StageSourceFile[plGALShaderStage::DomainShader] = tmp;
  m_StageSourceFile[plGALShaderStage::DomainShader].ChangeFileExtension("ds");

  m_StageSourceFile[plGALShaderStage::GeometryShader] = tmp;
  m_StageSourceFile[plGALShaderStage::GeometryShader].ChangeFileExtension("gs");

  m_StageSourceFile[plGALShaderStage::PixelShader] = tmp;
  m_StageSourceFile[plGALShaderStage::PixelShader].ChangeFileExtension("ps");

  m_StageSourceFile[plGALShaderStage::ComputeShader] = tmp;
  m_StageSourceFile[plGALShaderStage::ComputeShader].ChangeFileExtension("cs");

  // try out every compiler that we can find
  plResult result = PL_SUCCESS;
  plRTTI::ForEachDerivedType<plShaderProgramCompiler>(
    [&](const plRTTI* pRtti)
    {
      plUniquePtr<plShaderProgramCompiler> pCompiler = pRtti->GetAllocator()->Allocate<plShaderProgramCompiler>();

      if (RunShaderCompiler(sFile, sPlatform, pCompiler.Borrow(), pLog).Failed())
        result = PL_FAILURE;
    },
    plRTTI::ForEachOptions::ExcludeNonAllocatable);

  return result;
}

plResult plShaderCompiler::RunShaderCompiler(plStringView sFile, plStringView sPlatform, plShaderProgramCompiler* pCompiler, plLogInterface* pLog)
{
  PL_LOG_BLOCK(pLog, "Compiling Shader", sFile);

  plStringBuilder sProcessed[plGALShaderStage::ENUM_COUNT];

  plHybridArray<plString, 4> Platforms;
  pCompiler->GetSupportedPlatforms(Platforms);

  for (plUInt32 p = 0; p < Platforms.GetCount(); ++p)
  {
    if (!PlatformEnabled(sPlatform, Platforms[p]))
      continue;

    // if this shader is not tagged for this platform, ignore it
    if (!PlatformEnabled(m_ShaderData.m_Platforms, Platforms[p]))
      continue;

    PL_LOG_BLOCK(pLog, "Platform", Platforms[p]);

    plShaderProgramData spd;
    spd.m_sSourceFile = sFile;
    spd.m_sPlatform = Platforms[p];

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
    // 'DEBUG' is a platform tag that enables additional compiler flags
    if (PlatformEnabled(m_ShaderData.m_Platforms, "DEBUG"))
    {
      plLog::Warning("Shader specifies the 'DEBUG' platform, which enables the debug shader compiler flag.");
      spd.m_Flags.Add(plShaderCompilerFlags::Debug);
    }
#endif

    m_IncludeFiles.Clear();

    plHybridArray<plString, 32> defines;
    GenerateDefines(Platforms[p], m_ShaderData.m_Permutations, defines);
    GenerateDefines(Platforms[p], m_ShaderData.m_FixedPermVars, defines);

    plShaderPermutationBinary shaderPermutationBinary;

    // Generate Shader State Source
    {
      PL_LOG_BLOCK(pLog, "Preprocessing Shader State Source");

      plPreprocessor pp;
      pp.SetCustomFileCache(&m_FileCache);
      pp.SetLogInterface(plLog::GetThreadLocalLogSystem());
      pp.SetFileOpenFunction(plPreprocessor::FileOpenCB(&plShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(false);
      pp.SetPassThroughLine(false);

      for (auto& define : defines)
      {
        PL_SUCCEED_OR_RETURN(pp.AddCustomDefine(define));
      }

      bool bFoundUndefinedVars = false;
      pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVars](const plPreprocessor::ProcessingEvent& e)
        {
        if (e.m_Type == plPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVars = true;

          plLog::Error("Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}", e.m_pToken->m_DataView, e.m_pToken->m_File, e.m_pToken->m_uiLine);
        } });

      plStringBuilder sOutput;
      if (pp.Process("ShaderRenderState", sOutput, false).Failed() || bFoundUndefinedVars)
      {
        plLog::Error(pLog, "Preprocessing the Shader State block failed");
        return PL_FAILURE;
      }
      else
      {
        if (shaderPermutationBinary.m_StateDescriptor.Parse(sOutput).Failed())
        {
          plLog::Error(pLog, "Failed to interpret the shader state block");
          return PL_FAILURE;
        }
      }
    }

    // Shader Preprocessing
    for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
    {
      spd.m_uiSourceHash[stage] = 0;

      if (m_ShaderData.m_ShaderStageSource[stage].IsEmpty())
        continue;

      bool bFoundUndefinedVars = false;

      plPreprocessor pp;
      pp.SetCustomFileCache(&m_FileCache);
      pp.SetLogInterface(plLog::GetThreadLocalLogSystem());
      pp.SetFileOpenFunction(plPreprocessor::FileOpenCB(&plShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(true);
      pp.SetPassThroughUnknownCmdsCB(plMakeDelegate(&plShaderCompiler::PassThroughUnknownCommandCB, this));
      pp.SetPassThroughLine(false);
      pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVars](const plPreprocessor::ProcessingEvent& e)
        {
        if (e.m_Type == plPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVars = true;

          plLog::Error("Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}", e.m_pToken->m_DataView, e.m_pToken->m_File, e.m_pToken->m_uiLine);
        } });

      PL_SUCCEED_OR_RETURN(pp.AddCustomDefine(s_szStageDefines[stage]));
      for (auto& define : defines)
      {
        PL_SUCCEED_OR_RETURN(pp.AddCustomDefine(define));
      }

      if (pp.Process(m_StageSourceFile[stage], sProcessed[stage], true, true, true).Failed() || bFoundUndefinedVars)
      {
        sProcessed[stage].Clear();
        spd.m_sShaderSource[stage] = m_StageSourceFile[stage];

        plLog::Error(pLog, "Shader preprocessing failed");
        return PL_FAILURE;
      }
      else
      {
        spd.m_sShaderSource[stage] = sProcessed[stage];
      }
    }

    // Let the shader compiler make any modifications to the source code before we hash and compile the shader.
    if (pCompiler->ModifyShaderSource(spd, pLog).Failed())
    {
      WriteFailedShaderSource(spd, pLog);
      return PL_FAILURE;
    }

    // Load shader cache
    for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
    {
      plUInt32 uiSourceStringLen = spd.m_sShaderSource[stage].GetElementCount();
      spd.m_uiSourceHash[stage] = uiSourceStringLen == 0 ? 0u : plHashingUtils::xxHash32(spd.m_sShaderSource[stage].GetData(), uiSourceStringLen);

      if (spd.m_uiSourceHash[stage] != 0)
      {
        plShaderStageBinary* pBinary = plShaderStageBinary::LoadStageBinary((plGALShaderStage::Enum)stage, spd.m_uiSourceHash[stage]);

        if (pBinary)
        {
          spd.m_ByteCode[stage] = pBinary->m_pGALByteCode;
          spd.m_bWriteToDisk[stage] = false;
        }
        else
        {
          // Can't find shader with given hash on disk, create a new plGALShaderByteCode and let the compiler build it.
          spd.m_ByteCode[stage] = PL_DEFAULT_NEW(plGALShaderByteCode);
          spd.m_ByteCode[stage]->m_Stage = (plGALShaderStage::Enum)stage;
          spd.m_ByteCode[stage]->m_bWasCompiledWithDebug = spd.m_Flags.IsSet(plShaderCompilerFlags::Debug);
        }
      }
    }

    // copy the source hashes
    for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
    {
      shaderPermutationBinary.m_uiShaderStageHashes[stage] = spd.m_uiSourceHash[stage];
    }

    // if compilation failed, the stage binary for the source hash will simply not exist and therefore cannot be loaded
    // the .plPermutation file should be updated, however, to store the new source hash to the broken shader
    if (pCompiler->Compile(spd, plLog::GetThreadLocalLogSystem()).Failed())
    {
      WriteFailedShaderSource(spd, pLog);
      return PL_FAILURE;
    }

    for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
    {
      if (spd.m_uiSourceHash[stage] != 0 && spd.m_bWriteToDisk[stage])
      {
        plShaderStageBinary bin;
        bin.m_uiSourceHash = spd.m_uiSourceHash[stage];
        bin.m_pGALByteCode = spd.m_ByteCode[stage];

        if (bin.WriteStageBinary(pLog).Failed())
        {
          plLog::Error(pLog, "Writing stage {0} binary failed", stage);
          return PL_FAILURE;
        }
        plShaderStageBinary::s_ShaderStageBinaries[stage].Insert(bin.m_uiSourceHash, bin);
      }
    }

    plStringBuilder sTemp = plShaderManager::GetCacheDirectory();
    sTemp.AppendPath(Platforms[p]);
    sTemp.AppendPath(sFile);
    sTemp.ChangeFileExtension("");
    if (sTemp.EndsWith("."))
      sTemp.Shrink(0, 1);

    const plUInt32 uiPermutationHash = plShaderHelper::CalculateHash(m_ShaderData.m_Permutations);
    sTemp.AppendFormat("_{0}.plPermutation", plArgU(uiPermutationHash, 8, true, 16, true));

    shaderPermutationBinary.m_DependencyFile.Clear();
    shaderPermutationBinary.m_DependencyFile.AddFileDependency(sFile);

    for (auto it = m_IncludeFiles.GetIterator(); it.IsValid(); ++it)
    {
      shaderPermutationBinary.m_DependencyFile.AddFileDependency(it.Key());
    }

    shaderPermutationBinary.m_PermutationVars = m_ShaderData.m_Permutations;

    plDeferredFileWriter PermutationFileOut;
    PermutationFileOut.SetOutput(sTemp);
    PL_SUCCEED_OR_RETURN(shaderPermutationBinary.Write(PermutationFileOut));

    if (PermutationFileOut.Close().Failed())
    {
      plLog::Error(pLog, "Could not open file for writing: '{0}'", sTemp);
      return PL_FAILURE;
    }
  }

  return PL_SUCCESS;
}


void plShaderCompiler::WriteFailedShaderSource(plShaderProgramData& spd, plLogInterface* pLog)
{
  for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (spd.m_uiSourceHash[stage] != 0 && spd.m_bWriteToDisk[stage])
    {
      plStringBuilder sShaderStageFile = plShaderManager::GetCacheDirectory();

      sShaderStageFile.AppendPath(plShaderManager::GetActivePlatform());
      sShaderStageFile.AppendFormat("/_Failed_{0}_{1}.plShaderSource", plGALShaderStage::Names[stage], plArgU(spd.m_uiSourceHash[stage], 8, true, 16, true));

      plFileWriter StageFileOut;
      if (StageFileOut.Open(sShaderStageFile).Succeeded())
      {
        StageFileOut.WriteBytes(spd.m_sShaderSource[stage].GetData(), spd.m_sShaderSource[stage].GetElementCount()).AssertSuccess();
        plLog::Info(pLog, "Failed shader source written to '{0}'", sShaderStageFile);
      }
    }
  }
}

PL_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderCompiler);
