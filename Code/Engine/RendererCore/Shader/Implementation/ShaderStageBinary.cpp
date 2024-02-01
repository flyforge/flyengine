#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>





//////////////////////////////////////////////////////////////////////////

plMap<plUInt32, plShaderStageBinary> plShaderStageBinary::s_ShaderStageBinaries[plGALShaderStage::ENUM_COUNT];

plShaderStageBinary::plShaderStageBinary() = default;

plShaderStageBinary::~plShaderStageBinary()
{
  m_pGALByteCode = nullptr;
}

plResult plShaderStageBinary::Write(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = plShaderStageBinary::VersionCurrent;

  // plShaderStageBinary
  inout_stream << uiVersion;
  inout_stream << m_uiSourceHash;

  // plGALShaderByteCode
  inout_stream << m_pGALByteCode->m_Stage;
  inout_stream << m_pGALByteCode->m_bWasCompiledWithDebug;

  // m_ByteCode
  const plUInt32 uiByteCodeSize = m_pGALByteCode->m_ByteCode.GetCount();
  inout_stream << uiByteCodeSize;
  if (!m_pGALByteCode->m_ByteCode.IsEmpty() && inout_stream.WriteBytes(&m_pGALByteCode->m_ByteCode[0], uiByteCodeSize).Failed())
    return PL_FAILURE;

  // m_ShaderResourceBindings
  const plUInt16 uiResources = static_cast<plUInt16>(m_pGALByteCode->m_ShaderResourceBindings.GetCount());
  inout_stream << uiResources;
  for (const auto& r : m_pGALByteCode->m_ShaderResourceBindings)
  {
    inout_stream << r.m_ResourceType;
    inout_stream << r.m_TextureType;
    inout_stream << r.m_Stages;
    inout_stream << r.m_iSet;
    inout_stream << r.m_iSlot;
    inout_stream << r.m_uiArraySize;
    inout_stream << r.m_sName.GetData();
    const bool bHasLayout = r.m_pLayout != nullptr;
    inout_stream << bHasLayout;
    if (bHasLayout)
    {
      PL_SUCCEED_OR_RETURN(Write(inout_stream, *r.m_pLayout));
    }
  }

  // m_ShaderVertexInput
  const plUInt16 uiVertexInputs = static_cast<plUInt16>(m_pGALByteCode->m_ShaderVertexInput.GetCount());
  inout_stream << uiVertexInputs;
  for (const auto& v : m_pGALByteCode->m_ShaderVertexInput)
  {
    inout_stream << v.m_eSemantic;
    inout_stream << v.m_eFormat;
    inout_stream << v.m_uiLocation;
  }

  return PL_SUCCESS;
}


plResult plShaderStageBinary::Write(plStreamWriter& inout_stream, const plShaderConstantBufferLayout& layout) const
{
  inout_stream << layout.m_uiTotalSize;

  plUInt16 uiConstants = static_cast<plUInt16>(layout.m_Constants.GetCount());
  inout_stream << uiConstants;

  for (auto& constant : layout.m_Constants)
  {
    inout_stream << constant.m_sName;
    inout_stream << constant.m_Type;
    inout_stream << constant.m_uiArrayElements;
    inout_stream << constant.m_uiOffset;
  }

  return PL_SUCCESS;
}

plResult plShaderStageBinary::Read(plStreamReader& inout_stream)
{
  PL_ASSERT_DEBUG(m_pGALByteCode == nullptr, "");
  m_pGALByteCode = PL_DEFAULT_NEW(plGALShaderByteCode);

  plUInt8 uiVersion = 0;

  if (inout_stream.ReadBytes(&uiVersion, sizeof(plUInt8)) != sizeof(plUInt8))
    return PL_FAILURE;

  if (uiVersion < plShaderStageBinary::Version::Version6)
  {
    plLog::Error("Old shader binaries are not supported anymore and need to be recompiled, please delete shader cache.");
    return PL_FAILURE;
  }

  PL_ASSERT_DEV(uiVersion <= plShaderStageBinary::VersionCurrent, "Wrong Version {0}", uiVersion);

  inout_stream >> m_uiSourceHash;

  // plGALShaderByteCode
  inout_stream >> m_pGALByteCode->m_Stage;
  inout_stream >> m_pGALByteCode->m_bWasCompiledWithDebug;

  // m_ByteCode
  {
    plUInt32 uiByteCodeSize = 0;
    inout_stream >> uiByteCodeSize;
    m_pGALByteCode->m_ByteCode.SetCountUninitialized(uiByteCodeSize);
    if (!m_pGALByteCode->m_ByteCode.IsEmpty() && inout_stream.ReadBytes(&m_pGALByteCode->m_ByteCode[0], uiByteCodeSize) != uiByteCodeSize)
      return PL_FAILURE;
  }

  // m_ShaderResourceBindings
  {
    plUInt16 uiResources = 0;
    inout_stream >> uiResources;

    m_pGALByteCode->m_ShaderResourceBindings.SetCount(uiResources);

    plString sTemp;

    for (auto& r : m_pGALByteCode->m_ShaderResourceBindings)
    {
      inout_stream >> r.m_ResourceType;
      inout_stream >> r.m_TextureType;
      inout_stream >> r.m_Stages;
      inout_stream >> r.m_iSet;
      inout_stream >> r.m_iSlot;
      inout_stream >> r.m_uiArraySize;
      inout_stream >> sTemp;
      r.m_sName.Assign(sTemp.GetData());

      bool bHasLayout = false;
      inout_stream >> bHasLayout;

      if (bHasLayout)
      {
        r.m_pLayout = PL_DEFAULT_NEW(plShaderConstantBufferLayout);
        PL_SUCCEED_OR_RETURN(Read(inout_stream, *r.m_pLayout));
      }
    }
  }

  // m_ShaderVertexInput
  {
    plUInt16 uiVertexInputs = 0;
    inout_stream >> uiVertexInputs;
    m_pGALByteCode->m_ShaderVertexInput.SetCount(uiVertexInputs);

    for (auto& v : m_pGALByteCode->m_ShaderVertexInput)
    {
      inout_stream >> v.m_eSemantic;
      inout_stream >> v.m_eFormat;
      inout_stream >> v.m_uiLocation;
    }
  }

  return PL_SUCCESS;
}



plResult plShaderStageBinary::Read(plStreamReader& inout_stream, plShaderConstantBufferLayout& out_layout)
{
  inout_stream >> out_layout.m_uiTotalSize;

  plUInt16 uiConstants = 0;
  inout_stream >> uiConstants;

  out_layout.m_Constants.SetCount(uiConstants);

  for (auto& constant : out_layout.m_Constants)
  {
    inout_stream >> constant.m_sName;
    inout_stream >> constant.m_Type;
    inout_stream >> constant.m_uiArrayElements;
    inout_stream >> constant.m_uiOffset;
  }

  return PL_SUCCESS;
}

plSharedPtr<const plGALShaderByteCode> plShaderStageBinary::GetByteCode() const
{
  return m_pGALByteCode;
}

plResult plShaderStageBinary::WriteStageBinary(plLogInterface* pLog) const
{
  plStringBuilder sShaderStageFile = plShaderManager::GetCacheDirectory();

  sShaderStageFile.AppendPath(plShaderManager::GetActivePlatform().GetData());
  sShaderStageFile.AppendFormat("/{0}_{1}.plShaderStage", plGALShaderStage::Names[m_pGALByteCode->m_Stage], plArgU(m_uiSourceHash, 8, true, 16, true));

  plFileWriter StageFileOut;
  if (StageFileOut.Open(sShaderStageFile.GetData()).Failed())
  {
    plLog::Error(pLog, "Could not open shader stage file '{0}' for writing", sShaderStageFile);
    return PL_FAILURE;
  }

  if (Write(StageFileOut).Failed())
  {
    plLog::Error(pLog, "Could not write shader stage file '{0}'", sShaderStageFile);
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

// static
plShaderStageBinary* plShaderStageBinary::LoadStageBinary(plGALShaderStage::Enum Stage, plUInt32 uiHash)
{
  auto itStage = s_ShaderStageBinaries[Stage].Find(uiHash);

  if (!itStage.IsValid())
  {
    plStringBuilder sShaderStageFile = plShaderManager::GetCacheDirectory();

    sShaderStageFile.AppendPath(plShaderManager::GetActivePlatform().GetData());
    sShaderStageFile.AppendFormat("/{0}_{1}.plShaderStage", plGALShaderStage::Names[Stage], plArgU(uiHash, 8, true, 16, true));

    plFileReader StageFileIn;
    if (StageFileIn.Open(sShaderStageFile.GetData()).Failed())
    {
      plLog::Debug("Could not open shader stage file '{0}' for reading", sShaderStageFile);
      return nullptr;
    }

    plShaderStageBinary shaderStageBinary;
    if (shaderStageBinary.Read(StageFileIn).Failed())
    {
      plLog::Error("Could not read shader stage file '{0}'", sShaderStageFile);
      return nullptr;
    }

    itStage = plShaderStageBinary::s_ShaderStageBinaries[Stage].Insert(uiHash, shaderStageBinary);
  }

  plShaderStageBinary* pShaderStageBinary = &itStage.Value();
  return pShaderStageBinary;
}

// static
void plShaderStageBinary::OnEngineShutdown()
{
  for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    s_ShaderStageBinaries[stage].Clear();
  }
}

PL_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderStageBinary);
