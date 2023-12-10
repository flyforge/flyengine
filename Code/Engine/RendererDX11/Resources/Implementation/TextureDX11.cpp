#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

plGALTextureDX11::plGALTextureDX11(const plGALTextureCreationDescription& Description)
  : plGALTexture(Description)
{
}

plGALTextureDX11::~plGALTextureDX11() = default;

plResult plGALTextureDX11::InitPlatform(plGALDevice* pDevice, plArrayPtr<plGALSystemMemoryDescription> pInitialData)
{
  plGALDeviceDX11* pDXDevice = static_cast<plGALDeviceDX11*>(pDevice);

  if (m_Description.m_pExisitingNativeObject != nullptr)
  {
    return InitFromNativeObject(pDXDevice);
  }

  switch (m_Description.m_Type)
  {
    case plGALTextureType::Texture2D:
    case plGALTextureType::TextureCube:
    {
      D3D11_TEXTURE2D_DESC Tex2DDesc;
      PLASMA_SUCCEED_OR_RETURN(Create2DDesc(m_Description, pDXDevice, Tex2DDesc));

      plHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
      ConvertInitialData(m_Description, pInitialData, InitialData);

      if (FAILED(pDXDevice->GetDXDevice()->CreateTexture2D(&Tex2DDesc, pInitialData.IsEmpty() ? nullptr : &InitialData[0], reinterpret_cast<ID3D11Texture2D**>(&m_pDXTexture))))
      {
        return PLASMA_FAILURE;
      }
    }
    break;

    case plGALTextureType::Texture3D:
    {
      D3D11_TEXTURE3D_DESC Tex3DDesc;
      PLASMA_SUCCEED_OR_RETURN(Create3DDesc(m_Description, pDXDevice, Tex3DDesc));

      plHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
      ConvertInitialData(m_Description, pInitialData, InitialData);

      if (FAILED(pDXDevice->GetDXDevice()->CreateTexture3D(&Tex3DDesc, pInitialData.IsEmpty() ? nullptr : &InitialData[0], reinterpret_cast<ID3D11Texture3D**>(&m_pDXTexture))))
      {
        return PLASMA_FAILURE;
      }
    }
    break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      return PLASMA_FAILURE;
  }

  if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
    return CreateStagingTexture(pDXDevice);

  return PLASMA_SUCCESS;
}


plResult plGALTextureDX11::InitFromNativeObject(plGALDeviceDX11* pDXDevice)
{
  /// \todo Validation if interface of corresponding texture object exists
  m_pDXTexture = static_cast<ID3D11Resource*>(m_Description.m_pExisitingNativeObject);
  if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
  {
    plResult res = CreateStagingTexture(pDXDevice);
    if (res == PLASMA_FAILURE)
    {
      m_pDXTexture = nullptr;
      return res;
    }
  }
  return PLASMA_SUCCESS;
}


plResult plGALTextureDX11::Create2DDesc(const plGALTextureCreationDescription& description, plGALDeviceDX11* pDXDevice, D3D11_TEXTURE2D_DESC& out_Tex2DDesc)
{
  out_Tex2DDesc.ArraySize = (description.m_Type == plGALTextureType::Texture2D ? description.m_uiArraySize : (description.m_uiArraySize * 6));
  out_Tex2DDesc.BindFlags = 0;

  if (description.m_bAllowShaderResourceView || description.m_bAllowDynamicMipGeneration)
    out_Tex2DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
  if (description.m_bAllowUAV)
    out_Tex2DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

  if (description.m_bCreateRenderTarget || description.m_bAllowDynamicMipGeneration)
    out_Tex2DDesc.BindFlags |= plGALResourceFormat::IsDepthFormat(description.m_Format) ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;

  out_Tex2DDesc.CPUAccessFlags = 0; // We always use staging textures to update the data
  out_Tex2DDesc.Usage = description.m_ResourceAccess.IsImmutable() ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;

  if (description.m_bCreateRenderTarget || description.m_bAllowUAV)
    out_Tex2DDesc.Usage = D3D11_USAGE_DEFAULT;

  out_Tex2DDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(description.m_Format).m_eStorage;

  if (out_Tex2DDesc.Format == DXGI_FORMAT_UNKNOWN)
  {
    plLog::Error("No storage format available for given format: {0}", description.m_Format);
    return PLASMA_FAILURE;
  }

  out_Tex2DDesc.Width = description.m_uiWidth;
  out_Tex2DDesc.Height = description.m_uiHeight;
  out_Tex2DDesc.MipLevels = description.m_uiMipLevelCount;

  out_Tex2DDesc.MiscFlags = 0;

  if (description.m_bAllowDynamicMipGeneration)
    out_Tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

  if (description.m_Type == plGALTextureType::TextureCube)
    out_Tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

  out_Tex2DDesc.SampleDesc.Count = description.m_SampleCount;
  out_Tex2DDesc.SampleDesc.Quality = 0;
  return PLASMA_SUCCESS;
}

plResult plGALTextureDX11::Create3DDesc(const plGALTextureCreationDescription& description, plGALDeviceDX11* pDXDevice, D3D11_TEXTURE3D_DESC& out_Tex3DDesc)
{
  out_Tex3DDesc.BindFlags = 0;

  if (description.m_bAllowShaderResourceView)
    out_Tex3DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
  if (description.m_bAllowUAV)
    out_Tex3DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

  if (description.m_bCreateRenderTarget)
    out_Tex3DDesc.BindFlags |= plGALResourceFormat::IsDepthFormat(description.m_Format) ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;

  out_Tex3DDesc.CPUAccessFlags = 0; // We always use staging textures to update the data
  out_Tex3DDesc.Usage = description.m_ResourceAccess.IsImmutable() ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;

  if (description.m_bCreateRenderTarget || description.m_bAllowUAV)
    out_Tex3DDesc.Usage = D3D11_USAGE_DEFAULT;

  out_Tex3DDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(description.m_Format).m_eStorage;

  if (out_Tex3DDesc.Format == DXGI_FORMAT_UNKNOWN)
  {
    plLog::Error("No storage format available for given format: {0}", description.m_Format);
    return PLASMA_FAILURE;
  }

  out_Tex3DDesc.Width = description.m_uiWidth;
  out_Tex3DDesc.Height = description.m_uiHeight;
  out_Tex3DDesc.Depth = description.m_uiDepth;
  out_Tex3DDesc.MipLevels = description.m_uiMipLevelCount;

  out_Tex3DDesc.MiscFlags = 0;

  if (description.m_bAllowDynamicMipGeneration)
    out_Tex3DDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

  if (description.m_Type == plGALTextureType::TextureCube)
    out_Tex3DDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

  return PLASMA_SUCCESS;
}


void plGALTextureDX11::ConvertInitialData(const plGALTextureCreationDescription& description, plArrayPtr<plGALSystemMemoryDescription> pInitialData, plHybridArray<D3D11_SUBRESOURCE_DATA, 16>& out_InitialData)
{
  if (!pInitialData.IsEmpty())
  {
    plUInt32 uiArraySize = 1;
    switch (description.m_Type)
    {
      case plGALTextureType::Texture2D:
        uiArraySize = description.m_uiArraySize;
        break;
      case plGALTextureType::TextureCube:
        uiArraySize = description.m_uiArraySize * 6;
        break;
      case plGALTextureType::Texture3D:
      default:
        break;
    }
    const plUInt32 uiInitialDataCount = (description.m_uiMipLevelCount * uiArraySize);

    PLASMA_ASSERT_DEV(pInitialData.GetCount() == uiInitialDataCount, "The array of initial data values is not equal to the amount of mip levels!");

    out_InitialData.SetCountUninitialized(uiInitialDataCount);

    for (plUInt32 i = 0; i < uiInitialDataCount; i++)
    {
      out_InitialData[i].pSysMem = pInitialData[i].m_pData;
      out_InitialData[i].SysMemPitch = pInitialData[i].m_uiRowPitch;
      out_InitialData[i].SysMemSlicePitch = pInitialData[i].m_uiSlicePitch;
    }
  }
}

plResult plGALTextureDX11::DeInitPlatform(plGALDevice* pDevice)
{
  PLASMA_GAL_DX11_RELEASE(m_pDXTexture);
  PLASMA_GAL_DX11_RELEASE(m_pDXStagingTexture);
  return PLASMA_SUCCESS;
}

void plGALTextureDX11::SetDebugNamePlatform(const char* szName) const
{
  plUInt32 uiLength = plStringUtils::GetStringElementCount(szName);

  if (m_pDXTexture != nullptr)
  {
    m_pDXTexture->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

plResult plGALTextureDX11::CreateStagingTexture(plGALDeviceDX11* pDevice)
{

  switch (m_Description.m_Type)
  {
    case plGALTextureType::Texture2D:
    case plGALTextureType::TextureCube:
    {
      D3D11_TEXTURE2D_DESC Desc;
      static_cast<ID3D11Texture2D*>(m_pDXTexture)->GetDesc(&Desc);
      Desc.BindFlags = 0;
      Desc.CPUAccessFlags = 0;
      // Need to remove this flag on the staging resource or texture readback no longer works.
      Desc.MiscFlags &= ~D3D11_RESOURCE_MISC_GENERATE_MIPS;
      Desc.Usage = D3D11_USAGE_STAGING;
      Desc.SampleDesc.Count = 1; // We need to disable MSAA for the readback texture, the conversion needs to happen during readback!

      if (m_Description.m_ResourceAccess.m_bReadBack)
        Desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
      if (!m_Description.m_ResourceAccess.IsImmutable())
        Desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;

      if (FAILED(pDevice->GetDXDevice()->CreateTexture2D(&Desc, nullptr, reinterpret_cast<ID3D11Texture2D**>(&m_pDXStagingTexture))))
      {
        plLog::Error("Couldn't create staging resource for data upload and/or read back!");
        return PLASMA_FAILURE;
      }
    }
    break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }



  return PLASMA_SUCCESS;
}


PLASMA_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_TextureDX11);
