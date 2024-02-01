
PL_ALWAYS_INLINE const vk::PipelineColorBlendStateCreateInfo* plGALBlendStateVulkan::GetBlendState() const
{
  return &m_blendState;
}

PL_ALWAYS_INLINE const vk::PipelineDepthStencilStateCreateInfo* plGALDepthStencilStateVulkan::GetDepthStencilState() const
{
  return &m_depthStencilState;
}

PL_ALWAYS_INLINE const vk::PipelineRasterizationStateCreateInfo* plGALRasterizerStateVulkan::GetRasterizerState() const
{
  return &m_rasterizerState;
}

PL_ALWAYS_INLINE const vk::DescriptorImageInfo& plGALSamplerStateVulkan::GetImageInfo() const
{
  return m_resourceImageInfo;
}
