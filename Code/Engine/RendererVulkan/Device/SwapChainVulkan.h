
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class plGALDeviceVulkan;

class plGALSwapChainVulkan : public plGALWindowSwapChain
{
public:
  virtual void AcquireNextRenderTarget(plGALDevice* pDevice) override;
  virtual void PresentRenderTarget(plGALDevice* pDevice) override;
  virtual plResult UpdateSwapChain(plGALDevice* pDevice, plEnum<plGALPresentMode> newPresentMode) override;

  PLASMA_ALWAYS_INLINE vk::SwapchainKHR GetVulkanSwapChain() const;

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALSwapChainVulkan(const plGALWindowSwapChainCreationDescription& Description);

  virtual ~plGALSwapChainVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;
  plResult CreateSwapChainInternal();
  void DestroySwapChainInternal(plGALDeviceVulkan* pVulkanDevice);
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

protected:
  plGALDeviceVulkan* m_pVulkanDevice = nullptr;
  plEnum<plGALPresentMode> m_currentPresentMode;

  vk::SurfaceKHR m_vulkanSurface;
  vk::SwapchainKHR m_vulkanSwapChain;
  plHybridArray<vk::Image, 3> m_swapChainImages;
  plHybridArray<plGALTextureHandle, 3> m_swapChainTextures;
  plHybridArray<vk::Fence, 3> m_swapChainImageInUseFences;
  plUInt32 m_uiCurrentSwapChainImage = 0;

  vk::Semaphore m_currentPipelineImageAvailableSemaphore;
};

#include <RendererVulkan/Device/Implementation/SwapChainVulkan_inl.h>
