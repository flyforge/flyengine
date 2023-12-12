#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/Shader/Types.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>

plUInt32 plShaderConstantBufferLayout::Constant::s_TypeSize[(plUInt32)Type::ENUM_COUNT] = {0, sizeof(float) * 1, sizeof(float) * 2, sizeof(float) * 3, sizeof(float) * 4, sizeof(int) * 1, sizeof(int) * 2, sizeof(int) * 3, sizeof(int) * 4, sizeof(plUInt32) * 1, sizeof(plUInt32) * 2,
  sizeof(plUInt32) * 3, sizeof(plUInt32) * 4, sizeof(plShaderMat3), sizeof(plMat4), sizeof(plShaderTransform), sizeof(plShaderBool)};

void plShaderConstantBufferLayout::Constant::CopyDataFormVariant(plUInt8* pDest, plVariant* pValue) const
{
  PLASMA_ASSERT_DEV(m_uiArrayElements == 1, "Array constants are not supported");

  plResult conversionResult = PLASMA_FAILURE;

  if (pValue != nullptr)
  {
    switch (m_Type)
    {
      case Type::Float1:
        *reinterpret_cast<float*>(pDest) = pValue->ConvertTo<float>(&conversionResult);
        break;
      case Type::Float2:
        *reinterpret_cast<plVec2*>(pDest) = pValue->Get<plVec2>();
        return;
      case Type::Float3:
        *reinterpret_cast<plVec3*>(pDest) = pValue->Get<plVec3>();
        return;
      case Type::Float4:
        if (pValue->GetType() == plVariant::Type::Color || pValue->GetType() == plVariant::Type::ColorGamma)
        {
          const plColor tmp = pValue->ConvertTo<plColor>();
          *reinterpret_cast<plVec4*>(pDest) = *reinterpret_cast<const plVec4*>(&tmp);
        }
        else
        {
          *reinterpret_cast<plVec4*>(pDest) = pValue->Get<plVec4>();
        }
        return;

      case Type::Int1:
        *reinterpret_cast<plInt32*>(pDest) = pValue->ConvertTo<plInt32>(&conversionResult);
        break;
      case Type::Int2:
        *reinterpret_cast<plVec2I32*>(pDest) = pValue->Get<plVec2I32>();
        return;
      case Type::Int3:
        *reinterpret_cast<plVec3I32*>(pDest) = pValue->Get<plVec3I32>();
        return;
      case Type::Int4:
        *reinterpret_cast<plVec4I32*>(pDest) = pValue->Get<plVec4I32>();
        return;

      case Type::UInt1:
        *reinterpret_cast<plUInt32*>(pDest) = pValue->ConvertTo<plUInt32>(&conversionResult);
        break;
      case Type::UInt2:
        *reinterpret_cast<plVec2U32*>(pDest) = pValue->Get<plVec2U32>();
        return;
      case Type::UInt3:
        *reinterpret_cast<plVec3U32*>(pDest) = pValue->Get<plVec3U32>();
        return;
      case Type::UInt4:
        *reinterpret_cast<plVec4U32*>(pDest) = pValue->Get<plVec4U32>();
        return;

      case Type::Mat3x3:
        *reinterpret_cast<plShaderMat3*>(pDest) = pValue->Get<plMat3>();
        return;
      case Type::Mat4x4:
        *reinterpret_cast<plMat4*>(pDest) = pValue->Get<plMat4>();
        return;
      case Type::Transform:
        *reinterpret_cast<plShaderTransform*>(pDest) = pValue->Get<plTransform>();
        return;

      case Type::Bool:
        *reinterpret_cast<plShaderBool*>(pDest) = pValue->ConvertTo<bool>(&conversionResult);
        break;

      default:
        PLASMA_ASSERT_NOT_IMPLEMENTED;
    }
  }

  if (conversionResult.Succeeded())
  {
    return;
  }

  // plLog::Error("Constant '{0}' is not set, invalid or couldn't be converted to target type and will be set to zero.", m_sName);
  const plUInt32 uiSize = s_TypeSize[m_Type];
  plMemoryUtils::ZeroFill(pDest, uiSize);
}

plShaderConstantBufferLayout::plShaderConstantBufferLayout()
{
  m_uiTotalSize = 0;
}

plShaderConstantBufferLayout::~plShaderConstantBufferLayout() {}

plResult plShaderConstantBufferLayout::Write(plStreamWriter& inout_stream) const
{
  inout_stream << m_uiTotalSize;

  plUInt16 uiConstants = static_cast<plUInt16>(m_Constants.GetCount());
  inout_stream << uiConstants;

  for (auto& constant : m_Constants)
  {
    inout_stream << constant.m_sName;
    inout_stream << constant.m_Type;
    inout_stream << constant.m_uiArrayElements;
    inout_stream << constant.m_uiOffset;
  }

  return PLASMA_SUCCESS;
}

plResult plShaderConstantBufferLayout::Read(plStreamReader& inout_stream)
{
  inout_stream >> m_uiTotalSize;

  plUInt16 uiConstants = 0;
  inout_stream >> uiConstants;

  m_Constants.SetCount(uiConstants);

  for (auto& constant : m_Constants)
  {
    inout_stream >> constant.m_sName;
    inout_stream >> constant.m_Type;
    inout_stream >> constant.m_uiArrayElements;
    inout_stream >> constant.m_uiOffset;
  }

  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

plShaderResourceBinding::plShaderResourceBinding()
{
  m_Type = plShaderResourceType::Unknown;
  m_iSlot = -1;
  m_pLayout = nullptr;
}

plShaderResourceBinding::~plShaderResourceBinding() {}

//////////////////////////////////////////////////////////////////////////

plMap<plUInt32, plShaderStageBinary> plShaderStageBinary::s_ShaderStageBinaries[plGALShaderStage::ENUM_COUNT];

plShaderStageBinary::plShaderStageBinary() = default;

plShaderStageBinary::~plShaderStageBinary()
{
  if (m_GALByteCode)
  {
    plGALShaderByteCode* pByteCode = m_GALByteCode;
    m_GALByteCode = nullptr;

    if (pByteCode->GetRefCount() == 0)
      PLASMA_DEFAULT_DELETE(pByteCode);
  }

  for (auto& binding : m_ShaderResourceBindings)
  {
    if (binding.m_pLayout != nullptr)
    {
      plShaderConstantBufferLayout* pLayout = binding.m_pLayout;
      binding.m_pLayout = nullptr;

      if (pLayout->GetRefCount() == 0)
        PLASMA_DEFAULT_DELETE(pLayout);
    }
  }
}

plResult plShaderStageBinary::Write(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = plShaderStageBinary::VersionCurrent;

  if (inout_stream.WriteBytes(&uiVersion, sizeof(plUInt8)).Failed())
    return PLASMA_FAILURE;

  if (inout_stream.WriteDWordValue(&m_uiSourceHash).Failed())
    return PLASMA_FAILURE;

  const plUInt8 uiStage = (plUInt8)m_Stage;

  if (inout_stream.WriteBytes(&uiStage, sizeof(plUInt8)).Failed())
    return PLASMA_FAILURE;

  const plUInt32 uiByteCodeSize = m_ByteCode.GetCount();

  if (inout_stream.WriteDWordValue(&uiByteCodeSize).Failed())
    return PLASMA_FAILURE;

  if (!m_ByteCode.IsEmpty() && inout_stream.WriteBytes(&m_ByteCode[0], uiByteCodeSize).Failed())
    return PLASMA_FAILURE;

  plUInt16 uiResources = static_cast<plUInt16>(m_ShaderResourceBindings.GetCount());
  inout_stream << uiResources;

  for (const auto& r : m_ShaderResourceBindings)
  {
    inout_stream << r.m_sName.GetData();
    inout_stream << r.m_iSlot;
    inout_stream << (plUInt8)r.m_Type;

    if (r.m_Type == plShaderResourceType::ConstantBuffer)
    {
      PLASMA_SUCCEED_OR_RETURN(r.m_pLayout->Write(inout_stream));
    }
  }

  inout_stream << m_bWasCompiledWithDebug;

  return PLASMA_SUCCESS;
}

plResult plShaderStageBinary::Read(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;

  if (inout_stream.ReadBytes(&uiVersion, sizeof(plUInt8)) != sizeof(plUInt8))
    return PLASMA_FAILURE;

  PLASMA_ASSERT_DEV(uiVersion <= plShaderStageBinary::VersionCurrent, "Wrong Version {0}", uiVersion);

  if (inout_stream.ReadDWordValue(&m_uiSourceHash).Failed())
    return PLASMA_FAILURE;

  plUInt8 uiStage = plGALShaderStage::ENUM_COUNT;

  if (inout_stream.ReadBytes(&uiStage, sizeof(plUInt8)) != sizeof(plUInt8))
    return PLASMA_FAILURE;

  m_Stage = (plGALShaderStage::Enum)uiStage;

  plUInt32 uiByteCodeSize = 0;

  if (inout_stream.ReadDWordValue(&uiByteCodeSize).Failed())
    return PLASMA_FAILURE;

  m_ByteCode.SetCountUninitialized(uiByteCodeSize);

  if (!m_ByteCode.IsEmpty() && inout_stream.ReadBytes(&m_ByteCode[0], uiByteCodeSize) != uiByteCodeSize)
    return PLASMA_FAILURE;

  if (uiVersion >= plShaderStageBinary::Version2)
  {
    plUInt16 uiResources = 0;
    inout_stream >> uiResources;

    m_ShaderResourceBindings.SetCount(uiResources);

    plString sTemp;

    for (auto& r : m_ShaderResourceBindings)
    {
      inout_stream >> sTemp;
      r.m_sName.Assign(sTemp.GetData());
      inout_stream >> r.m_iSlot;

      plUInt8 uiType = 0;
      inout_stream >> uiType;
      r.m_Type = (plShaderResourceType::Enum)uiType;

      if (r.m_Type == plShaderResourceType::ConstantBuffer && uiVersion >= plShaderStageBinary::Version4)
      {
        auto pLayout = PLASMA_DEFAULT_NEW(plShaderConstantBufferLayout);
        PLASMA_SUCCEED_OR_RETURN(pLayout->Read(inout_stream));

        r.m_pLayout = pLayout;
      }
    }
  }

  if (uiVersion >= plShaderStageBinary::Version5)
  {
    inout_stream >> m_bWasCompiledWithDebug;
  }

  return PLASMA_SUCCESS;
}


plDynamicArray<plUInt8>& plShaderStageBinary::GetByteCode()
{
  return m_ByteCode;
}

void plShaderStageBinary::AddShaderResourceBinding(const plShaderResourceBinding& binding)
{
  m_ShaderResourceBindings.PushBack(binding);
}


plArrayPtr<const plShaderResourceBinding> plShaderStageBinary::GetShaderResourceBindings() const
{
  return m_ShaderResourceBindings;
}

const plShaderResourceBinding* plShaderStageBinary::GetShaderResourceBinding(const plTempHashedString& sName) const
{
  for (auto& binding : m_ShaderResourceBindings)
  {
    if (binding.m_sName == sName)
    {
      return &binding;
    }
  }

  return nullptr;
}

plShaderConstantBufferLayout* plShaderStageBinary::CreateConstantBufferLayout() const
{
  return PLASMA_DEFAULT_NEW(plShaderConstantBufferLayout);
}

plResult plShaderStageBinary::WriteStageBinary(plLogInterface* pLog) const
{
  plStringBuilder sShaderStageFile = plShaderManager::GetCacheDirectory();

  sShaderStageFile.AppendPath(plShaderManager::GetActivePlatform().GetData());
  sShaderStageFile.AppendFormat("/{0}_{1}.plShaderStage", plGALShaderStage::Names[m_Stage], plArgU(m_uiSourceHash, 8, true, 16, true));

  plFileWriter StageFileOut;
  if (StageFileOut.Open(sShaderStageFile.GetData()).Failed())
  {
    plLog::Error(pLog, "Could not open shader stage file '{0}' for writing", sShaderStageFile);
    return PLASMA_FAILURE;
  }

  if (Write(StageFileOut).Failed())
  {
    plLog::Error(pLog, "Could not write shader stage file '{0}'", sShaderStageFile);
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
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

  if (!itStage.IsValid())
  {
    return nullptr;
  }

  plShaderStageBinary* pShaderStageBinary = &itStage.Value();

  if (pShaderStageBinary->m_GALByteCode == nullptr && !pShaderStageBinary->m_ByteCode.IsEmpty())
  {
    pShaderStageBinary->m_GALByteCode = PLASMA_DEFAULT_NEW(plGALShaderByteCode, pShaderStageBinary->m_ByteCode);
  }

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



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderStageBinary);
