#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Simple pool for command buffers
///
/// Do not call ReclaimCommandBuffer manually, instead call plGALDeviceVulkan::ReclaimLater which will make sure to reclaim the command buffer once it is no longer in use.
/// Usage:
/// \code{.cpp}
///   vk::CommandBuffer c = pPool->RequestCommandBuffer();
///   c.begin();
///   ...
///   c.end();
///   plGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(c);
/// \endcode
class PL_RENDERERVULKAN_DLL plCommandBufferPoolVulkan
{
public:
  ~plCommandBufferPoolVulkan();

  void Initialize(vk::Device device, plUInt32 graphicsFamilyIndex);
  void DeInitialize();

  vk::CommandBuffer RequestCommandBuffer();
  void ReclaimCommandBuffer(vk::CommandBuffer& CommandBuffer);

private:
  vk::Device m_device;
  vk::CommandPool m_commandPool;
  plHybridArray<vk::CommandBuffer, 4> m_CommandBuffers;
};
