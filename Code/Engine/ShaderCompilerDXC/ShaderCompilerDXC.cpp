#include <ShaderCompilerDXC/ShaderCompilerDXC.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringConversion.h>

#include <ShaderCompilerDXC/SpirvMetaData.h>
#include <spirv_reflect.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <d3dcompiler.h>
#endif

#include <dxc/dxcapi.h>

template <typename T>
struct plComPtr
{
public:
  plComPtr() {}
  ~plComPtr()
  {
    if (m_ptr != nullptr)
    {
      m_ptr->Release();
      m_ptr = nullptr;
    }
  }

  plComPtr(const plComPtr& other)
    : m_ptr(other.m_ptr)
  {
    if (m_ptr)
    {
      m_ptr->AddRef();
    }
  }

  T* operator->() { return m_ptr; }
  T* const operator->() const { return m_ptr; }

  T** put()
  {
    PLASMA_ASSERT_DEV(m_ptr == nullptr, "Can only put into an empty plComPtr");
    return &m_ptr;
  }

  bool operator==(nullptr_t)
  {
    return m_ptr == nullptr;
  }

  bool operator!=(nullptr_t)
  {
    return m_ptr != nullptr;
  }

private:
  T* m_ptr = nullptr;
};

plComPtr<IDxcUtils> s_pDxcUtils;
plComPtr<IDxcCompiler3> s_pDxcCompiler;

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(ShaderCompilerDXC, ShaderCompilerDXCPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(s_pDxcUtils.put()));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(s_pDxcCompiler.put()));
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pDxcUtils = {};
    s_pDxcCompiler = {};
  }

PLASMA_END_SUBSYSTEM_DECLARATION;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plShaderCompilerDXC, 1, plRTTIDefaultAllocator<plShaderCompilerDXC>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static plResult CompileVulkanShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, plDynamicArray<plUInt8>& out_ByteCode);

static const char* GetProfileName(plStringView sPlatform, plGALShaderStage::Enum Stage)
{
  if (sPlatform == "VULKAN")
  {
    switch (Stage)
    {
      case plGALShaderStage::VertexShader:
        return "vs_6_0";
      case plGALShaderStage::HullShader:
        return "hs_6_0";
      case plGALShaderStage::DomainShader:
        return "ds_6_0";
      case plGALShaderStage::GeometryShader:
        return "gs_6_0";
      case plGALShaderStage::PixelShader:
        return "ps_6_0";
      case plGALShaderStage::ComputeShader:
        return "cs_6_0";
      default:
        break;
    }
  }

  PLASMA_REPORT_FAILURE("Unknown Platform '{}' or Stage {}", sPlatform, Stage);
  return "";
}

plResult plShaderCompilerDXC::Initialize()
{
  if (m_VertexInputMapping.IsEmpty())
  {
    m_VertexInputMapping["in.var.POSITION"] = plGALVertexAttributeSemantic::Position;
    m_VertexInputMapping["in.var.NORMAL"] = plGALVertexAttributeSemantic::Normal;
    m_VertexInputMapping["in.var.TANGENT"] = plGALVertexAttributeSemantic::Tangent;

    m_VertexInputMapping["in.var.COLOR0"] = plGALVertexAttributeSemantic::Color0;
    m_VertexInputMapping["in.var.COLOR1"] = plGALVertexAttributeSemantic::Color1;
    m_VertexInputMapping["in.var.COLOR2"] = plGALVertexAttributeSemantic::Color2;
    m_VertexInputMapping["in.var.COLOR3"] = plGALVertexAttributeSemantic::Color3;
    m_VertexInputMapping["in.var.COLOR4"] = plGALVertexAttributeSemantic::Color4;
    m_VertexInputMapping["in.var.COLOR5"] = plGALVertexAttributeSemantic::Color5;
    m_VertexInputMapping["in.var.COLOR6"] = plGALVertexAttributeSemantic::Color6;
    m_VertexInputMapping["in.var.COLOR7"] = plGALVertexAttributeSemantic::Color7;

    m_VertexInputMapping["in.var.TEXCOORD0"] = plGALVertexAttributeSemantic::TexCoord0;
    m_VertexInputMapping["in.var.TEXCOORD1"] = plGALVertexAttributeSemantic::TexCoord1;
    m_VertexInputMapping["in.var.TEXCOORD2"] = plGALVertexAttributeSemantic::TexCoord2;
    m_VertexInputMapping["in.var.TEXCOORD3"] = plGALVertexAttributeSemantic::TexCoord3;
    m_VertexInputMapping["in.var.TEXCOORD4"] = plGALVertexAttributeSemantic::TexCoord4;
    m_VertexInputMapping["in.var.TEXCOORD5"] = plGALVertexAttributeSemantic::TexCoord5;
    m_VertexInputMapping["in.var.TEXCOORD6"] = plGALVertexAttributeSemantic::TexCoord6;
    m_VertexInputMapping["in.var.TEXCOORD7"] = plGALVertexAttributeSemantic::TexCoord7;
    m_VertexInputMapping["in.var.TEXCOORD8"] = plGALVertexAttributeSemantic::TexCoord8;
    m_VertexInputMapping["in.var.TEXCOORD9"] = plGALVertexAttributeSemantic::TexCoord9;

    m_VertexInputMapping["in.var.BITANGENT"] = plGALVertexAttributeSemantic::BiTangent;
    m_VertexInputMapping["in.var.BONEINDICES0"] = plGALVertexAttributeSemantic::BoneIndices0;
    m_VertexInputMapping["in.var.BONEINDICES1"] = plGALVertexAttributeSemantic::BoneIndices1;
    m_VertexInputMapping["in.var.BONEWEIGHTS0"] = plGALVertexAttributeSemantic::BoneWeights0;
    m_VertexInputMapping["in.var.BONEWEIGHTS1"] = plGALVertexAttributeSemantic::BoneWeights1;
  }

  PLASMA_ASSERT_DEV(s_pDxcUtils != nullptr && s_pDxcCompiler != nullptr, "ShaderCompiler SubSystem init should have initialized library pointers.");
  return PLASMA_SUCCESS;
}

plResult plShaderCompilerDXC::Compile(plShaderProgramData& inout_Data, plLogInterface* pLog)
{
  PLASMA_SUCCEED_OR_RETURN(Initialize());

  for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_Data.m_uiSourceHash[stage] == 0)
      continue;
    if (inout_Data.m_bWriteToDisk[stage] == false)
    {
      plLog::Debug("Shader for stage '{}' is already compiled.", plGALShaderStage::Names[stage]);
      continue;
    }

    const plStringBuilder sShaderSource = inout_Data.m_sShaderSource[stage];

    if (!sShaderSource.IsEmpty() && sShaderSource.FindSubString("main") != nullptr)
    {
      const plStringBuilder sSourceFile = inout_Data.m_sSourceFile;

      if (CompileVulkanShader(sSourceFile, sShaderSource, inout_Data.m_Flags.IsSet(plShaderCompilerFlags::Debug), GetProfileName(inout_Data.m_sPlatform, (plGALShaderStage::Enum)stage), "main", inout_Data.m_ByteCode[stage]->m_ByteCode).Succeeded())
      {
        PLASMA_SUCCEED_OR_RETURN(ReflectShaderStage(inout_Data, (plGALShaderStage::Enum)stage));
      }
      else
      {
        return PLASMA_FAILURE;
      }
    }
  }

  return PLASMA_SUCCESS;
}

plResult CompileVulkanShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, plDynamicArray<plUInt8>& out_ByteCode)
{
  out_ByteCode.Clear();

  const char* szCompileSource = szSource;
  plStringBuilder sDebugSource;

  plDynamicArray<plStringWChar> args;
  args.PushBack(plStringWChar(szFile));
  args.PushBack(L"-E");
  args.PushBack(plStringWChar(szEntryPoint));
  args.PushBack(L"-T");
  args.PushBack(plStringWChar(szProfile));
  args.PushBack(L"-spirv");
  args.PushBack(L"-fvk-use-dx-position-w");
  args.PushBack(L"-fspv-target-env=vulkan1.1");

  if (bDebug)
  {
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = szSource;
    sDebugSource.ReplaceAll("#line ", "//ine ");
    szCompileSource = sDebugSource;

    // plLog::Warning("Vulkan DEBUG shader support not really implemented.");

    args.PushBack(L"-Zi"); // Enable debug information.
    // args.PushBack(L"-Fo"); // Optional. Stored in the pdb.
    // args.PushBack(L"myshader.bin");
    // args.PushBack(L"-Fd"); // The file name of the pdb.
    // args.PushBack(L"myshader.pdb");
  }

  plComPtr<IDxcBlobEncoding> pSource;
  s_pDxcUtils->CreateBlob(szCompileSource, (UINT32)strlen(szCompileSource), DXC_CP_UTF8, pSource.put());

  DxcBuffer Source;
  Source.Ptr = pSource->GetBufferPointer();
  Source.Size = pSource->GetBufferSize();
  Source.Encoding = DXC_CP_UTF8;

  plHybridArray<LPCWSTR, 16> pszArgs;
  pszArgs.SetCount(args.GetCount());
  for (plUInt32 i = 0; i < args.GetCount(); ++i)
  {
    pszArgs[i] = args[i].GetData();
  }

  plComPtr<IDxcResult> pResults;
  s_pDxcCompiler->Compile(&Source, pszArgs.GetData(), pszArgs.GetCount(), nullptr, IID_PPV_ARGS(pResults.put()));

  plComPtr<IDxcBlobUtf8> pErrors;
  pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.put()), nullptr);

  HRESULT hrStatus;
  pResults->GetStatus(&hrStatus);
  if (FAILED(hrStatus))
  {
    plLog::Error("Vulkan shader compilation failed.");

    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      plLog::Error("{}", plStringUtf8(pErrors->GetStringPointer()).GetData());
    }

    return PLASMA_FAILURE;
  }
  else
  {
    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      plLog::Warning("{}", plStringUtf8(pErrors->GetStringPointer()).GetData());
    }
  }

  plComPtr<IDxcBlob> pShader;
  plComPtr<IDxcBlobWide> pShaderName;
  pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(pShader.put()), pShaderName.put());

  if (pShader == nullptr)
  {
    plLog::Error("No Vulkan bytecode was generated.");
    return PLASMA_FAILURE;
  }

  out_ByteCode.SetCountUninitialized(static_cast<plUInt32>(pShader->GetBufferSize()));

  plMemoryUtils::Copy(out_ByteCode.GetData(), reinterpret_cast<plUInt8*>(pShader->GetBufferPointer()), out_ByteCode.GetCount());

  return PLASMA_SUCCESS;
}

plResult plShaderCompilerDXC::DefineShaderResourceBindings(plShaderProgramData& inout_data, plHashTable<plHashedString, plShaderResourceBinding>& ref_resourceBinding, plLogInterface* pLog)
{
  plHybridArray<plHybridBitfield<64>, 4> indexInUse;
  for (auto it : ref_resourceBinding)
  {
    plInt16& iSet = it.Value().m_iSet;
    if (iSet == -1)
      iSet = 0;

    indexInUse.EnsureCount(iSet + 1);

    if (it.Value().m_iSlot != -1)
    {
      indexInUse[iSet].SetCount(plMath::Max(indexInUse[iSet].GetCount(), static_cast<plUInt32>(it.Value().m_iSlot + 1)));
      indexInUse[iSet].SetBit(it.Value().m_iSlot);
    }
  }

  // Create stable order of resources
  plHybridArray<plHybridArray<plHashedString, 16>, 4> order;
  order.SetCount(indexInUse.GetCount());
  for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_data.m_sShaderSource[stage].IsEmpty())
      continue;

    for (const auto& res : inout_data.m_Resources[stage])
    {
      const plInt16 iSet = res.m_Binding.m_iSet < 0 ? (plInt16)0 : res.m_Binding.m_iSet;
      if (!order[iSet].Contains(res.m_Binding.m_sName))
      {
        order[iSet].PushBack(res.m_Binding.m_sName);
      }
    }
  }

  //
  for (plInt16 iSet = 0; iSet < (plInt16)indexInUse.GetCount(); ++iSet)
  {
    plUInt32 uiCurrentIndex = 0;
    for (const auto& sName : order[iSet])
    {
      plInt16& iSlot = ref_resourceBinding[sName].m_iSlot;
      if (iSlot != -1)
        continue;
      while (uiCurrentIndex < indexInUse[iSet].GetCount() && indexInUse[iSet].IsBitSet(uiCurrentIndex))
      {
        uiCurrentIndex++;
      }
      iSlot = static_cast<plInt16>(uiCurrentIndex);
      indexInUse[iSet].SetCount(plMath::Max(indexInUse[iSet].GetCount(), uiCurrentIndex + 1));
      indexInUse[iSet].SetBit(uiCurrentIndex);
    }
  }

  return PLASMA_SUCCESS;
}

void plShaderCompilerDXC::CreateShaderResourceDeclaration(plStringView sDeclaration, const plShaderResourceBinding& binding, plStringBuilder& out_sDeclaration)
{
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
  out_sDeclaration.Format("{} : register({}{}, space{})", sDeclaration, sResourcePrefix, binding.m_iSlot, binding.m_iSet);
}

plResult plShaderCompilerDXC::FillResourceBinding(plGALShaderByteCode& shaderBinary, plShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV)
  {
    return FillSRVResourceBinding(shaderBinary, binding, info);
  }

  else if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_UAV)
  {
    return FillUAVResourceBinding(shaderBinary, binding, info);
  }

  else if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_CBV)
  {
    binding.m_DescriptorType = plGALShaderDescriptorType::ConstantBuffer;
    binding.m_pLayout = ReflectConstantBufferLayout(shaderBinary, info);

    return PLASMA_SUCCESS;
  }

  else if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
  {
    binding.m_DescriptorType = plGALShaderDescriptorType::Sampler;

    // TODO: not sure how this will map to Vulkan
    if (binding.m_sName.GetString().EndsWith("_AutoSampler"))
    {
      plStringBuilder sb = binding.m_sName.GetString();
      sb.TrimWordEnd("_AutoSampler");
      binding.m_sName.Assign(sb);
    }

    return PLASMA_SUCCESS;
  }

  plLog::Error("Resource '{}': Unsupported resource type.", info.name);
  return PLASMA_FAILURE;
}

plGALShaderTextureType::Enum plShaderCompilerDXC::GetTextureType(const SpvReflectDescriptorBinding& info)
{
  switch (info.image.dim)
  {
    case SpvDim::SpvDim1D:
    {
      if (info.image.ms == 0)
      {
        if (info.image.arrayed > 0)
        {
          return plGALShaderTextureType::Texture1DArray;
        }
        else
        {
          return plGALShaderTextureType::Texture1D;
        }
      }

      break;
    }

    case SpvDim::SpvDim2D:
    {
      if (info.image.ms == 0)
      {
        if (info.image.arrayed > 0)
        {
          return plGALShaderTextureType::Texture2DArray;
        }
        else
        {
          return plGALShaderTextureType::Texture2D;
        }
      }
      else
      {
        if (info.image.arrayed > 0)
        {
          return plGALShaderTextureType::Texture2DMSArray;
        }
        else
        {
          return plGALShaderTextureType::Texture2DMS;
        }
      }

      break;
    }

    case SpvDim::SpvDim3D:
    {
      if (info.image.ms == 0 && info.image.arrayed == 0)
      {
        return plGALShaderTextureType::Texture3D;
      }

      break;
    }

    case SpvDim::SpvDimCube:
    {
      if (info.image.ms == 0)
      {
        if (info.image.arrayed == 0)
        {
          return plGALShaderTextureType::TextureCube;
        }
        else
        {
          return plGALShaderTextureType::TextureCubeArray;
        }
      }

      break;
    }

    case SpvDim::SpvDimBuffer:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      //binding.m_TextureType = plGALShaderTextureType::GenericBuffer;
      return plGALShaderTextureType::Unknown;

    case SpvDim::SpvDimRect:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      return plGALShaderTextureType::Unknown;

    case SpvDim::SpvDimSubpassData:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      return plGALShaderTextureType::Unknown;

    case SpvDim::SpvDimMax:
      PLASMA_ASSERT_DEV(false, "Invalid enum value");
      break;
  }
  return plGALShaderTextureType::Unknown;
}

plResult plShaderCompilerDXC::FillSRVResourceBinding(plGALShaderByteCode& shaderBinary, plShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
  {
    if (info.type_description->op == SpvOp::SpvOpTypeStruct)
    {
      binding.m_DescriptorType = plGALShaderDescriptorType::StructuredBuffer;
      return PLASMA_SUCCESS;
    }
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
  {
    binding.m_DescriptorType = plGALShaderDescriptorType::Texture;
    binding.m_TextureType = GetTextureType(info);
    return PLASMA_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_DescriptorType = plGALShaderDescriptorType::TexelBuffer;
      return PLASMA_SUCCESS;
    }

    plLog::Error("Resource '{}': Unsupported texel buffer SRV type.", info.name);
    return PLASMA_FAILURE;
  }

  plLog::Error("Resource '{}': Unsupported SRV type.", info.name);
  return PLASMA_FAILURE;
}

plResult plShaderCompilerDXC::FillUAVResourceBinding(plGALShaderByteCode& shaderBinary, plShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE)
  {
    binding.m_DescriptorType = plGALShaderDescriptorType::TextureRW;
    binding.m_TextureType = GetTextureType(info);
    return PLASMA_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_DescriptorType = plGALShaderDescriptorType::TexelBufferRW;
      return PLASMA_SUCCESS;
    }

    plLog::Error("Resource '{}': Unsupported texel buffer UAV type.", info.name);
    return PLASMA_FAILURE;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_DescriptorType = plGALShaderDescriptorType::StructuredBufferRW;
      return PLASMA_SUCCESS;
    }
    else if (info.image.dim == SpvDim::SpvDim1D)
    {
      binding.m_DescriptorType = plGALShaderDescriptorType::StructuredBufferRW;
      return PLASMA_SUCCESS;
    }

    plLog::Error("Resource '{}': Unsupported storage buffer UAV type.", info.name);
    return PLASMA_FAILURE;
  }
  plLog::Error("Resource '{}': Unsupported UAV type.", info.name);
  return PLASMA_FAILURE;
}

plGALResourceFormat::Enum GetPLASMAFormat(SpvReflectFormat format)
{
  switch (format)
  {
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_UINT:
      return plGALResourceFormat::RUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SINT:
      return plGALResourceFormat::RInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SFLOAT:
      return plGALResourceFormat::RFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_UINT:
      return plGALResourceFormat::RGUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SINT:
      return plGALResourceFormat::RGInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SFLOAT:
      return plGALResourceFormat::RGFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_UINT:
      return plGALResourceFormat::RGBUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SINT:
      return plGALResourceFormat::RGBInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
      return plGALResourceFormat::RGBFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
      return plGALResourceFormat::RGBAUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
      return plGALResourceFormat::RGBAInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
      return plGALResourceFormat::RGBAFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_UNDEFINED:
    default:
      return plGALResourceFormat::Invalid;
  }
}

plResult plShaderCompilerDXC::ReflectShaderStage(plShaderProgramData& inout_Data, plGALShaderStage::Enum Stage)
{
  PLASMA_LOG_BLOCK("ReflectShaderStage", inout_Data.m_sSourceFile);

  plGALShaderByteCode* pShader = inout_Data.m_ByteCode[Stage];
  auto& bytecode = pShader->m_ByteCode;

  SpvReflectShaderModule module;

  if (spvReflectCreateShaderModule(bytecode.GetCount(), bytecode.GetData(), &module) != SPV_REFLECT_RESULT_SUCCESS)
  {
    plLog::Error("Extracting shader reflection information failed.");
    return PLASMA_FAILURE;
  }

  PLASMA_SCOPE_EXIT(spvReflectDestroyShaderModule(&module));

  //
  auto& vertexInputAttributes = pShader->m_ShaderVertexInput;
  if (Stage == plGALShaderStage::VertexShader)
  {
    plUInt32 uiNumVars = 0;
    if (spvReflectEnumerateInputVariables(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      plLog::Error("Failed to retrieve number of input variables.");
      return PLASMA_FAILURE;
    }
    plDynamicArray<SpvReflectInterfaceVariable*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumerateInputVariables(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      plLog::Error("Failed to retrieve input variables.");
      return PLASMA_FAILURE;
    }

    vertexInputAttributes.Reserve(vars.GetCount());

    for (plUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      SpvReflectInterfaceVariable* pVar = vars[i];
      if (pVar->name != nullptr)
      {
        plShaderVertexInputAttribute& attr = vertexInputAttributes.ExpandAndGetRef();
        attr.m_uiLocation = static_cast<plUInt8>(pVar->location);

        plGALVertexAttributeSemantic::Enum* pVAS = m_VertexInputMapping.GetValue(pVar->name);
        PLASMA_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input sematic found: {}", pVar->name);
        attr.m_eSemantic = *pVAS;
        attr.m_eFormat = GetPLASMAFormat(pVar->format);
        PLASMA_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input format found: {}", pVar->format);
      }
    }
  }

  // descriptor bindings
  {
    plUInt32 uiNumVars = 0;
    if (spvReflectEnumerateDescriptorBindings(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      plLog::Error("Failed to retrieve number of descriptor bindings.");
      return PLASMA_FAILURE;
    }

    plDynamicArray<SpvReflectDescriptorBinding*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumerateDescriptorBindings(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      plLog::Error("Failed to retrieve descriptor bindings.");
      return PLASMA_FAILURE;
    }

    for (plUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      auto& info = *vars[i];

      plLog::Info("Bound Resource: '{}' at slot {} (Count: {})", info.name, info.binding, info.count);

      plShaderResourceBinding shaderResourceBinding;
      shaderResourceBinding.m_iSet = static_cast<plInt16>(info.set);
      shaderResourceBinding.m_iSlot = static_cast<plInt16>(info.binding);
      shaderResourceBinding.m_uiArraySize = info.count;
      shaderResourceBinding.m_sName.Assign(info.name);
      shaderResourceBinding.m_Stages = plGALShaderStageFlags::MakeFromShaderStage(Stage);

      if (FillResourceBinding(*inout_Data.m_ByteCode[Stage], shaderResourceBinding, info).Failed())
        continue;

      PLASMA_ASSERT_DEV(shaderResourceBinding.m_DescriptorType != plGALShaderDescriptorType::Unknown, "FillResourceBinding should have failed.");

      inout_Data.m_ByteCode[Stage]->m_ShaderResourceBindings.PushBack(shaderResourceBinding);
    }
  }
  return PLASMA_SUCCESS;
}

plShaderConstantBufferLayout* plShaderCompilerDXC::ReflectConstantBufferLayout(plGALShaderByteCode& pStageBinary, const SpvReflectDescriptorBinding& constantBufferReflection)
{
  const auto& block = constantBufferReflection.block;

  PLASMA_LOG_BLOCK("Constant Buffer Layout", constantBufferReflection.name);
  plLog::Debug("Constant Buffer has {} variables, Size is {}", block.member_count, block.padded_size);

  plShaderConstantBufferLayout* pLayout = PLASMA_DEFAULT_NEW(plShaderConstantBufferLayout);

  pLayout->m_uiTotalSize = block.padded_size;

  for (plUInt32 var = 0; var < block.member_count; ++var)
  {
    const auto& svd = block.members[var];

    plShaderConstant constant;
    constant.m_sName.Assign(svd.name);
    constant.m_uiOffset = svd.offset; // TODO: or svd.absolute_offset ??
    constant.m_uiArrayElements = 1;

    plUInt32 uiFlags = svd.type_description->type_flags;

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY;

      if (svd.array.dims_count != 1)
      {
        plLog::Error("Variable '{}': Multi-dimensional arrays are not supported.", constant.m_sName);
        continue;
      }

      constant.m_uiArrayElements = svd.array.dims[0];
    }

    plUInt32 uiComponents = 0;

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR;

      uiComponents = svd.numeric.vector.component_count;
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL;

      // TODO: unfortunately this never seems to be set, 'bool' types are always exposed as 'int'
      PLASMA_ASSERT_NOT_IMPLEMENTED;

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = plShaderConstant::Type::Bool;
          break;

        default:
          plLog::Error("Variable '{}': Multi-component bools are not supported.", constant.m_sName);
          continue;
      }
    }
    else if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT;

      // TODO: there doesn't seem to be a way to detect 'unsigned' types

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = plShaderConstant::Type::Int1;
          break;
        case 2:
          constant.m_Type = plShaderConstant::Type::Int2;
          break;
        case 3:
          constant.m_Type = plShaderConstant::Type::Int3;
          break;
        case 4:
          constant.m_Type = plShaderConstant::Type::Int4;
          break;
      }
    }
    else if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT;

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = plShaderConstant::Type::Float1;
          break;
        case 2:
          constant.m_Type = plShaderConstant::Type::Float2;
          break;
        case 3:
          constant.m_Type = plShaderConstant::Type::Float3;
          break;
        case 4:
          constant.m_Type = plShaderConstant::Type::Float4;
          break;
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX;

      constant.m_Type = plShaderConstant::Type::Default;

      const plUInt32 rows = svd.type_description->traits.numeric.matrix.row_count;
      const plUInt32 columns = svd.type_description->traits.numeric.matrix.column_count;

      if ((svd.type_description->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT) == 0)
      {
        plLog::Error("Variable '{}': Only float matrices are supported", constant.m_sName);
        continue;
      }

      if (columns == 3 && rows == 3)
      {
        constant.m_Type = plShaderConstant::Type::Mat3x3;
      }
      else if (columns == 4 && rows == 4)
      {
        constant.m_Type = plShaderConstant::Type::Mat4x4;
      }
      else
      {
        plLog::Error("Variable '{}': {}x{} matrices are not supported", constant.m_sName, rows, columns);
        continue;
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT;
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK;

      constant.m_Type = plShaderConstant::Type::Struct;
    }

    if (uiFlags != 0)
    {
      plLog::Error("Variable '{}': Unknown additional type flags '{}'", constant.m_sName, uiFlags);
    }

    if (constant.m_Type == plShaderConstant::Type::Default)
    {
      plLog::Error("Variable '{}': Variable type is unknown / not supported", constant.m_sName);
      continue;
    }

    const char* typeNames[] = {
      "Default",
      "Float1",
      "Float2",
      "Float3",
      "Float4",
      "Int1",
      "Int2",
      "Int3",
      "Int4",
      "UInt1",
      "UInt2",
      "UInt3",
      "UInt4",
      "Mat3x3",
      "Mat4x4",
      "Transform",
      "Bool",
      "Struct",
    };

    if (constant.m_uiArrayElements > 1)
    {
      plLog::Info("{1} {3}[{2}] {0}", constant.m_sName, plArgU(constant.m_uiOffset, 3, true), constant.m_uiArrayElements, typeNames[constant.m_Type]);
    }
    else
    {
      plLog::Info("{1} {3} {0}", constant.m_sName, plArgU(constant.m_uiOffset, 3, true), constant.m_uiArrayElements, typeNames[constant.m_Type]);
    }

    pLayout->m_Constants.PushBack(constant);
  }

  return pLayout;
}