PL_ALWAYS_INLINE const vk::DescriptorImageInfo& plGALResourceViewVulkan::GetImageInfo(bool bIsArray) const
{
  PL_ASSERT_DEBUG((bIsArray ? m_resourceImageInfoArray : m_resourceImageInfo).imageView, "View does not support bIsArray: {}", bIsArray);
  return bIsArray ? m_resourceImageInfoArray : m_resourceImageInfo;
}

PL_ALWAYS_INLINE vk::ImageSubresourceRange plGALResourceViewVulkan::GetRange() const
{
  return m_range;
}

PL_ALWAYS_INLINE const vk::BufferView& plGALResourceViewVulkan::GetBufferView() const
{
  return m_bufferView;
}
