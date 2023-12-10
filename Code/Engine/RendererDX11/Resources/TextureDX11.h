#pragma once

#include <RendererFoundation/Resources/Texture.h>

struct ID3D11Resource;
struct D3D11_TEXTURE2D_DESC;
struct D3D11_TEXTURE3D_DESC;
struct D3D11_SUBRESOURCE_DATA;
class plGALDeviceDX11;

PLASMA_DEFINE_AS_POD_TYPE(D3D11_SUBRESOURCE_DATA);

class plGALTextureDX11 : public plGALTexture
{
public:
  PLASMA_ALWAYS_INLINE ID3D11Resource* GetDXTexture() const;

  PLASMA_ALWAYS_INLINE ID3D11Resource* GetDXStagingTexture() const;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;
  friend class plGALSharedTextureDX11;

  plGALTextureDX11(const plGALTextureCreationDescription& Description);
  ~plGALTextureDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice, plArrayPtr<plGALSystemMemoryDescription> pInitialData) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

  plResult InitFromNativeObject(plGALDeviceDX11* pDXDevice);
  static plResult Create2DDesc(const plGALTextureCreationDescription& description, plGALDeviceDX11* pDXDevice, D3D11_TEXTURE2D_DESC& out_Tex2DDesc);
  static plResult Create3DDesc(const plGALTextureCreationDescription& description, plGALDeviceDX11* pDXDevice, D3D11_TEXTURE3D_DESC& out_Tex3DDesc);
  static void ConvertInitialData(const plGALTextureCreationDescription& description, plArrayPtr<plGALSystemMemoryDescription> pInitialData, plHybridArray<D3D11_SUBRESOURCE_DATA, 16>& out_InitialData);
  plResult CreateStagingTexture(plGALDeviceDX11* pDevice);

protected:
  ID3D11Resource* m_pDXTexture = nullptr;
  ID3D11Resource* m_pDXStagingTexture = nullptr;
};



#include <RendererDX11/Resources/Implementation/TextureDX11_inl.h>
