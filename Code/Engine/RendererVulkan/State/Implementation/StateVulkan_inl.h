
PLASMA_ALWAYS_INLINE const vk::PipelineColorBlendStateCreateInfo* plGALBlendStateVulkan::GetBlendState() const
{
  return &m_blendState;
}

PLASMA_ALWAYS_INLINE const vk::PipelineDepthStencilStateCreateInfo* plGALDepthStencilStateVulkan::GetDepthStencilState() const
{
  return &m_depthStencilState;
}

PLASMA_ALWAYS_INLINE const vk::PipelineRasterizationStateCreateInfo* plGALRasterizerStateVulkan::GetRasterizerState() const
{
  return &m_rasterizerState;
}

PLASMA_ALWAYS_INLINE const vk::DescriptorImageInfo& plGALSamplerStateVulkan::GetImageInfo() const
{
  return m_resourceImageInfo;
}
