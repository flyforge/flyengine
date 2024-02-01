#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>

/// \brief Description of a shared texture swap chain. Use plGALSharedTextureSwapChain::Create to create instance.
struct plGALSharedTextureSwapChainCreationDescription : public plHashableStruct<plGALSharedTextureSwapChainCreationDescription>
{
  plGALTextureCreationDescription m_TextureDesc;
  plHybridArray<plGALPlatformSharedHandle, 3> m_Textures;
  /// \brief Called when rendering to a swap chain texture has been submitted to the GPU queue. Use this to get the new semaphore value of the previously armed texture.
  plDelegate<void(plUInt32 uiTextureIndex, plUInt64 uiSemaphoreValue)> m_OnPresent;
};

/// \brief Use to render to a set of shared textures.
/// To use it, it needs to be armed with the next shared texture index and its current semaphore value.
class PL_RENDERERFOUNDATION_DLL plGALSharedTextureSwapChain : public plGALSwapChain
{
  friend class plGALDevice;

public:
  using Functor = plDelegate<plGALSwapChainHandle(const plGALSharedTextureSwapChainCreationDescription&)>;
  static void SetFactoryMethod(Functor factory);

  /// \brief Creates an instance of a plGALSharedTextureSwapChain.
  static plGALSwapChainHandle Create(const plGALSharedTextureSwapChainCreationDescription& desc);

public:
  /// \brief Call this before rendering.
  /// \param uiTextureIndex Texture to render into.
  /// \param uiCurrentSemaphoreValue Current semaphore value of the texture.
  void Arm(plUInt32 uiTextureIndex, plUInt64 uiCurrentSemaphoreValue);

protected:
  plGALSharedTextureSwapChain(const plGALSharedTextureSwapChainCreationDescription& desc);
  virtual void AcquireNextRenderTarget(plGALDevice* pDevice) override;
  virtual void PresentRenderTarget(plGALDevice* pDevice) override;
  virtual plResult UpdateSwapChain(plGALDevice* pDevice, plEnum<plGALPresentMode> newPresentMode) override;
  virtual plResult InitPlatform(plGALDevice* pDevice) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

protected:
  static Functor s_Factory;

protected:
  plUInt32 m_uiCurrentTexture = plMath::MaxValue<plUInt32>();
  plUInt64 m_uiCurrentSemaphoreValue = 0;
  plHybridArray<plGALTextureHandle, 3> m_SharedTextureHandles;
  plHybridArray<const plGALSharedTexture*, 3> m_SharedTextureInterfaces;
  plHybridArray<plUInt64, 3> m_CurrentSemaphoreValue;
  plGALSharedTextureSwapChainCreationDescription m_Desc = {};
};
PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERFOUNDATION_DLL, plGALSharedTextureSwapChain);
