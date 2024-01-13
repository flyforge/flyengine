#pragma once

#include <RendererFoundation/Resources/Texture.h>

struct ID3D12Resource;
struct D3D12_RESOURCE_DESC;
struct D3D12_SUBRESOURCE_DATA;
class plGALDeviceDX12;

PLASMA_DEFINE_AS_POD_TYPE(D3D12_SUBRESOURCE_DATA);

class plGALTextureD3D12 : public plGALTexture
{

public:
  PLASMA_ALWAYS_INLINE ID3D12Resource* GetDXTexture() const;
  PLASMA_ALWAYS_INLINE ID3D12Resource* GetDXStagingTexture() const;

protected:
  friend class plGALDeviceDX12;
  friend class plMemoryUtils;
  friend class plGALSharedTextureDX12;

  plGALTextureD3D12(const plGALTextureCreationDescription& Description);
  ~plGALTextureD3D12();

  virtual plResult InitPlatform(plGALDevice* pDevice, plArrayPtr<plGALSystemMemoryDescription> pInitialData) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  plResult InitFromNativeObject(plGALDeviceDX12* pDXDevice);

  static plResult Create2DDesc(const plGALTextureCreationDescription& description, plGALDeviceDX12* pDXDevice, D3D12_RESOURCE_DESC& out_Tex2DDesc);
  static plResult Create3DDesc(const plGALTextureCreationDescription& description, plGALDeviceDX12* pDXDevice, D3D12_RESOURCE_DESC& out_Tex3DDesc);
  static void ConvertInitialData(const plGALTextureCreationDescription& description, plArrayPtr<plGALSystemMemoryDescription> pInitialData, plHybridArray<D3D12_SUBRESOURCE_DATA, 16>& out_InitialData);

  plResult CreateStagingTexture(plGALDeviceDX12* pDevice);

protected:
  ID3D12Resource* m_finaltextureresource = nullptr;
  ID3D12Resource* m_temptextureresource = nullptr;
};

#include <RendererDX12/Resources/Implementation/TextureD3D12_inl.h>