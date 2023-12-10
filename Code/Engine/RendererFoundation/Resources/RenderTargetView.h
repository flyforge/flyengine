
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class PLASMA_RENDERERFOUNDATION_DLL plGALRenderTargetView : public plGALObject<plGALRenderTargetViewCreationDescription>
{
public:
  PLASMA_ALWAYS_INLINE plGALTexture* GetTexture() const { return m_pTexture; }

protected:
  friend class plGALDevice;

  plGALRenderTargetView(plGALTexture* pTexture, const plGALRenderTargetViewCreationDescription& description);

  virtual ~plGALRenderTargetView();

  virtual plResult InitPlatform(plGALDevice* pDevice) = 0;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;

  plGALTexture* m_pTexture;
};
