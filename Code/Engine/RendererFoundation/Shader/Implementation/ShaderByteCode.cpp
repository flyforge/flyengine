#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>
#include <RendererFoundation/Shader/Types.h>

plUInt32 plShaderConstant::s_TypeSize[(plUInt32)Type::ENUM_COUNT] = {0, sizeof(float) * 1, sizeof(float) * 2, sizeof(float) * 3, sizeof(float) * 4, sizeof(int) * 1, sizeof(int) * 2, sizeof(int) * 3, sizeof(int) * 4, sizeof(plUInt32) * 1, sizeof(plUInt32) * 2,
  sizeof(plUInt32) * 3, sizeof(plUInt32) * 4, sizeof(plShaderMat3), sizeof(plMat4), sizeof(plShaderTransform), sizeof(plShaderBool)};

void plShaderConstant::CopyDataFormVariant(plUInt8* pDest, plVariant* pValue) const
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

plResult plShaderResourceBinding::CreateMergedShaderResourceBinding(const plArrayPtr<plArrayPtr<const plShaderResourceBinding>>& resourcesPerStage, plDynamicArray<plShaderResourceBinding>& out_bindings)
{
  plUInt32 uiSize = 0;
  for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    uiSize += resourcesPerStage[stage].GetCount();
  }
  
  out_bindings.Clear();
  out_bindings.Reserve(uiSize);

  plMap<plHashedString, plUInt32> nameToIndex;
  plMap<plHashedString, plUInt32> samplerToIndex;
  for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    for (const plShaderResourceBinding& res : resourcesPerStage[stage])
    {
      plHashedString sName = res.m_sName;

      plUInt32 uiIndex = plInvalidIndex;
      if (res.m_DescriptorType == plGALShaderDescriptorType::Sampler)
      {
        // #TODO_SHADER Samplers are special! Since the shader compiler edits the reflection data and renames "*_AutoSampler" to just "*", we generate a naming collision between the texture and the sampler. See plRenderContext::BindTexture2D for binding code. For now, we allow this collision, but it will probably bite us later on.
        samplerToIndex.TryGetValue(res.m_sName, uiIndex);
      }
      else
      {
        nameToIndex.TryGetValue(res.m_sName, uiIndex);
      }

      if (uiIndex != plInvalidIndex)
      {
        plShaderResourceBinding& current = out_bindings[uiIndex];
        if (current.m_DescriptorType != res.m_DescriptorType || current.m_TextureType != res.m_TextureType || current.m_uiArraySize != res.m_uiArraySize || current.m_iSet != res.m_iSet || current.m_iSlot != res.m_iSlot)
        {
          // #TODO_SHADER better error reporting.
          plLog::Error("A shared shader resource '{}' has a mismatching signatures between stages", sName);
          return PLASMA_FAILURE;
        }

        current.m_Stages |= plGALShaderStageFlags::MakeFromShaderStage((plGALShaderStage::Enum)stage);
      }
      else
      {
        plShaderResourceBinding& newBinding = out_bindings.ExpandAndGetRef();
        newBinding = res;
        newBinding.m_Stages |= plGALShaderStageFlags::MakeFromShaderStage((plGALShaderStage::Enum)stage);
        if (res.m_DescriptorType == plGALShaderDescriptorType::Sampler)
        {
          samplerToIndex[res.m_sName] = out_bindings.GetCount() - 1;
        }
        else
        {
          nameToIndex[res.m_sName] = out_bindings.GetCount() - 1;
        }
      }
    }
  }
  return PLASMA_SUCCESS;
}

plGALShaderByteCode::plGALShaderByteCode() = default;

plGALShaderByteCode::~plGALShaderByteCode()
{
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

PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_ShaderByteCode);
