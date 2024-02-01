
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

#include <vulkan/vulkan.hpp>

class plGALBufferVulkan;

class plGALUnorderedAccessViewVulkan : public plGALUnorderedAccessView
{
public:
  PL_ALWAYS_INLINE const vk::DescriptorImageInfo& GetImageInfo() const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;
  const vk::BufferView& GetBufferView() const;
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
  vk::BufferView m_bufferView;
  vk::ImageSubresourceRange m_range;
};

#include <RendererVulkan/Resources/Implementation/UnorderedAccessViewVulkan_inl.h>
