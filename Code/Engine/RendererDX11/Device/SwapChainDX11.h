
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>

struct IDXGISwapChain;

class plGALSwapChainDX11 : public plGALWindowSwapChain
{
public:
  virtual void AcquireNextRenderTarget(plGALDevice* pDevice) override;
  virtual void PresentRenderTarget(plGALDevice* pDevice) override;
  virtual plResult UpdateSwapChain(plGALDevice* pDevice, plEnum<plGALPresentMode> newPresentMode) override;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALSwapChainDX11(const plGALWindowSwapChainCreationDescription& Description);

  virtual ~plGALSwapChainDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;
  plResult CreateBackBufferInternal(plGALDeviceDX11* pDXDevice);
  void DestroyBackBufferInternal(plGALDeviceDX11* pDXDevice);
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;


  IDXGISwapChain* m_pDXSwapChain = nullptr;

  plGALTextureHandle m_hBackBufferTexture;

  plEnum<plGALPresentMode> m_CurrentPresentMode;
  bool m_bCanMakeDirectScreenshots = true;
  // We can't do screenshots if we're using any of the FLIP swap effects.
  // If the user requests screenshots anyways, we need to put another buffer in between.
  // For ease of use, this is m_hBackBufferTexture and the actual "OS backbuffer" is this texture.
  // In any other case this handle is unused.
  plGALTextureHandle m_hActualBackBufferTexture;
};

#include <RendererDX11/Device/Implementation/SwapChainDX11_inl.h>
