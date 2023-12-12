
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

#include <vulkan/vulkan.hpp>

class plGALBufferVulkan;

class plGALUnorderedAccessViewVulkan : public plGALUnorderedAccessView
{
public:
  PLASMA_ALWAYS_INLINE const vk::DescriptorImageInfo& GetImageInfo() const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;
  vk::ImageSubresourceRange GetRange() const;

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALUnorderedAccessViewVulkan(plGALResourceBase* pResource, const plGALUnorderedAccessViewCreationDescription& Description);
  ~plGALUnorderedAccessViewVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

private:
  mutable vk::DescriptorImageInfo m_resourceImageInfo;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  vk::ImageSubresourceRange m_range;
};

#include <RendererVulkan/Resources/Implementation/UnorderedAccessViewVulkan_inl.h>
