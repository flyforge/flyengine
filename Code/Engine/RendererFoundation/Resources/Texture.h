
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class PLASMA_RENDERERFOUNDATION_DLL plGALTexture : public plGALResource<plGALTextureCreationDescription>
{
public:
protected:
  friend class plGALDevice;

  plGALTexture(const plGALTextureCreationDescription& Description);

  virtual ~plGALTexture();

  virtual plResult InitPlatform(plGALDevice* pDevice, plArrayPtr<plGALSystemMemoryDescription> pInitialData) = 0;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;
};

/// \brief Optional interface for plGALTexture if it was created via plGALDevice::CreateSharedTexture.
/// A plGALTexture can be a shared texture, but doesn't have to be. Access through plGALDevice::GetSharedTexture.
class PLASMA_RENDERERFOUNDATION_DLL plGALSharedTexture
{
public:
  /// \brief Returns the handle that can be used to open this texture on another device / process. Call  plGALDevice::OpenSharedTexture to do so.
  virtual plGALPlatformSharedHandle GetSharedHandle() const = 0;
  /// \brief Before the current render pipeline is executed, the GPU will wait for the semaphore to have the given value.
  /// \param iValue Value the semaphore needs to have before the texture can be used.
  virtual void WaitSemaphoreGPU(plUInt64 uiValue) const = 0;
  /// \brief Once the current render pipeline is done on the GPU, the semaphore will be signaled with the given value.
  /// \param iValue Value the semaphore is set to once we are done using the texture (after the current render pipeline).
  virtual void SignalSemaphoreGPU(plUInt64 uiValue) const = 0;
};
