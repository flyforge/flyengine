#include <RendererFoundation/Resources/ResourceFormats.h>

namespace
{
  bool IsArrayViewInternal(const plGALTextureCreationDescription& texDesc, const plGALResourceViewCreationDescription& viewDesc)
  {
    return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
  }
  bool IsArrayViewInternal(const plGALTextureCreationDescription& texDesc, const plGALUnorderedAccessViewCreationDescription& viewDesc)
  {
    return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
  }
} // namespace

PLASMA_ALWAYS_INLINE vk::SampleCountFlagBits plConversionUtilsVulkan::GetSamples(plEnum<plGALMSAASampleCount> samples)
{
  switch (samples)
  {
    case plGALMSAASampleCount::None:
      return vk::SampleCountFlagBits::e1;
    case plGALMSAASampleCount::TwoSamples:
      return vk::SampleCountFlagBits::e2;
    case plGALMSAASampleCount::FourSamples:
      return vk::SampleCountFlagBits::e4;
    case plGALMSAASampleCount::EightSamples:
      return vk::SampleCountFlagBits::e8;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      return vk::SampleCountFlagBits::e1;
  }
}

PLASMA_ALWAYS_INLINE vk::PresentModeKHR plConversionUtilsVulkan::GetPresentMode(plEnum<plGALPresentMode> presentMode, const plDynamicArray<vk::PresentModeKHR>& supportedModes)
{
  switch (presentMode)
  {
    case plGALPresentMode::Immediate:
    {
      if (supportedModes.Contains(vk::PresentModeKHR::eImmediate))
        return vk::PresentModeKHR::eImmediate;
      else if (supportedModes.Contains(vk::PresentModeKHR::eMailbox))
        return vk::PresentModeKHR::eMailbox;
      else
        return vk::PresentModeKHR::eFifo;
    }
    case plGALPresentMode::VSync:
      return vk::PresentModeKHR::eFifo; // FIFO must be supported according to the standard.
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      return vk::PresentModeKHR::eFifo;
  }
}

PLASMA_ALWAYS_INLINE vk::ImageSubresourceRange plConversionUtilsVulkan::GetSubresourceRange(const plGALTextureCreationDescription& texDesc, const plGALRenderTargetViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;
  plGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == plGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask = plGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  range.setBaseMipLevel(viewDesc.m_uiMipLevel).setLevelCount(1).setBaseArrayLayer(viewDesc.m_uiFirstSlice).setLayerCount(viewDesc.m_uiSliceCount);
  return range;
}

PLASMA_ALWAYS_INLINE vk::ImageSubresourceRange plConversionUtilsVulkan::GetSubresourceRange(const plGALTextureCreationDescription& texDesc, const plGALResourceViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;

  const bool bIsArrayView = IsArrayViewInternal(texDesc, viewDesc);

  plGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == plGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask = plGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (viewFormat == plGALResourceFormat::D24S8)
  {
    range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
  }
  range.baseMipLevel = viewDesc.m_uiMostDetailedMipLevel;
  range.levelCount = plMath::Min(viewDesc.m_uiMipLevelsToUse, texDesc.m_uiMipLevelCount - range.baseMipLevel);

  switch (texDesc.m_Type)
  {
    case plGALTextureType::Texture2D:
    case plGALTextureType::Texture2DProxy:
      range.layerCount = viewDesc.m_uiArraySize;
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case plGALTextureType::TextureCube:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      range.layerCount = viewDesc.m_uiArraySize * 6;
      break;
    case plGALTextureType::Texture3D:
      range.layerCount = 1;
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }
  return range;
}


PLASMA_ALWAYS_INLINE vk::ImageSubresourceRange plConversionUtilsVulkan::GetSubresourceRange(const plGALTextureCreationDescription& texDesc, const plGALUnorderedAccessViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;

  const bool bIsArrayView = IsArrayViewInternal(texDesc, viewDesc);

  plGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == plGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask = plGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (viewFormat == plGALResourceFormat::D24S8)
  {
    range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
  }

  range.baseMipLevel = viewDesc.m_uiMipLevelToUse;
  range.levelCount = 1;
  range.layerCount = viewDesc.m_uiArraySize;

  switch (texDesc.m_Type)
  {
    case plGALTextureType::Texture2D:
    case plGALTextureType::Texture2DProxy:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case plGALTextureType::TextureCube:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case plGALTextureType::Texture3D:
      if (bIsArrayView)
      {
        PLASMA_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      }
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }
  return range;
}

PLASMA_ALWAYS_INLINE vk::ImageSubresourceRange plConversionUtilsVulkan::GetSubresourceRange(
  const vk::ImageSubresourceLayers& layers)
{
  vk::ImageSubresourceRange range;
  range.aspectMask = layers.aspectMask;
  range.baseMipLevel = layers.mipLevel;
  range.levelCount = 1;
  range.baseArrayLayer = layers.baseArrayLayer;
  range.layerCount = layers.layerCount;
  return range;
}

PLASMA_ALWAYS_INLINE vk::ImageViewType plConversionUtilsVulkan::GetImageViewType(plEnum<plGALTextureType> texType, bool bIsArrayView)
{
  switch (texType)
  {
    case plGALTextureType::Texture2D:
    case plGALTextureType::Texture2DProxy:
      if (!bIsArrayView)
      {
        return vk::ImageViewType::e2D;
      }
      else
      {
        return vk::ImageViewType::e2DArray;
      }
    case plGALTextureType::TextureCube:
      if (!bIsArrayView)
      {
        return vk::ImageViewType::eCube;
      }
      else
      {
        return vk::ImageViewType::eCubeArray;
      }
    case plGALTextureType::Texture3D:
      return vk::ImageViewType::e3D;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      return vk::ImageViewType::e1D;
  }
}

PLASMA_ALWAYS_INLINE bool plConversionUtilsVulkan::IsDepthFormat(vk::Format format)
{
  switch (format)
  {
    case vk::Format::eD16Unorm:
    case vk::Format::eD32Sfloat:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
      return true;
    default:
      return false;
  }
}

PLASMA_ALWAYS_INLINE bool plConversionUtilsVulkan::IsStencilFormat(vk::Format format)
{
  switch (format)
  {
    case vk::Format::eS8Uint:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
      return true;
    default:
      return false;
  }
}

PLASMA_ALWAYS_INLINE vk::PrimitiveTopology plConversionUtilsVulkan::GetPrimitiveTopology(plEnum<plGALPrimitiveTopology> topology)
{
  switch (topology)
  {
    case plGALPrimitiveTopology::Points:
      return vk::PrimitiveTopology::ePointList;
    case plGALPrimitiveTopology::Lines:
      return vk::PrimitiveTopology::eLineList;
    case plGALPrimitiveTopology::Triangles:
      return vk::PrimitiveTopology::eTriangleList;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      return vk::PrimitiveTopology::ePointList;
  }
}

PLASMA_ALWAYS_INLINE vk::ShaderStageFlagBits plConversionUtilsVulkan::GetShaderStage(plGALShaderStage::Enum stage)
{
  switch (stage)
  {
    case plGALShaderStage::VertexShader:
      return vk::ShaderStageFlagBits::eVertex;
    case plGALShaderStage::HullShader:
      return vk::ShaderStageFlagBits::eTessellationControl;
    case plGALShaderStage::DomainShader:
      return vk::ShaderStageFlagBits::eTessellationEvaluation;
    case plGALShaderStage::GeometryShader:
      return vk::ShaderStageFlagBits::eGeometry;
    case plGALShaderStage::PixelShader:
      return vk::ShaderStageFlagBits::eFragment;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      [[fallthrough]];
    case plGALShaderStage::ComputeShader:
      return vk::ShaderStageFlagBits::eCompute;
  }
}

PLASMA_ALWAYS_INLINE vk::PipelineStageFlags plConversionUtilsVulkan::GetPipelineStage(plGALShaderStage::Enum stage)
{
  switch (stage)
  {
    case plGALShaderStage::VertexShader:
      return vk::PipelineStageFlagBits::eVertexShader;
    case plGALShaderStage::HullShader:
      return vk::PipelineStageFlagBits::eTessellationControlShader;
    case plGALShaderStage::DomainShader:
      return vk::PipelineStageFlagBits::eTessellationEvaluationShader;
    case plGALShaderStage::GeometryShader:
      return vk::PipelineStageFlagBits::eGeometryShader;
    case plGALShaderStage::PixelShader:
      return vk::PipelineStageFlagBits::eFragmentShader;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      [[fallthrough]];
    case plGALShaderStage::ComputeShader:
      return vk::PipelineStageFlagBits::eComputeShader;
  }
}

PLASMA_ALWAYS_INLINE vk::PipelineStageFlags plConversionUtilsVulkan::GetPipelineStage(vk::ShaderStageFlags flags)
{
  vk::PipelineStageFlags res;
  if (flags & vk::ShaderStageFlagBits::eVertex)
    res |= vk::PipelineStageFlagBits::eVertexShader;
  if (flags & vk::ShaderStageFlagBits::eTessellationControl)
    res |= vk::PipelineStageFlagBits::eTessellationControlShader;
  if (flags & vk::ShaderStageFlagBits::eTessellationEvaluation)
    res |= vk::PipelineStageFlagBits::eTessellationEvaluationShader;
  if (flags & vk::ShaderStageFlagBits::eGeometry)
    res |= vk::PipelineStageFlagBits::eGeometryShader;
  if (flags & vk::ShaderStageFlagBits::eFragment)
    res |= vk::PipelineStageFlagBits::eFragmentShader;
  if (flags & vk::ShaderStageFlagBits::eCompute)
    res |= vk::PipelineStageFlagBits::eComputeShader;
  if(flags & vk::ShaderStageFlagBits::eTaskEXT)
    res |= vk::PipelineStageFlagBits::eTaskShaderEXT;
  if(flags & vk::ShaderStageFlagBits::eMeshEXT)
    res |= vk::PipelineStageFlagBits::eMeshShaderEXT;
  if(flags & vk::ShaderStageFlagBits::eRaygenKHR ||
       flags & vk::ShaderStageFlagBits::eAnyHitKHR ||
       flags & vk::ShaderStageFlagBits::eClosestHitKHR ||
       flags & vk::ShaderStageFlagBits::eMissKHR ||
       flags & vk::ShaderStageFlagBits::eIntersectionKHR)
  {
    res |= vk::PipelineStageFlagBits::eRayTracingShaderKHR;
  }

  return res;
}
