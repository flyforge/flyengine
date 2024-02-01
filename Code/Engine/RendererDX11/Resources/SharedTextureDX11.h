#pragma once

#include <RendererDX11/Resources/TextureDX11.h>

struct IDXGIKeyedMutex;

class plGALSharedTextureDX11 : public plGALTextureDX11, public plGALSharedTexture
{
  using SUPER = plGALTextureDX11;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALSharedTextureDX11(const plGALTextureCreationDescription& Description, plEnum<plGALSharedTextureType> sharedType, plGALPlatformSharedHandle hSharedHandle);
  ~plGALSharedTextureDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice, plArrayPtr<plGALSystemMemoryDescription> pInitialData) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  virtual plGALPlatformSharedHandle GetSharedHandle() const override;
  virtual void WaitSemaphoreGPU(plUInt64 uiValue) const override;
  virtual void SignalSemaphoreGPU(plUInt64 uiValue) const override;

protected:
  plEnum<plGALSharedTextureType> m_SharedType = plGALSharedTextureType::None;
  plGALPlatformSharedHandle m_hSharedHandle;
  IDXGIKeyedMutex* m_pKeyedMutex = nullptr;
};
