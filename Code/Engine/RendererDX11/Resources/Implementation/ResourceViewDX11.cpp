#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

bool IsArrayView(const plGALTextureCreationDescription& texDesc, const plGALResourceViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
}

plGALResourceViewDX11::plGALResourceViewDX11(plGALResourceBase* pResource, const plGALResourceViewCreationDescription& Description)
  : plGALResourceView(pResource, Description)
  , m_pDXResourceView(nullptr)
{
}

plGALResourceViewDX11::~plGALResourceViewDX11() {}

plResult plGALResourceViewDX11::InitPlatform(plGALDevice* pDevice)
{
  const plGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  const plGALBuffer* pBuffer = nullptr;
  if (!m_Description.m_hBuffer.IsInvalidated())
    pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);

  if (pTexture == nullptr && pBuffer == nullptr)
  {
    //plLog::Error("No valid texture handle or buffer handle given for resource view creation!");
    return PLASMA_FAILURE;
  }


  plGALResourceFormat::Enum ViewFormat = m_Description.m_OverrideViewFormat;

  if (pTexture)
  {
    if (ViewFormat == plGALResourceFormat::Invalid)
      ViewFormat = pTexture->GetDescription().m_Format;
  }
  else if (pBuffer)
  {
    if (ViewFormat == plGALResourceFormat::Invalid)
      ViewFormat = plGALResourceFormat::RUInt;

    if (!pBuffer->GetDescription().m_bAllowRawViews && m_Description.m_bRawView)
    {
      plLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
      return PLASMA_FAILURE;
    }
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

  D3D11_SHADER_RESOURCE_VIEW_DESC DXSRVDesc;
  DXSRVDesc.Format = DXViewFormat;

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

        if (!bIsArrayView)
        {
          if (texDesc.m_SampleCount == plGALMSAASampleCount::None)
          {
            DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            DXSRVDesc.Texture2D.MipLevels = m_Description.m_uiMipLevelsToUse;
            DXSRVDesc.Texture2D.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
          }
          else
          {
            DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
          }
        }
        else
        {
          if (texDesc.m_SampleCount == plGALMSAASampleCount::None)
          {
            DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            DXSRVDesc.Texture2DArray.MipLevels = m_Description.m_uiMipLevelsToUse;
            DXSRVDesc.Texture2DArray.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
            DXSRVDesc.Texture2DArray.ArraySize = m_Description.m_uiArraySize;
            DXSRVDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
          }
          else
          {
            DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
            DXSRVDesc.Texture2DMSArray.ArraySize = m_Description.m_uiArraySize;
            DXSRVDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
          }
        }

        break;

      case plGALTextureType::TextureCube:

        if (!bIsArrayView)
        {
          DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
          DXSRVDesc.TextureCube.MipLevels = m_Description.m_uiMipLevelsToUse;
          DXSRVDesc.TextureCube.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
        }
        else
        {
          DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
          DXSRVDesc.TextureCube.MipLevels = m_Description.m_uiMipLevelsToUse;
          DXSRVDesc.TextureCube.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
          DXSRVDesc.TextureCubeArray.NumCubes = m_Description.m_uiArraySize;
          DXSRVDesc.TextureCubeArray.First2DArrayFace = m_Description.m_uiFirstArraySlice;
        }

        break;

      case plGALTextureType::Texture3D:

        DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        DXSRVDesc.Texture3D.MipLevels = m_Description.m_uiMipLevelsToUse;
        DXSRVDesc.Texture3D.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;

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
      DXSRVDesc.Format = DXGI_FORMAT_UNKNOWN;

    DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    DXSRVDesc.BufferEx.FirstElement = DXSRVDesc.Buffer.FirstElement = m_Description.m_uiFirstElement;
    DXSRVDesc.BufferEx.NumElements = DXSRVDesc.Buffer.NumElements = m_Description.m_uiNumElements;
    DXSRVDesc.BufferEx.Flags = m_Description.m_bRawView ? D3D11_BUFFEREX_SRV_FLAG_RAW : 0;
  }

  if (FAILED(pDXDevice->GetDXDevice()->CreateShaderResourceView(pDXResource, &DXSRVDesc, &m_pDXResourceView)))
  {
    return PLASMA_FAILURE;
  }
  else
  {
    return PLASMA_SUCCESS;
  }
}

plResult plGALResourceViewDX11::DeInitPlatform(plGALDevice* pDevice)
{
  PLASMA_GAL_DX11_RELEASE(m_pDXResourceView);
  return PLASMA_SUCCESS;
}



PLASMA_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_ResourceViewDX11);
