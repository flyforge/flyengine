#include <RendererDX12/Device/D3D12Device.h>
#include <RendererDX12/RendererDX12PCH.h>
#include <RendererDX12/Resources/TextureD3D12.h>


plGALTextureD3D12::plGALTextureD3D12(const plGALTextureCreationDescription& Description)
  : plGALTexture(Description)
{
}

plGALTextureD3D12::~plGALTextureD3D12() = default;

plResult plGALTextureD3D12::InitPlatform(plGALDevice* pDevice, plArrayPtr<plGALSystemMemoryDescription> pInitialData)
{
  /// Cast to device.
  plGALDeviceDX12* pdxDevice = static_cast<plGALDeviceDX12*>(pDevice);

  if (m_Description.m_pExisitingNativeObject != nullptr)
  {
    return InitFromNativeObject(pdxDevice);
  }

   switch (m_Description.m_Type)
  {
    case plGALTextureType::Texture2D:
    case plGALTextureType::TextureCube:
    {
      D3D12_RESOURCE_DESC Tex2DDesc = {};
      PLASMA_SUCCEED_OR_RETURN(Create2DDesc(m_Description, pdxDevice, Tex2DDesc));

      plHybridArray<D3D12_SUBRESOURCE_DATA, 16> InitialData;
      ConvertInitialData(m_Description, pInitialData, InitialData);
    }
    break;

    case plGALTextureType::Texture3D:
    {
      D3D12_RESOURCE_DESC Tex3DDesc;
      PLASMA_SUCCEED_OR_RETURN(Create3DDesc(m_Description, pdxDevice, Tex3DDesc));

      plHybridArray<D3D12_SUBRESOURCE_DATA, 16> InitialData;
      ConvertInitialData(m_Description, pInitialData, InitialData);

      /// Create the committed resource.
    }
    break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      return PLASMA_FAILURE;
  }

  if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
    return CreateStagingTexture(pdxDevice);

  return PLASMA_SUCCESS;
}

plResult plGALTextureD3D12::DeInitPlatform(plGALDevice* pDevice)
{
}


void plGALTextureD3D12::SetDebugNamePlatform(const char* szName) const
{
}

plResult plGALTextureD3D12::InitFromNativeObject(plGALDeviceDX12* pDXDevice)
{
}

plResult plGALTextureD3D12::Create2DDesc(const plGALTextureCreationDescription& description, plGALDeviceDX12* pDXDevice, D3D12_RESOURCE_DESC& out_Tex2DDesc)
{
}

plResult plGALTextureD3D12::Create3DDesc(const plGALTextureCreationDescription& description, plGALDeviceDX12* pDXDevice, D3D12_RESOURCE_DESC& out_Tex3DDesc)
{
}

void plGALTextureD3D12::ConvertInitialData(const plGALTextureCreationDescription& description, plArrayPtr<plGALSystemMemoryDescription> pInitialData, plHybridArray<D3D12_SUBRESOURCE_DATA, 16>& out_InitialData)
{
}

plResult plGALTextureD3D12::CreateStagingTexture(plGALDeviceDX12* pDevice)
{
}