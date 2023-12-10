
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

#include <vulkan/vulkan.hpp>

class plGALBufferVulkan;
class plGALTextureVulkan;

class plGALResourceViewVulkan : public plGALResourceView
{
public:
  const vk::DescriptorImageInfo& GetImageInfo(bool bIsArray) const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;
  vk::ImageSubresourceRange GetRange() const;
  const vk::BufferView& GetBufferView() const;

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALResourceViewVulkan(plGALResourceBase* pResource, const plGALResourceViewCreationDescription& Description);
  ~plGALResourceViewVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  vk::ImageSubresourceRange m_range;
  mutable vk::DescriptorImageInfo m_resourceImageInfo;
  mutable vk::DescriptorImageInfo m_resourceImageInfoArray;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  vk::BufferView m_bufferView;
};

#include <RendererVulkan/Resources/Implementation/ResourceViewVulkan_inl.h>
