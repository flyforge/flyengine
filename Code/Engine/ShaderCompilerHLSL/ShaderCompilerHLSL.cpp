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

  plGALShaderByteCode* pShader = inout_Data.m_ByteCode[Stage];
  D3DReflect(pShader->m_ByteCode.GetData(), pShader->m_ByteCode.GetCount(), IID_ID3D11ShaderReflection, (void**)&pReflector);


  D3D11_SHADER_DESC shaderDesc;
  pReflector->GetDesc(&shaderDesc);

  if (Stage == plGALShaderStage::VertexShader)
  {
    auto& vertexInputAttributes = pShader->m_ShaderVertexInput;
    vertexInputAttributes.Reserve(shaderDesc.InputParameters);
    for (plUInt32 i = 0; i < shaderDesc.InputParameters; ++i)
    {
      D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
      pReflector->GetInputParameterDesc(i, &paramDesc);

      plGALVertexAttributeSemantic::Enum semantic;
      if (!m_VertexInputMapping.TryGetValue(paramDesc.SemanticName, semantic))
      {
        // We ignore all system-value semantics as they are not provided by the user but the system so we don't care to reflect them.
        if (plStringUtils::StartsWith_NoCase(paramDesc.SemanticName, "SV_"))
          continue;

        PLASMA_ASSERT_NOT_IMPLEMENTED;
      }
      switch (semantic)
      {
        case plGALVertexAttributeSemantic::Color0:
          PLASMA_ASSERT_DEBUG(paramDesc.SemanticIndex <= 7, "Color out of range");
          semantic = static_cast<plGALVertexAttributeSemantic::Enum>((plUInt32)semantic + paramDesc.SemanticIndex);
          break;
        case plGALVertexAttributeSemantic::TexCoord0:
          PLASMA_ASSERT_DEBUG(paramDesc.SemanticIndex <= 9, "TexCoord out of range");
          semantic = static_cast<plGALVertexAttributeSemantic::Enum>((plUInt32)semantic + paramDesc.SemanticIndex);
          break;
        case plGALVertexAttributeSemantic::BoneIndices0:
          PLASMA_ASSERT_DEBUG(paramDesc.SemanticIndex <= 1, "BoneIndices out of range");
          semantic = static_cast<plGALVertexAttributeSemantic::Enum>((plUInt32)semantic + paramDesc.SemanticIndex);
          break;
        case plGALVertexAttributeSemantic::BoneWeights0:
          PLASMA_ASSERT_DEBUG(paramDesc.SemanticIndex <= 1, "BoneWeights out of range");
          semantic = static_cast<plGALVertexAttributeSemantic::Enum>((plUInt32)semantic + paramDesc.SemanticIndex);
          break;
      }

      plShaderVertexInputAttribute& attr = vertexInputAttributes.ExpandAndGetRef();
      attr.m_eSemantic = semantic;
      attr.m_eFormat = GetPLASMAFormat(paramDesc);
      attr.m_uiLocation = paramDesc.Register;
    }
  }
  for (plUInt32 r = 0; r < shaderDesc.BoundResources; ++r)
  {
    D3D11_SHADER_INPUT_BIND_DESC shaderInputBindDesc;
    pReflector->GetResourceBindingDesc(r, &shaderInputBindDesc);

    // plLog::Info("Bound Resource: '{0}' at slot {1} (Count: {2}, Flags: {3})", sibd.Name, sibd.BindPoint, sibd.BindCount, sibd.uFlags);
    // #TODO_SHADER remove [x] at the end of the name for arrays
    plShaderResourceBinding shaderResourceBinding;
    shaderResourceBinding.m_iSet = 0;
    shaderResourceBinding.m_iSlot = static_cast<plInt16>(shaderInputBindDesc.BindPoint);
    shaderResourceBinding.m_uiArraySize = shaderInputBindDesc.BindCount;
    shaderResourceBinding.m_sName.Assign(shaderInputBindDesc.Name);
    shaderResourceBinding.m_Stages = plGALShaderStageFlags::MakeFromShaderStage(Stage);

    if (shaderInputBindDesc.Type == D3D_SIT_TEXTURE || shaderInputBindDesc.Type == D3D_SIT_UAV_RWTYPED)
    {
      shaderResourceBinding.m_DescriptorType = shaderInputBindDesc.Type == D3D_SIT_TEXTURE ? plGALShaderDescriptorType::Texture : plGALShaderDescriptorType::TextureRW;
      switch (shaderInputBindDesc.Dimension)
      {
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
          shaderResourceBinding.m_TextureType = plGALShaderTextureType::Texture1D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
          shaderResourceBinding.m_TextureType = plGALShaderTextureType::Texture1DArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
          shaderResourceBinding.m_TextureType = plGALShaderTextureType::Texture2D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
          shaderResourceBinding.m_TextureType = plGALShaderTextureType::Texture2DArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMS:
          shaderResourceBinding.m_TextureType = plGALShaderTextureType::Texture2DMS;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
          shaderResourceBinding.m_TextureType = plGALShaderTextureType::Texture2DMSArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE3D:
          shaderResourceBinding.m_TextureType = plGALShaderTextureType::Texture3D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBE:
          shaderResourceBinding.m_TextureType = plGALShaderTextureType::TextureCube;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
          shaderResourceBinding.m_TextureType = plGALShaderTextureType::TextureCubeArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER:
          shaderResourceBinding.m_DescriptorType = shaderInputBindDesc.Type == D3D_SIT_TEXTURE ? plGALShaderDescriptorType::TexelBuffer : plGALShaderDescriptorType::TexelBufferRW;
          shaderResourceBinding.m_TextureType = plGALShaderTextureType::Unknown;
          break;

        default:
          PLASMA_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }

    else if (shaderInputBindDesc.Type == D3D_SIT_STRUCTURED)
      shaderResourceBinding.m_DescriptorType = plGALShaderDescriptorType::StructuredBuffer;

    else if (shaderInputBindDesc.Type == D3D_SIT_BYTEADDRESS)
      shaderResourceBinding.m_DescriptorType = plGALShaderDescriptorType::StructuredBuffer;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED)
      shaderResourceBinding.m_DescriptorType = plGALShaderDescriptorType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS)
      shaderResourceBinding.m_DescriptorType = plGALShaderDescriptorType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED)
      shaderResourceBinding.m_DescriptorType = plGALShaderDescriptorType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED)
      shaderResourceBinding.m_DescriptorType = plGALShaderDescriptorType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER)
      shaderResourceBinding.m_DescriptorType = plGALShaderDescriptorType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_CBUFFER)
    {
      shaderResourceBinding.m_DescriptorType = plGALShaderDescriptorType::ConstantBuffer;
      shaderResourceBinding.m_pLayout = ReflectConstantBufferLayout(*inout_Data.m_ByteCode[Stage], pReflector->GetConstantBufferByName(shaderInputBindDesc.Name));
    }
    else if (shaderInputBindDesc.Type == D3D_SIT_SAMPLER)
    {
      shaderResourceBinding.m_DescriptorType = plGALShaderDescriptorType::Sampler;
      if (plStringUtils::EndsWith(shaderInputBindDesc.Name, "_AutoSampler"))
      {
        plStringBuilder sb = shaderInputBindDesc.Name;
        sb.Shrink(0, plStringUtils::GetStringElementCount("_AutoSampler"));
        shaderResourceBinding.m_sName.Assign(sb.GetData());
      }
    }
    else
    {
      shaderResourceBinding.m_DescriptorType = plGALShaderDescriptorType::Enum::Unknown;
    }

    if (shaderResourceBinding.m_DescriptorType != plGALShaderDescriptorType::Unknown)
    {
      inout_Data.m_ByteCode[Stage]->m_ShaderResourceBindings.PushBack(shaderResourceBinding);
    }
  }

  pReflector->Release();
}

plShaderConstantBufferLayout* plShaderCompilerHLSL::ReflectConstantBufferLayout(plGALShaderByteCode& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection)
{
  D3D11_SHADER_BUFFER_DESC shaderBufferDesc;

  if (FAILED(pConstantBufferReflection->GetDesc(&shaderBufferDesc)))
  {
    return nullptr;
  }

  PLASMA_LOG_BLOCK("Constant Buffer Layout", shaderBufferDesc.Name);
  plLog::Debug("Constant Buffer has {0} variables, Size is {1}", shaderBufferDesc.Variables, shaderBufferDesc.Size);

  plShaderConstantBufferLayout* pLayout = PLASMA_DEFAULT_NEW(plShaderConstantBufferLayout);

  pLayout->m_uiTotalSize = shaderBufferDesc.Size;

  for (plUInt32 var = 0; var < shaderBufferDesc.Variables; ++var)
  {
    ID3D11ShaderReflectionVariable* pVar = pConstantBufferReflection->GetVariableByIndex(var);

    D3D11_SHADER_VARIABLE_DESC svd;
    pVar->GetDesc(&svd);

    PLASMA_LOG_BLOCK("Constant", svd.Name);

    D3D11_SHADER_TYPE_DESC std;
    pVar->GetType()->GetDesc(&std);

    plShaderConstant constant;
    constant.m_uiArrayElements = static_cast<plUInt8>(plMath::Max(std.Elements, 1u));
    constant.m_uiOffset = static_cast<plUInt16>(svd.StartOffset);
    constant.m_sName.Assign(svd.Name);

    if (std.Class == D3D_SVC_SCALAR || std.Class == D3D_SVC_VECTOR)
    {
      switch (std.Type)
      {
        case D3D_SVT_FLOAT:
          constant.m_Type = (plShaderConstant::Type::Enum)((plInt32)plShaderConstant::Type::Float1 + std.Columns - 1);
          break;
        case D3D_SVT_INT:
          constant.m_Type = (plShaderConstant::Type::Enum)((plInt32)plShaderConstant::Type::Int1 + std.Columns - 1);
          break;
        case D3D_SVT_UINT:
          constant.m_Type = (plShaderConstant::Type::Enum)((plInt32)plShaderConstant::Type::UInt1 + std.Columns - 1);
          break;
        case D3D_SVT_BOOL:
          if (std.Columns == 1)
          {
            constant.m_Type = plShaderConstant::Type::Bool;
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
        constant.m_Type = plShaderConstant::Type::Mat3x3;
      }
      else if (std.Columns == 4 && std.Rows == 4)
      {
        constant.m_Type = plShaderConstant::Type::Mat4x4;
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

    if (constant.m_Type == plShaderConstant::Type::Default)
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
  Initialize();
  plStringBuilder sFile, sSource;

  for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    // Shader stage not used.
    if (inout_data.m_uiSourceHash[stage] == 0)
      continue;

    // Shader already compiled.
    if (inout_data.m_bWriteToDisk[stage] == false)
    {
      plLog::Debug("Shader for stage '{0}' is already compiled.", plGALShaderStage::Names[stage]);
      continue;
    }

    plStringView sShaderSource = inout_data.m_sShaderSource[stage];
    const plUInt32 uiLength = sShaderSource.GetElementCount();

    if (uiLength > 0 && sShaderSource.FindSubString("main") != nullptr)
    {
      if (CompileDXShader(inout_data.m_sSourceFile.GetData(sFile), sShaderSource.GetData(sSource), inout_data.m_Flags.IsSet(plShaderCompilerFlags::Debug), GetProfileName(inout_data.m_sPlatform, (plGALShaderStage::Enum)stage), "main", inout_data.m_ByteCode[stage]->m_ByteCode).Succeeded())
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

plResult plShaderCompilerHLSL::DefineShaderResourceBindings(plShaderProgramData& inout_data, plHashTable<plHashedString, plShaderResourceBinding>& ref_resourceBinding, plLogInterface* pLog)
{
  plHybridBitfield<64> indexInUse[plGALShaderResourceType::ENUM_COUNT];
  for (auto it : ref_resourceBinding)
  {
    const plEnum<plGALShaderResourceType> type = plGALShaderResourceType::MakeFromShaderDescriptorType(it.Value().m_DescriptorType);
    const plInt16 iSlot = it.Value().m_iSlot;
    if (iSlot != -1)
    {
      indexInUse[type].SetCount(plMath::Max(indexInUse[type].GetCount(), static_cast<plUInt32>(iSlot + 1)));
      indexInUse[type].SetBit(iSlot);
    }
    // DX11: Everything is set 0.
    it.Value().m_iSet = 0;
  }

  // Create stable order of resources
  plHybridArray<plHashedString, 16> order[plGALShaderResourceType::ENUM_COUNT];
  for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_data.m_sShaderSource[stage].IsEmpty())
      continue;

    for (const auto& res : inout_data.m_Resources[stage])
    {
      const plEnum<plGALShaderResourceType> type = plGALShaderResourceType::MakeFromShaderDescriptorType(res.m_Binding.m_DescriptorType);
      if (!order[type].Contains(res.m_Binding.m_sName))
      {
        order[type].PushBack(res.m_Binding.m_sName);
      }
    }
  }

  // PLASMA: We only allow constant buffers to be bound globally, so they must all have unique indices.
  // DX11: UAV are bound globally
  // #TODO_SHADER: DX11: SRV, Samplers can be bound by stage, so indices can be re-used. But we treat them globally for now.
  for (plUInt32 type = 0; type < plGALShaderResourceType::ENUM_COUNT; ++type)
  {
    plUInt32 uiCurrentIndex = 0;
    // Workaround for this: error X4509: UAV registers live in the same name space as outputs, so they must be bound to at least u1, manual bind to slot u0 failed
    if (type == plGALShaderResourceType::UAV)
      uiCurrentIndex = 1;

    for (const auto& sName : order[type])
    {
      while (uiCurrentIndex < indexInUse[type].GetCount() && indexInUse[type].IsBitSet(uiCurrentIndex))
      {
        uiCurrentIndex++;
      }
      ref_resourceBinding[sName].m_iSlot = static_cast<plInt16>(uiCurrentIndex);
      indexInUse[type].SetCount(plMath::Max(indexInUse[type].GetCount(), uiCurrentIndex + 1));
      indexInUse[type].SetBit(uiCurrentIndex);
    }
  }

  return PLASMA_SUCCESS;
}

void plShaderCompilerHLSL::CreateShaderResourceDeclaration(plStringView sDeclaration, const plShaderResourceBinding& binding, plStringBuilder& out_sDeclaration)
{
  PLASMA_ASSERT_DEBUG(binding.m_iSet == 0, "HLSL: error X3721: space is only supported for shader targets 5.1 and higher");
  const plEnum<plGALShaderResourceType> type = plGALShaderResourceType::MakeFromShaderDescriptorType(binding.m_DescriptorType);
  plStringView sResourcePrefix;
  switch (type)
  {
    case plGALShaderResourceType::Sampler:
      sResourcePrefix = "s"_plsv;
      break;
    case plGALShaderResourceType::ConstantBuffer:
      sResourcePrefix = "b"_plsv;
      break;
    case plGALShaderResourceType::SRV:
      sResourcePrefix = "t"_plsv;
      break;
    case plGALShaderResourceType::UAV:
      sResourcePrefix = "u"_plsv;
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      break;
  }
  out_sDeclaration.Format("{} : register({}{})", sDeclaration, sResourcePrefix, binding.m_iSlot);
}

void plShaderCompilerHLSL::Initialize()
{
  if (m_VertexInputMapping.IsEmpty())
  {
    m_VertexInputMapping["POSITION"] = plGALVertexAttributeSemantic::Position;
    m_VertexInputMapping["NORMAL"] = plGALVertexAttributeSemantic::Normal;
    m_VertexInputMapping["TANGENT"] = plGALVertexAttributeSemantic::Tangent;
    m_VertexInputMapping["COLOR"] = plGALVertexAttributeSemantic::Color0;
    m_VertexInputMapping["TEXCOORD"] = plGALVertexAttributeSemantic::TexCoord0;
    m_VertexInputMapping["BITANGENT"] = plGALVertexAttributeSemantic::BiTangent;
    m_VertexInputMapping["BONEINDICES"] = plGALVertexAttributeSemantic::BoneIndices0;
    m_VertexInputMapping["BONEWEIGHTS"] = plGALVertexAttributeSemantic::BoneWeights0;
  }
}

plGALResourceFormat::Enum plShaderCompilerHLSL::GetPLASMAFormat(const _D3D11_SIGNATURE_PARAMETER_DESC& paramDesc)
{
  plUInt32 uiComponents = plMath::Log2i(paramDesc.Mask + 1);
  switch (paramDesc.ComponentType)
  {
    case D3D_REGISTER_COMPONENT_UNKNOWN:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      break;
    case D3D_REGISTER_COMPONENT_UINT32:
      switch (uiComponents)
      {
        case 1:
          return plGALResourceFormat::RUInt;
        case 2:
          return plGALResourceFormat::RGUInt;
        case 3:
          return plGALResourceFormat::RGBUInt;
        case 4:
          return plGALResourceFormat::RGBAUInt;
        default:
          PLASMA_ASSERT_NOT_IMPLEMENTED;
          break;
      }
      break;
    case D3D_REGISTER_COMPONENT_SINT32:
      switch (uiComponents)
      {
        case 1:
          return plGALResourceFormat::RInt;
        case 2:
          return plGALResourceFormat::RGInt;
        case 3:
          return plGALResourceFormat::RGBInt;
        case 4:
          return plGALResourceFormat::RGBAInt;
        default:
          PLASMA_ASSERT_NOT_IMPLEMENTED;
          break;
      }
      break;
    case D3D_REGISTER_COMPONENT_FLOAT32:
      switch (uiComponents)
      {
        case 1:
          return plGALResourceFormat::RFloat;
        case 2:
          return plGALResourceFormat::RGFloat;
        case 3:
          return plGALResourceFormat::RGBFloat;
        case 4:
          return plGALResourceFormat::RGBAFloat;
        default:
          PLASMA_ASSERT_NOT_IMPLEMENTED;
          break;
      }
      break;
  }
  return plGALResourceFormat::Invalid;
}
