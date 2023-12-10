#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Simple pool for semaphores
///
/// Do not call ReclaimSemaphore manually, instead call plGALDeviceVulkan::ReclaimLater which will make sure to reclaim the semaphore once it is no longer in use.
/// Usage:
/// \code{.cpp}
///   vk::Semaphore s = plSemaphorePoolVulkan::RequestSemaphore();
///   ...
///   plGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(s);
/// \endcode
class PLASMA_RENDERERVULKAN_DLL plSemaphorePoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();

  static vk::Semaphore RequestSemaphore();
  static void ReclaimSemaphore(vk::Semaphore& semaphore);

private:
  static plHybridArray<vk::Semaphore, 4> s_semaphores;
  static vk::Device s_device;
};
