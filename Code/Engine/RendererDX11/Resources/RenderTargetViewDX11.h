
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

class plGALRenderTargetViewDX11 : public plGALRenderTargetView
{
public:
  PLASMA_ALWAYS_INLINE ID3D11RenderTargetView* GetRenderTargetView() const;

  PLASMA_ALWAYS_INLINE ID3D11DepthStencilView* GetDepthStencilView() const;

  PLASMA_ALWAYS_INLINE ID3D11UnorderedAccessView* GetUnorderedAccessView() const;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALRenderTargetViewDX11(plGALTexture* pTexture, const plGALRenderTargetViewCreationDescription& Description);

  virtual ~plGALRenderTargetViewDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  ID3D11RenderTargetView* m_pRenderTargetView;

  ID3D11DepthStencilView* m_pDepthStencilView;

  ID3D11UnorderedAccessView* m_pUnorderedAccessView;
};

#include <RendererDX11/Resources/Implementation/RenderTargetViewDX11_inl.h>
