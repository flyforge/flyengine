
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

struct ID3D11ShaderResourceView;

class plGALResourceViewDX11 : public plGALResourceView
{
public:
  PLASMA_ALWAYS_INLINE ID3D11ShaderResourceView* GetDXResourceView() const;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALResourceViewDX11(plGALResourceBase* pResource, const plGALResourceViewCreationDescription& Description);

  ~plGALResourceViewDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  ID3D11ShaderResourceView* m_pDXResourceView = nullptr;
};

#include <RendererDX11/Resources/Implementation/ResourceViewDX11_inl.h>
