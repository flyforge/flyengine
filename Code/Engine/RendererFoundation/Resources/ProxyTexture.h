
#pragma once

#include <RendererFoundation/Resources/Texture.h>

class PLASMA_RENDERERFOUNDATION_DLL plGALProxyTexture : public plGALTexture
{
public:
  virtual ~plGALProxyTexture();

  virtual const plGALResourceBase* GetParentResource() const override;

protected:
  friend class plGALDevice;

  plGALProxyTexture(const plGALTexture& parentTexture);

  virtual plResult InitPlatform(plGALDevice* pDevice, plArrayPtr<plGALSystemMemoryDescription> pInitialData) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  const plGALTexture* m_pParentTexture;
};
