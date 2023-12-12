
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
