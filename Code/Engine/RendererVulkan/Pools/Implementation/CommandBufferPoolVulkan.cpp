#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/CommandBufferPoolVulkan.h>

void plCommandBufferPoolVulkan::Initialize(vk::Device device, plUInt32 graphicsFamilyIndex)
{
  m_device = device;

  // Command buffer
  vk::CommandPoolCreateInfo commandPoolCreateInfo = {};
  commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  commandPoolCreateInfo.queueFamilyIndex = graphicsFamilyIndex;

  m_commandPool = m_device.createCommandPool(commandPoolCreateInfo);
}

void plCommandBufferPoolVulkan::DeInitialize()
{
  for (vk::CommandBuffer& commandBuffer : m_CommandBuffers)
  {
    m_device.freeCommandBuffers(m_commandPool, 1, &commandBuffer);
  }
  m_CommandBuffers.Clear();
  m_CommandBuffers.Compact();

  m_device.destroyCommandPool(m_commandPool);
  m_commandPool = nullptr;

  m_device = nullptr;
}

vk::CommandBuffer plCommandBufferPoolVulkan::RequestCommandBuffer()
{
  PLASMA_ASSERT_DEBUG(m_device, "plCommandBufferPoolVulkan::Initialize not called");
  if (!m_CommandBuffers.IsEmpty())
  {
    vk::CommandBuffer CommandBuffer = m_CommandBuffers.PeekBack();
    m_CommandBuffers.PopBack();
    return CommandBuffer;
  }
  else
  {
    vk::CommandBuffer commandBuffer;

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.commandPool = m_commandPool;
    commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;

    VK_ASSERT_DEV(m_device.allocateCommandBuffers(&commandBufferAllocateInfo, &commandBuffer));
    return commandBuffer;
  }
}

void plCommandBufferPoolVulkan::ReclaimCommandBuffer(vk::CommandBuffer& commandBuffer)
{
  PLASMA_ASSERT_DEBUG(m_device, "plCommandBufferPoolVulkan::Initialize not called");
  commandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
  m_CommandBuffers.PushBack(commandBuffer);
}
