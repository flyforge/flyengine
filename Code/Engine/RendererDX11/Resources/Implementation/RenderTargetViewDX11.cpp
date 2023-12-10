#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

bool IsArrayView(const plGALTextureCreationDescription& texDesc, const plGALRenderTargetViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstSlice > 0;
}

plGALRenderTargetViewDX11::plGALRenderTargetViewDX11(plGALTexture* pTexture, const plGALRenderTargetViewCreationDescription& Description)
  : plGALRenderTargetView(pTexture, Description)

{
}

plGALRenderTargetViewDX11::~plGALRenderTargetViewDX11() = default;

plResult plGALRenderTargetViewDX11::InitPlatform(plGALDevice* pDevice)
{
  const plGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  if (pTexture == nullptr)
  {
    plLog::Error("No valid texture handle given for rendertarget view creation!");
    return PLASMA_FAILURE;
  }

  const plGALTextureCreationDescription& texDesc = pTexture->GetDescription();
  plGALResourceFormat::Enum viewFormat = texDesc.m_Format;

  if (m_Description.m_OverrideViewFormat != plGALResourceFormat::Invalid)
    viewFormat = m_Description.m_OverrideViewFormat;


  plGALDeviceDX11* pDXDevice = static_cast<plGALDeviceDX11*>(pDevice);

  DXGI_FORMAT DXViewFormat = DXGI_FORMAT_UNKNOWN;

  const bool bIsDepthFormat = plGALResourceFormat::IsDepthFormat(viewFormat);
  if (bIsDepthFormat)
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eDepthStencilType;
  }
  else
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eRenderTarget;
  }

  if (DXViewFormat == DXGI_FORMAT_UNKNOWN)
  {
    plLog::Error("Couldn't get DXGI format for view!");
    return PLASMA_FAILURE;
  }

  ID3D11Resource* pDXResource = static_cast<const plGALTextureDX11*>(pTexture->GetParentResource())->GetDXTexture();
  const bool bIsArrayView = IsArrayView(texDesc, m_Description);

  if (bIsDepthFormat)
  {
    D3D11_DEPTH_STENCIL_VIEW_DESC DSViewDesc;
    DSViewDesc.Format = DXViewFormat;

    if (texDesc.m_SampleCount == plGALMSAASampleCount::None)
    {
      if (!bIsArrayView)
      {
        DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        DSViewDesc.Texture2D.MipSlice = m_Description.m_uiMipLevel;
      }
      else
      {
        DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        DSViewDesc.Texture2DArray.MipSlice = m_Description.m_uiMipLevel;
        DSViewDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstSlice;
        DSViewDesc.Texture2DArray.ArraySize = m_Description.m_uiSliceCount;
      }
    }
    else
    {
      if (!bIsArrayView)
      {
        DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        // DSViewDesc.Texture2DMS.UnusedField_NothingToDefine;
      }
      else
      {
        DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
        DSViewDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstSlice;
        DSViewDesc.Texture2DMSArray.ArraySize = m_Description.m_uiSliceCount;
      }
    }

    DSViewDesc.Flags = 0;
    if (m_Description.m_bReadOnly)
      DSViewDesc.Flags |= (D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL);

    if (FAILED(pDXDevice->GetDXDevice()->CreateDepthStencilView(pDXResource, &DSViewDesc, &m_pDepthStencilView)))
    {
      plLog::Error("Couldn't create depth stencil view!");
      return PLASMA_FAILURE;
    }
    else
    {
      return PLASMA_SUCCESS;
    }
  }
  else
  {
    D3D11_RENDER_TARGET_VIEW_DESC RTViewDesc;
    RTViewDesc.Format = DXViewFormat;

    if (texDesc.m_SampleCount == plGALMSAASampleCount::None)
    {
      if (!bIsArrayView)
      {
        RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        RTViewDesc.Texture2D.MipSlice = m_Description.m_uiMipLevel;
      }
      else
      {
        RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        RTViewDesc.Texture2DArray.MipSlice = m_Description.m_uiMipLevel;
        RTViewDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstSlice;
        RTViewDesc.Texture2DArray.ArraySize = m_Description.m_uiSliceCount;
      }
    }
    else
    {
      if (!bIsArrayView)
      {
        RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        // RTViewDesc.Texture2DMS.UnusedField_NothingToDefine;
      }
      else
      {
        RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
        RTViewDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstSlice;
        RTViewDesc.Texture2DMSArray.ArraySize = m_Description.m_uiSliceCount;
      }
    }

    if (FAILED(pDXDevice->GetDXDevice()->CreateRenderTargetView(pDXResource, &RTViewDesc, &m_pRenderTargetView)))
    {
      plLog::Error("Couldn't create rendertarget view!");
      return PLASMA_FAILURE;
    }
    else
    {
      return PLASMA_SUCCESS;
    }
  }
}

plResult plGALRenderTargetViewDX11::DeInitPlatform(plGALDevice* pDevice)
{
  PLASMA_GAL_DX11_RELEASE(m_pRenderTargetView);
  PLASMA_GAL_DX11_RELEASE(m_pDepthStencilView);
  PLASMA_GAL_DX11_RELEASE(m_pUnorderedAccessView);

  return PLASMA_SUCCESS;
}



PLASMA_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_RenderTargetViewDX11);
