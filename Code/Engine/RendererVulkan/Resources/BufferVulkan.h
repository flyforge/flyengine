
#pragma once

#include <RendererFoundation/Resources/Buffer.h>

#include <RendererVulkan/Device/DeviceVulkan.h>

#include <vulkan/vulkan.hpp>

class PL_RENDERERVULKAN_DLL plGALBufferVulkan : public plGALBuffer
{
public:
  void DiscardBuffer() const;
  PL_ALWAYS_INLINE vk::Buffer GetVkBuffer() const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;

  PL_ALWAYS_INLINE vk::IndexType GetIndexType() const;
  PL_ALWAYS_INLINE plVulkanAllocation GetAllocation() const;
  PL_ALWAYS_INLINE const plVulkanAllocationInfo& GetAllocationInfo() const;
  PL_ALWAYS_INLINE vk::PipelineStageFlags GetUsedByPipelineStage() const;
  PL_ALWAYS_INLINE vk::AccessFlags GetAccessMask() const;
  static vk::DeviceSize GetAlignment(const plGALDeviceVulkan* pDevice, vk::BufferUsageFlags usage);

protected:
  struct BufferVulkan
  {
    vk::Buffer m_buffer;
    plVulkanAllocation m_alloc;
    mutable plUInt64 m_currentFrame = 0;
  };

  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALBufferVulkan(const plGALBufferCreationDescription& Description, bool bCPU = false);

  virtual ~plGALBufferVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice, plArrayPtr<const plUInt8> pInitialData) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;
  void CreateBuffer() const;

  mutable BufferVulkan m_currentBuffer;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  mutable plDeque<BufferVulkan> m_usedBuffers;
  mutable plVulkanAllocationInfo m_allocInfo;

  // Data for memory barriers and access
  vk::PipelineStageFlags m_stages = {};
  vk::AccessFlags m_access = {};
  vk::IndexType m_indexType = vk::IndexType::eUint16; // Only applicable for index buffers
  vk::BufferUsageFlags m_usage = {};
  vk::DeviceSize m_size = 0;

  plGALDeviceVulkan* m_pDeviceVulkan = nullptr;
  vk::Device m_device;

  bool m_bCPU = false;
  mutable plString m_sDebugName;
};

#include <RendererVulkan/Resources/Implementation/BufferVulkan_inl.h>
