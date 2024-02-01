#pragma once

#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class plGALDeviceVulkan;

struct plStagingBufferVulkan
{
  vk::Buffer m_buffer;
  plVulkanAllocation m_alloc;
  plVulkanAllocationInfo m_allocInfo;
};

class PL_RENDERERVULKAN_DLL plStagingBufferPoolVulkan
{
public:
  void Initialize(plGALDeviceVulkan* pDevice);
  void DeInitialize();

  plStagingBufferVulkan AllocateBuffer(vk::DeviceSize alignment, vk::DeviceSize size);
  void ReclaimBuffer(plStagingBufferVulkan& buffer);

private:
  plGALDeviceVulkan* m_pDevice = nullptr;
  vk::Device m_device;
};
