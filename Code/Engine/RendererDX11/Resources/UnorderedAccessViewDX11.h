
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

struct ID3D11UnorderedAccessView;

class plGALUnorderedAccessViewDX11 : public plGALUnorderedAccessView
{
public:
  PLASMA_ALWAYS_INLINE ID3D11UnorderedAccessView* GetDXResourceView() const;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALUnorderedAccessViewDX11(plGALResourceBase* pResource, const plGALUnorderedAccessViewCreationDescription& Description);

  ~plGALUnorderedAccessViewDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  ID3D11UnorderedAccessView* m_pDXUnorderedAccessView = nullptr;
};

#include <RendererDX11/Resources/Implementation/UnorderedAccessViewDX11_inl.h>
