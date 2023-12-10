#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>

#include <RendererVulkan/Device/DeviceVulkan.h>

void plStagingBufferPoolVulkan::Initialize(plGALDeviceVulkan* pDevice)
{
  m_pDevice = pDevice;
  m_device = pDevice->GetVulkanDevice();
}

void plStagingBufferPoolVulkan::DeInitialize()
{
  m_device = nullptr;
}

plStagingBufferVulkan plStagingBufferPoolVulkan::AllocateBuffer(vk::DeviceSize alignment, vk::DeviceSize size)
{
  //#TODO_VULKAN alignment
  plStagingBufferVulkan buffer;

  PLASMA_ASSERT_DEBUG(m_device, "plStagingBufferPoolVulkan::Initialize not called");
  vk::BufferCreateInfo bufferCreateInfo = {};
  bufferCreateInfo.size = size;
  bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

  bufferCreateInfo.pQueueFamilyIndices = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;


  plVulkanAllocationCreateInfo allocInfo;
  allocInfo.m_usage = plVulkanMemoryUsage::Auto;
  allocInfo.m_flags = plVulkanAllocationCreateFlags::HostAccessSequentialWrite;

  VK_ASSERT_DEV(plMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocInfo, buffer.m_buffer, buffer.m_alloc, &buffer.m_allocInfo));

  return buffer;
}

void plStagingBufferPoolVulkan::ReclaimBuffer(plStagingBufferVulkan& buffer)
{
  m_pDevice->DeleteLater(buffer.m_buffer, buffer.m_alloc);

  //PLASMA_ASSERT_DEBUG(m_device, "plStagingBufferPoolVulkan::Initialize not called");
  //plMemoryAllocatorVulkan::DestroyBuffer(buffer.m_buffer, buffer.m_alloc);
}
