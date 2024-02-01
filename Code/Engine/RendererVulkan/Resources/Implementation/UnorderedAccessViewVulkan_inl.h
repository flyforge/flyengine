
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>

PL_ALWAYS_INLINE const vk::DescriptorImageInfo& plGALUnorderedAccessViewVulkan::GetImageInfo() const
{
  return m_resourceImageInfo;
}

PL_ALWAYS_INLINE vk::ImageSubresourceRange plGALUnorderedAccessViewVulkan::GetRange() const
{
  return m_range;
}

PL_ALWAYS_INLINE const vk::BufferView& plGALUnorderedAccessViewVulkan::GetBufferView() const
{
  return m_bufferView;
}
