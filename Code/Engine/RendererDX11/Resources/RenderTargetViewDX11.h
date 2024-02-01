
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

class plGALRenderTargetViewDX11 : public plGALRenderTargetView
{
public:
  PL_ALWAYS_INLINE ID3D11RenderTargetView* GetRenderTargetView() const;

  PL_ALWAYS_INLINE ID3D11DepthStencilView* GetDepthStencilView() const;

  PL_ALWAYS_INLINE ID3D11UnorderedAccessView* GetUnorderedAccessView() const;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALRenderTargetViewDX11(plGALTexture* pTexture, const plGALRenderTargetViewCreationDescription& Description);

  virtual ~plGALRenderTargetViewDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  ID3D11RenderTargetView* m_pRenderTargetView = nullptr;

  ID3D11DepthStencilView* m_pDepthStencilView = nullptr;

  ID3D11UnorderedAccessView* m_pUnorderedAccessView = nullptr;
};

#include <RendererDX11/Resources/Implementation/RenderTargetViewDX11_inl.h>
