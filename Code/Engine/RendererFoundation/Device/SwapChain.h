
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Math/Size.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class PL_RENDERERFOUNDATION_DLL plGALSwapChain : public plGALObject<plGALSwapChainCreationDescription>
{
public:
  const plGALRenderTargets& GetRenderTargets() const { return m_RenderTargets; }
  plGALTextureHandle GetBackBufferTexture() const { return m_RenderTargets.m_hRTs[0]; }
  plSizeU32 GetCurrentSize() const { return m_CurrentSize; }

  virtual void AcquireNextRenderTarget(plGALDevice* pDevice) = 0;
  virtual void PresentRenderTarget(plGALDevice* pDevice) = 0;
  virtual plResult UpdateSwapChain(plGALDevice* pDevice, plEnum<plGALPresentMode> newPresentMode) = 0;

  virtual ~plGALSwapChain();

protected:
  friend class plGALDevice;

  plGALSwapChain(const plRTTI* pSwapChainType);

  virtual plResult InitPlatform(plGALDevice* pDevice) = 0;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;

  plGALRenderTargets m_RenderTargets;
  plSizeU32 m_CurrentSize = {};
};
PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERFOUNDATION_DLL, plGALSwapChain);


class PL_RENDERERFOUNDATION_DLL plGALWindowSwapChain : public plGALSwapChain
{
public:
  using Functor = plDelegate<plGALSwapChainHandle(const plGALWindowSwapChainCreationDescription&)>;
  static void SetFactoryMethod(Functor factory);

  static plGALSwapChainHandle Create(const plGALWindowSwapChainCreationDescription& desc);

public:
  const plGALWindowSwapChainCreationDescription& GetWindowDescription() const { return m_WindowDesc; }

protected:
  plGALWindowSwapChain(const plGALWindowSwapChainCreationDescription& Description);

protected:
  static Functor s_Factory;

protected:
  plGALWindowSwapChainCreationDescription m_WindowDesc;
};
PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERFOUNDATION_DLL, plGALWindowSwapChain);

#include <RendererFoundation/Device/Implementation/SwapChain_inl.h>
