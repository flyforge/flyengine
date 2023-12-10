
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class PLASMA_RENDERERFOUNDATION_DLL plGALResourceView : public plGALObject<plGALResourceViewCreationDescription>
{
public:
  PLASMA_ALWAYS_INLINE plGALResourceBase* GetResource() const { return m_pResource; }

protected:
  friend class plGALDevice;

  plGALResourceView(plGALResourceBase* pResource, const plGALResourceViewCreationDescription& description);

  virtual ~plGALResourceView();

  virtual plResult InitPlatform(plGALDevice* pDevice) = 0;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;

  plGALResourceBase* m_pResource;
};
