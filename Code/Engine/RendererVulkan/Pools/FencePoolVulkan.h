#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Simple pool for fences
///
/// Do not call ReclaimFence manually, instead call plGALDeviceVulkan::ReclaimLater which will make sure to reclaim the fence once it is no longer in use.
/// Fences are reclaimed once the frame in plGALDeviceVulkan is reused (currently 4 frames are in rotation). Do not call resetFences, this is already done by ReclaimFence.
/// Usage:
/// \code{.cpp}
///   vk::Fence f = plFencePoolVulkan::RequestFence();
///   <insert fence somewhere>
///   <wait for fence>
///   plGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(f);
/// \endcode
class PL_RENDERERVULKAN_DLL plFencePoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();

  static vk::Fence RequestFence();
  static void ReclaimFence(vk::Fence& fence);

private:
  static plHybridArray<vk::Fence, 4> s_Fences;
  static vk::Device s_device;
};
