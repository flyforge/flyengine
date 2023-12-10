
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class PLASMA_RENDERERFOUNDATION_DLL plGALBuffer : public plGALResource<plGALBufferCreationDescription>
{
public:
  PLASMA_ALWAYS_INLINE plUInt32 GetSize() const;

protected:
  friend class plGALDevice;

  plGALBuffer(const plGALBufferCreationDescription& Description);

  virtual ~plGALBuffer();

  virtual plResult InitPlatform(plGALDevice* pDevice, plArrayPtr<const plUInt8> pInitialData) = 0;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;
};

#include <RendererFoundation/Resources/Implementation/Buffer_inl.h>
