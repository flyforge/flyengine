#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>

#include <d3d11.h>

bool IsArrayView(const plGALTextureCreationDescription& texDesc, const plGALUnorderedAccessViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
}

plGALUnorderedAccessViewDX11::plGALUnorderedAccessViewDX11(
  plGALResourceBase* pResource, const plGALUnorderedAccessViewCreationDescription& Description)
  : plGALUnorderedAccessView(pResource, Description)

{
}

plGALUnorderedAccessViewDX11::~plGALUnorderedAccessViewDX11() = default;

plResult plGALUnorderedAccessViewDX11::InitPlatform(plGALDevice* pDevice)
{
  const plGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  const plGALBuffer* pBuffer = nullptr;
  if (!m_Description.m_hBuffer.IsInvalidated())
    pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);

  if (pTexture == nullptr && pBuffer == nullptr)
  {
    plLog::Error("No valid texture handle or buffer handle given for unordered access view creation!");
    return PLASMA_FAILURE;
  }


  plGALResourceFormat::Enum ViewFormat = m_Description.m_OverrideViewFormat;

  if (pTexture)
  {
    const plGALTextureCreationDescription& TexDesc = pTexture->GetDescription();

    if (ViewFormat == plGALResourceFormat::Invalid)
      ViewFormat = TexDesc.m_Format;
  }

  plGALDeviceDX11* pDXDevice = static_cast<plGALDeviceDX11*>(pDevice);


  DXGI_FORMAT DXViewFormat = DXGI_FORMAT_UNKNOWN;
  if (plGALResourceFormat::IsDepthFormat(ViewFormat))
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(ViewFormat).m_eDepthOnlyType;
  }
  else
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(ViewFormat).m_eResourceViewType;
  }

  if (DXViewFormat == DXGI_FORMAT_UNKNOWN)
  {
    plLog::Error("Couldn't get valid DXGI format for resource view! ({0})", ViewFormat);
    return PLASMA_FAILURE;
  }

  D3D11_UNORDERED_ACCESS_VIEW_DESC DXUAVDesc;
  DXUAVDesc.Format = DXViewFormat;

  ID3D11Resource* pDXResource = nullptr;

  if (pTexture)
  {
    pDXResource = static_cast<const plGALTextureDX11*>(pTexture->GetParentResource())->GetDXTexture();
    const plGALTextureCreationDescription& texDesc = pTexture->GetDescription();

    const bool bIsArrayView = IsArrayView(texDesc, m_Description);

    switch (texDesc.m_Type)
    {
      case plGALTextureType::Texture2D:
      case plGALTextureType::Texture2DProxy:
      case plGALTextureType::Texture2DShared:

        if (!bIsArrayView)
        {
          DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
          DXUAVDesc.Texture2D.MipSlice = m_Description.m_uiMipLevelToUse;
        }
        else
        {
          DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
          DXUAVDesc.Texture2DArray.MipSlice = m_Description.m_uiMipLevelToUse;
          DXUAVDesc.Texture2DArray.ArraySize = m_Description.m_uiArraySize;
          DXUAVDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        }
        break;

      case plGALTextureType::TextureCube:
        DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
        DXUAVDesc.Texture2DArray.MipSlice = m_Description.m_uiMipLevelToUse;
        DXUAVDesc.Texture2DArray.ArraySize = m_Description.m_uiArraySize;
        DXUAVDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        break;

      case plGALTextureType::Texture3D:

        DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
        DXUAVDesc.Texture3D.MipSlice = m_Description.m_uiMipLevelToUse;
        DXUAVDesc.Texture3D.FirstWSlice = m_Description.m_uiFirstArraySlice;
        DXUAVDesc.Texture3D.WSize = m_Description.m_uiArraySize;
        break;

      default:
        PLASMA_ASSERT_NOT_IMPLEMENTED;
        return PLASMA_FAILURE;
    }
  }
  else if (pBuffer)
  {
    pDXResource = static_cast<const plGALBufferDX11*>(pBuffer)->GetDXBuffer();

    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
      DXUAVDesc.Format = DXGI_FORMAT_UNKNOWN;

    DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    DXUAVDesc.Buffer.FirstElement = m_Description.m_uiFirstElement;
    DXUAVDesc.Buffer.NumElements = m_Description.m_uiNumElements;
    DXUAVDesc.Buffer.Flags = 0;
    if (m_Description.m_bRawView)
      DXUAVDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_RAW;
    if (m_Description.m_bAppend)
      DXUAVDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_APPEND;
  }

  if (FAILED(pDXDevice->GetDXDevice()->CreateUnorderedAccessView(pDXResource, &DXUAVDesc, &m_pDXUnorderedAccessView)))
  {
    return PLASMA_FAILURE;
  }
  else
  {
    return PLASMA_SUCCESS;
  }
}

plResult plGALUnorderedAccessViewDX11::DeInitPlatform(plGALDevice* pDevice)
{
  PLASMA_GAL_DX11_RELEASE(m_pDXUnorderedAccessView);
  return PLASMA_SUCCESS;
}



PLASMA_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_UnorderedAccessViewDX11);
