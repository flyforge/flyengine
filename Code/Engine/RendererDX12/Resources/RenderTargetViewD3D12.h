#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D12Resource;
struct ID3D12DesciptorHeap;
struct D3D12_CPU_DESCRIPTOR_RESOURCE;
struct D3D12_RENDER_TARGET_VIEW_DESC;

class plGALRenderTargetViewD3D12 : public plGALRenderTargetView
{
public:

  PLASMA_ALWAYS_INLINE ID3D12Resource* GetRenderTargetView() const;
  PLASMA_ALWAYS_INLINE ID3D12Resource* GetDepthStencilView() const;
  PLASMA_ALWAYS_INLINE ID3D12Resource* GetUnorderedAccessView() const;

private:
  ID3D12Resource* m_rtvresource;
  ID3D12Resource* m_dsvresource;
  ID3D12Resource* m_uavresource;

  ID3D12DesciptorHeap* m_dsvdesciptorheap;
  ID3D12DesciptorHeap* m_rtvdesciptorheap;
  ID3D12DesciptorHeap* m_uavdesciptorheap;

protected:
  friend class plGALDeviceDX12;
  friend class plMemoryUtils;

  plGALRenderTargetViewD3D12(plGALTexture* pTexture, const plGALRenderTargetViewCreationDescription& Description);

  virtual ~plGALRenderTargetViewD3D12();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;
};

#include <RendererDX12/Resources/Implementation/RenderTargetViewD3D12_inl.h>