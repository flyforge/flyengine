#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/SharedTextureDX11.h>

#include <d3d11.h>

//////////////////////////////////////////////////////////////////////////
// plGALSharedTextureDX11
//////////////////////////////////////////////////////////////////////////

plGALSharedTextureDX11::plGALSharedTextureDX11(const plGALTextureCreationDescription& Description, plEnum<plGALSharedTextureType> sharedType, plGALPlatformSharedHandle hSharedHandle)
  : plGALTextureDX11(Description)
  , m_SharedType(sharedType)
  , m_hSharedHandle(hSharedHandle)
{
}

plGALSharedTextureDX11::~plGALSharedTextureDX11() = default;

plResult plGALSharedTextureDX11::InitPlatform(plGALDevice* pDevice, plArrayPtr<plGALSystemMemoryDescription> pInitialData)
{
  plGALDeviceDX11* pDXDevice = static_cast<plGALDeviceDX11*>(pDevice);

  PL_ASSERT_DEBUG(m_SharedType != plGALSharedTextureType::None, "Shared texture must either be exported or imported");
  PL_ASSERT_DEBUG(m_Description.m_Type == plGALTextureType::Texture2DShared, "Shared texture must be of type plGALTextureType::Texture2DShared");

  if (m_SharedType == plGALSharedTextureType::Imported)
  {
    IDXGIResource* d3d11ResPtr = NULL;
    HRESULT hr = pDXDevice->GetDXDevice()->OpenSharedResource((HANDLE)m_hSharedHandle.m_hSharedTexture, __uuidof(ID3D11Resource), (void**)(&d3d11ResPtr));
    if (FAILED(hr))
    {
      plLog::Error("Failed to open shared texture: {}", plArgErrorCode(hr));
      return PL_FAILURE;
    }
    PL_SCOPE_EXIT(d3d11ResPtr->Release());

    hr = d3d11ResPtr->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pDXTexture));
    if (FAILED(hr))
    {
      plLog::Error("Failed to query shared texture interface: {}", plArgErrorCode(hr));
      return PL_FAILURE;
    }

    hr = d3d11ResPtr->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&m_pKeyedMutex);
    if (FAILED(hr))
    {
      plLog::Error("Failed to query keyed mutex interface: {}", plArgErrorCode(hr));
      return PL_FAILURE;
    }

    return PL_SUCCESS;
  }

  D3D11_TEXTURE2D_DESC Tex2DDesc;
  PL_SUCCEED_OR_RETURN(Create2DDesc(m_Description, pDXDevice, Tex2DDesc));

  if (m_SharedType == plGALSharedTextureType::Exported)
    Tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

  plHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
  ConvertInitialData(m_Description, pInitialData, InitialData);

  if (FAILED(pDXDevice->GetDXDevice()->CreateTexture2D(&Tex2DDesc, pInitialData.IsEmpty() ? nullptr : &InitialData[0], reinterpret_cast<ID3D11Texture2D**>(&m_pDXTexture))))
  {
    return PL_FAILURE;
  }
  else if (m_SharedType == plGALSharedTextureType::Exported)
  {
    IDXGIResource* pDXGIResource;
    HRESULT hr = m_pDXTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&pDXGIResource);
    if (FAILED(hr))
    {
      plLog::Error("Failed to get shared texture resource interface: {}", plArgErrorCode(hr));
      return PL_FAILURE;
    }
    PL_SCOPE_EXIT(pDXGIResource->Release());
    HANDLE hTexture = 0;
    hr = pDXGIResource->GetSharedHandle(&hTexture);
    if (FAILED(hr))
    {
      plLog::Error("Failed to get shared handle: {}", plArgErrorCode(hr));
      return PL_FAILURE;
    }
    hr = pDXGIResource->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&m_pKeyedMutex);
    if (FAILED(hr))
    {
      plLog::Error("Failed to query keyed mutex interface: {}", plArgErrorCode(hr));
      return PL_FAILURE;
    }
    m_hSharedHandle.m_hSharedTexture = (plUInt64)hTexture;
  }

  if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
    return CreateStagingTexture(pDXDevice);

  return PL_SUCCESS;
}


plResult plGALSharedTextureDX11::DeInitPlatform(plGALDevice* pDevice)
{
  PL_GAL_DX11_RELEASE(m_pKeyedMutex);
  return SUPER::DeInitPlatform(pDevice);
}

plGALPlatformSharedHandle plGALSharedTextureDX11::GetSharedHandle() const
{
  return m_hSharedHandle;
}

void plGALSharedTextureDX11::WaitSemaphoreGPU(plUInt64 uiValue) const
{
  m_pKeyedMutex->AcquireSync(uiValue, INFINITE);
}

void plGALSharedTextureDX11::SignalSemaphoreGPU(plUInt64 uiValue) const
{
  m_pKeyedMutex->ReleaseSync(uiValue);
}

PL_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_SharedTextureDX11);
