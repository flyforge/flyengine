#pragma once

#include <GameEngine/XR/XRSwapChain.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

class plOpenXR;

PLASMA_DEFINE_AS_POD_TYPE(XrSwapchainImageD3D11KHR);

class PLASMA_OPENXRPLUGIN_DLL plGALOpenXRSwapChain : public plGALXRSwapChain
{
public:
  plSizeU32 GetRenderTargetSize() const { return m_CurrentSize; }
  XrSwapchain GetColorSwapchain() const { return m_colorSwapchain.handle; }
  XrSwapchain GetDepthSwapchain() const { return m_depthSwapchain.handle; }

  virtual void AcquireNextRenderTarget(plGALDevice* pDevice) override;
  virtual void PresentRenderTarget(plGALDevice* pDevice) override;
  void PresentRenderTarget() const;

protected:
  virtual plResult InitPlatform(plGALDevice* pDevice) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

private:
  friend class plOpenXR;
  struct Swapchain
  {
    XrSwapchain handle = 0;
    int64_t format = 0;
    plUInt32 imageCount = 0;
    XrSwapchainImageBaseHeader* images = nullptr;
    uint32_t imageIndex = 0;
  };
  enum class SwapchainType
  {
    Color,
    Depth,
  };

private:
  plGALOpenXRSwapChain(plOpenXR* pXrInterface, plGALMSAASampleCount::Enum msaaCount);
  XrResult SelectSwapchainFormat(int64_t& colorFormat, int64_t& depthFormat);
  XrResult CreateSwapchainImages(Swapchain& swapchain, SwapchainType type);
  XrResult InitSwapChain(plGALMSAASampleCount::Enum msaaCount);
  void DeinitSwapChain();

private:
  XrInstance m_instance = XR_NULL_HANDLE;
  uint64_t m_systemId = XR_NULL_SYSTEM_ID;
  XrSession m_session = XR_NULL_HANDLE;
  plEnum<plGALMSAASampleCount> m_msaaCount;

  // Swapchain
  XrViewConfigurationView m_primaryConfigView;
  Swapchain m_colorSwapchain;
  Swapchain m_depthSwapchain;

  plHybridArray<XrSwapchainImageD3D11KHR, 3> m_colorSwapChainImagesD3D11;
  plHybridArray<XrSwapchainImageD3D11KHR, 3> m_depthSwapChainImagesD3D11;
  plHybridArray<plGALTextureHandle, 3> m_hColorRTs;
  plHybridArray<plGALTextureHandle, 3> m_hDepthRTs;

  bool m_bImageAcquired = false;
  plGALTextureHandle m_hColorRT;
  plGALTextureHandle m_hDepthRT;
};
