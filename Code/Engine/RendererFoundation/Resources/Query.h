#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class PLASMA_RENDERERFOUNDATION_DLL plGALQuery : public plGALResource<plGALQueryCreationDescription>
{
public:
protected:
  friend class plGALDevice;
  friend class plGALCommandEncoder;

  plGALQuery(const plGALQueryCreationDescription& Description);

  virtual ~plGALQuery();

  virtual plResult InitPlatform(plGALDevice* pDevice) = 0;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;

  bool m_bStarted;
};
