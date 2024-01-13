#include <RendererDX12/RendererDX12PCH.h>

#include <RendererDX12/Device/D3D12Device.h>
#include <RendererDX12/Resources/RenderTargetViewD3D12.h>
#include <RendererDX12/Resources/TextureD3D12.h>

plGALRenderTargetViewD3D12::plGALRenderTargetViewD3D12(plGALTexture* pTexture, const plGALRenderTargetBlendDescription& Description)
  : plGALRenderTargetView(pTexture,Description)
{
}