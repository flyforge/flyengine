#include <ShaderCompilerHLSL/ShaderCompilerHLSL.h>
#include <d3dcompiler.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plShaderCompilerHLSL, 1, plRTTIDefaultAllocator<plShaderCompilerHLSL>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult CompileDXShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, plDynamicArray<plUInt8>& out_byteCode)
{
  out_byteCode.Clear();

  ID3DBlob* pResultBlob = nullptr;
  ID3DBlob* pErrorBlob = nullptr;

  const char* szCompileSource = szSource;
  plStringBuilder sDebugSource;
  UINT flags1 = 0;
  if (bDebug)
  {
    flags1 = D3DCOMPILE_DEBUG | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS;
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = szSource;
    sDebugSource.ReplaceAll("#line ", "//ine ");
    szCompileSource = sDebugSource;
  }

  if (FAILED(D3DCompile(szCompileSource, strlen(szCompileSource), szFile, nullptr, nullptr, szEntryPoint, szProfile, flags1, 0, &pResultBlob, &pErrorBlob)))
  {
    if (bDebug)
    {
      // Try again with '#line' intact to get correct error messages with file and line info.
      pErrorBlob->Release();
      pErrorBlob = nullptr;
      PLASMA_VERIFY(FAILED(D3DCompile(szSource, strlen(szSource), szFile, nullptr, nullptr, szEntryPoint, szProfile, flags1, 0, &pResultBlob, &pErrorBlob)), "Debug compilation with commented out '#line' failed but original version did not.");
    }

    const char* szError = static_cast<const char*>(pErrorBlob->GetBufferPointer());

    PLASMA_LOG_BLOCK("Shader Compilation Failed", szFile);

    plLog::Error("Could not compile shader '{0}' for profile '{1}'", szFile, szProfile);
    plLog::Error("{0}", szError);

    pErrorBlob->Release();
    return PLASMA_FAILURE;
  }

  if (pErrorBlob != nullptr)
  {
    const char* szError = static_cast<const char*>(pErrorBlob->GetBufferPointer());

    PLASMA_LOG_BLOCK("Shader Compilation Error Message", szFile);
    plLog::Dev("{0}", szError);

    pErrorBlob->Release();
  }

  if (pResultBlob != nullptr)
  {
    out_byteCode.SetCountUninitialized((plUInt32)pResultBlob->GetBufferSize());
    plMemoryUtils::Copy(out_byteCode.GetData(), static_cast<plUInt8*>(pResultBlob->GetBufferPointer()), out_byteCode.GetCount());
    pResultBlob->Release();
  }

  return PLASMA_SUCCESS;
}

void plShaderCompilerHLSL::ReflectShaderStage(plShaderProgramData& inout_Data, plGALShaderStage::Enum Stage)
{
  ID3D11ShaderReflection* pReflector = nullptr;

  auto byteCode = inout_Data.m_StageBinary[Stage].GetByteCode();
  D3DReflect(byteCode.GetData(), byteCode.GetCount(), IID_ID3D11ShaderReflection, (void**)&pReflector);

  D3D11_SHADER_DESC shaderDesc;
  pReflector->GetDesc(&shaderDesc);

  for (plUInt32 r = 0; r < shaderDesc.BoundResources; ++r)
  {
    D3D11_SHADER_INPUT_BIND_DESC shaderInputBindDesc;
    pReflector->GetResourceBindingDesc(r, &shaderInputBindDesc);

    // plLog::Info("Bound Resource: '{0}' at slot {1} (Count: {2}, Flags: {3})", sibd.Name, sibd.BindPoint, sibd.BindCount, sibd.uFlags);

    plShaderResourceBinding shaderResourceBinding;
    shaderResourceBinding.m_Type = plShaderResourceType::Unknown;
    shaderResourceBinding.m_iSlot = shaderInputBindDesc.BindPoint;
    shaderResourceBinding.m_sName.Assign(shaderInputBindDesc.Name);

    if (shaderInputBindDesc.Type == D3D_SIT_TEXTURE)
    {
      switch (shaderInputBindDesc.Dimension)
      {
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
          shaderResourceBinding.m_Type = plShaderResourceType::Texture1D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
          shaderResourceBinding.m_Type = plShaderResourceType::Texture1DArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
          shaderResourceBinding.m_Type = plShaderResourceType::Texture2D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
          shaderResourceBinding.m_Type = plShaderResourceType::Texture2DArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMS:
          shaderResourceBinding.m_Type = plShaderResourceType::Texture2DMS;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
          shaderResourceBinding.m_Type = plShaderResourceType::Texture2DMSArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE3D:
          shaderResourceBinding.m_Type = plShaderResourceType::Texture3D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBE:
          shaderResourceBinding.m_Type = plShaderResourceType::TextureCube;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
          shaderResourceBinding.m_Type = plShaderResourceType::TextureCubeArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER:
          shaderResourceBinding.m_Type = plShaderResourceType::GenericBuffer;
          break;

        default:
          PLASMA_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWTYPED)
    {
      switch (shaderInputBindDesc.Dimension)
      {
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFEREX:
          shaderResourceBinding.m_Type = plShaderResourceType::UAV;
          break;

        default:
          PLASMA_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED)
      shaderResourceBinding.m_Type = plShaderResourceType::UAV;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS)
      shaderResourceBinding.m_Type = plShaderResourceType::UAV;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED)
      shaderResourceBinding.m_Type = plShaderResourceType::UAV;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED)
      shaderResourceBinding.m_Type = plShaderResourceType::UAV;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER)
      shaderResourceBinding.m_Type = plShaderResourceType::UAV;

    else if (shaderInputBindDesc.Type == D3D_SIT_CBUFFER)
    {
      shaderResourceBinding.m_Type = plShaderResourceType::ConstantBuffer;
      shaderResourceBinding.m_pLayout = ReflectConstantBufferLayout(inout_Data.m_StageBinary[Stage], pReflector->GetConstantBufferByName(shaderInputBindDesc.Name));
    }
    else if (shaderInputBindDesc.Type == D3D_SIT_SAMPLER)
    {
      shaderResourceBinding.m_Type = plShaderResourceType::Sampler;
      if (plStringUtils::EndsWith(shaderInputBindDesc.Name, "_AutoSampler"))
      {
        plStringBuilder sb = shaderInputBindDesc.Name;
        sb.Shrink(0, plStringUtils::GetStringElementCount("_AutoSampler"));
        shaderResourceBinding.m_sName.Assign(sb.GetData());
      }
    }
    else
    {
      shaderResourceBinding.m_Type = plShaderResourceType::GenericBuffer;
    }

    if (shaderResourceBinding.m_Type != plShaderResourceType::Unknown)
    {
      inout_Data.m_StageBinary[Stage].AddShaderResourceBinding(shaderResourceBinding);
    }
  }

  pReflector->Release();
}

plShaderConstantBufferLayout* plShaderCompilerHLSL::ReflectConstantBufferLayout(plShaderStageBinary& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection)
{
  D3D11_SHADER_BUFFER_DESC shaderBufferDesc;

  if (FAILED(pConstantBufferReflection->GetDesc(&shaderBufferDesc)))
  {
    return nullptr;
  }

  PLASMA_LOG_BLOCK("Constant Buffer Layout", shaderBufferDesc.Name);
  plLog::Debug("Constant Buffer has {0} variables, Size is {1}", shaderBufferDesc.Variables, shaderBufferDesc.Size);

  plShaderConstantBufferLayout* pLayout = pStageBinary.CreateConstantBufferLayout();

  pLayout->m_uiTotalSize = shaderBufferDesc.Size;

  for (plUInt32 var = 0; var < shaderBufferDesc.Variables; ++var)
  {
    ID3D11ShaderReflectionVariable* pVar = pConstantBufferReflection->GetVariableByIndex(var);

    D3D11_SHADER_VARIABLE_DESC svd;
    pVar->GetDesc(&svd);

    PLASMA_LOG_BLOCK("Constant", svd.Name);

    D3D11_SHADER_TYPE_DESC std;
    pVar->GetType()->GetDesc(&std);

    plShaderConstantBufferLayout::Constant constant;
    constant.m_uiArrayElements = static_cast<plUInt8>(plMath::Max(std.Elements, 1u));
    constant.m_uiOffset = static_cast<plUInt16>(svd.StartOffset);
    constant.m_sName.Assign(svd.Name);

    if (std.Class == D3D_SVC_SCALAR || std.Class == D3D_SVC_VECTOR)
    {
      switch (std.Type)
      {
        case D3D_SVT_FLOAT:
          constant.m_Type = (plShaderConstantBufferLayout::Constant::Type::Enum)((plInt32)plShaderConstantBufferLayout::Constant::Type::Float1 + std.Columns - 1);
          break;
        case D3D_SVT_INT:
          constant.m_Type = (plShaderConstantBufferLayout::Constant::Type::Enum)((plInt32)plShaderConstantBufferLayout::Constant::Type::Int1 + std.Columns - 1);
          break;
        case D3D_SVT_UINT:
          constant.m_Type = (plShaderConstantBufferLayout::Constant::Type::Enum)((plInt32)plShaderConstantBufferLayout::Constant::Type::UInt1 + std.Columns - 1);
          break;
        case D3D_SVT_BOOL:
          if (std.Columns == 1)
          {
            constant.m_Type = plShaderConstantBufferLayout::Constant::Type::Bool;
          }
          break;

        default:
          break;
      }
    }
    else if (std.Class == D3D_SVC_MATRIX_COLUMNS)
    {
      if (std.Type != D3D_SVT_FLOAT)
      {
        plLog::Error("Variable '{0}': Only float matrices are supported", svd.Name);
        continue;
      }

      if (std.Columns == 3 && std.Rows == 3)
      {
        constant.m_Type = plShaderConstantBufferLayout::Constant::Type::Mat3x3;
      }
      else if (std.Columns == 4 && std.Rows == 4)
      {
        constant.m_Type = plShaderConstantBufferLayout::Constant::Type::Mat4x4;
      }
      else
      {
        plLog::Error("Variable '{0}': {1}x{2} matrices are not supported", svd.Name, std.Rows, std.Columns);
        continue;
      }
    }
    else if (std.Class == D3D_SVC_MATRIX_ROWS)
    {
      plLog::Error("Variable '{0}': Row-Major matrices are not supported", svd.Name);
      continue;
    }
    else if (std.Class == D3D_SVC_STRUCT)
    {
      continue;
    }

    if (constant.m_Type == plShaderConstantBufferLayout::Constant::Type::Default)
    {
      plLog::Error("Variable '{0}': Variable type '{1}' is unknown / not supported", svd.Name, std.Class);
      continue;
    }

    pLayout->m_Constants.PushBack(constant);
  }

  return pLayout;
}

const char* GetProfileName(plStringView sPlatform, plGALShaderStage::Enum stage)
{
  if (sPlatform == "DX11_SM40_93")
  {
    switch (stage)
    {
      case plGALShaderStage::VertexShader:
        return "vs_4_0_level_9_3";
      case plGALShaderStage::PixelShader:
        return "ps_4_0_level_9_3";
      default:
        break;
    }
  }

  if (sPlatform == "DX11_SM40")
  {
    switch (stage)
    {
      case plGALShaderStage::VertexShader:
        return "vs_4_0";
      case plGALShaderStage::GeometryShader:
        return "gs_4_0";
      case plGALShaderStage::PixelShader:
        return "ps_4_0";
      case plGALShaderStage::ComputeShader:
        return "cs_4_0";
      default:
        break;
    }
  }

  if (sPlatform == "DX11_SM41")
  {
    switch (stage)
    {
      case plGALShaderStage::GeometryShader:
        return "gs_4_0";
      case plGALShaderStage::VertexShader:
        return "vs_4_1";
      case plGALShaderStage::PixelShader:
        return "ps_4_1";
      case plGALShaderStage::ComputeShader:
        return "cs_4_1";
      default:
        break;
    }
  }

  if (sPlatform == "DX11_SM50")
  {
    switch (stage)
    {
      case plGALShaderStage::VertexShader:
        return "vs_5_0";
      case plGALShaderStage::HullShader:
        return "hs_5_0";
      case plGALShaderStage::DomainShader:
        return "ds_5_0";
      case plGALShaderStage::GeometryShader:
        return "gs_5_0";
      case plGALShaderStage::PixelShader:
        return "ps_5_0";
      case plGALShaderStage::ComputeShader:
        return "cs_5_0";
      default:
        break;
    }
  }

  PLASMA_REPORT_FAILURE("Unknown Platform '{0}' or Stage {1}", sPlatform, stage);
  return "";
}

plResult plShaderCompilerHLSL::Compile(plShaderProgramData& inout_data, plLogInterface* pLog)
{
  plStringBuilder sFile, sSource;

  for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    // shader already compiled
    if (!inout_data.m_StageBinary[stage].GetByteCode().IsEmpty())
    {
      plLog::Debug("Shader for stage '{0}' is already compiled.", plGALShaderStage::Names[stage]);
      continue;
    }

    plStringView sShaderSource = inout_data.m_sShaderSource[stage];
    const plUInt32 uiLength = sShaderSource.GetElementCount();

    if (uiLength > 0 && sShaderSource.FindSubString("main") != nullptr)
    {
      if (CompileDXShader(inout_data.m_sSourceFile.GetData(sFile), sShaderSource.GetData(sSource), inout_data.m_Flags.IsSet(plShaderCompilerFlags::Debug), GetProfileName(inout_data.m_sPlatform, (plGALShaderStage::Enum)stage), "main", inout_data.m_StageBinary[stage].GetByteCode()).Succeeded())
      {
        ReflectShaderStage(inout_data, (plGALShaderStage::Enum)stage);
      }
      else
      {
        return PLASMA_FAILURE;
      }
    }
  }

  return PLASMA_SUCCESS;
}
