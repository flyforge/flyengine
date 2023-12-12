PLASMA_ALWAYS_INLINE const vk::DescriptorImageInfo& plGALUnorderedAccessViewVulkan::GetImageInfo() const
{
  return m_resourceImageInfo;
}

PLASMA_ALWAYS_INLINE vk::ImageSubresourceRange plGALUnorderedAccessViewVulkan::GetRange() const
{
  return m_range;
}
