#pragma once

#include <RendererFoundation/Resources/Texture.h>


struct ID3D11Resource;

class plGALTextureDX11 : public plGALTexture
{
public:
  PLASMA_ALWAYS_INLINE ID3D11Resource* GetDXTexture() const;

  PLASMA_ALWAYS_INLINE ID3D11Resource* GetDXStagingTexture() const;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALTextureDX11(const plGALTextureCreationDescription& Description);

  ~plGALTextureDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice, plArrayPtr<plGALSystemMemoryDescription> pInitialData) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  plResult CreateStagingTexture(plGALDeviceDX11* pDevice);

  ID3D11Resource* m_pDXTexture;

  ID3D11Resource* m_pDXStagingTexture;

  void* m_pExisitingNativeObject = nullptr;
};

#include <RendererDX11/Resources/Implementation/TextureDX11_inl.h>
