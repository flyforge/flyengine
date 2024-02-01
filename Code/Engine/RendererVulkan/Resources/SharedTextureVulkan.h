#pragma once

#include <RendererVulkan/Resources/TextureVulkan.h>

class plGALSharedTextureVulkan : public plGALTextureVulkan, public plGALSharedTexture
{
  using SUPER = plGALTextureVulkan;

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALSharedTextureVulkan(const plGALTextureCreationDescription& Description, plEnum<plGALSharedTextureType> sharedType, plGALPlatformSharedHandle hSharedHandle);
  ~plGALSharedTextureVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice, plArrayPtr<plGALSystemMemoryDescription> pInitialData) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  virtual plGALPlatformSharedHandle GetSharedHandle() const override;
  virtual void WaitSemaphoreGPU(plUInt64 uiValue) const override;
  virtual void SignalSemaphoreGPU(plUInt64 uiValue) const override;

protected:
  plEnum<plGALSharedTextureType> m_SharedType = plGALSharedTextureType::None;
  plGALPlatformSharedHandle m_hSharedHandle;
  vk::Semaphore m_SharedSemaphore;
};
