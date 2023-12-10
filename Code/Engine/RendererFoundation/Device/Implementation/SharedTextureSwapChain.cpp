#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SharedTextureSwapChain.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plGALSharedTextureSwapChain, plGALSwapChain, 1, plRTTINoAllocator)
{
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plGALSharedTextureSwapChain::Functor plGALSharedTextureSwapChain::s_Factory;

void plGALSharedTextureSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

plGALSwapChainHandle plGALSharedTextureSwapChain::Create(const plGALSharedTextureSwapChainCreationDescription& desc)
{
  PLASMA_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for plGALWindowSwapChain.");
  return s_Factory(desc);
}

plGALSharedTextureSwapChain::plGALSharedTextureSwapChain(const plGALSharedTextureSwapChainCreationDescription& desc)
  : plGALSwapChain(plGetStaticRTTI<plGALSharedTextureSwapChain>())
  , m_Desc(desc)
{
}

void plGALSharedTextureSwapChain::Arm(plUInt32 uiTextureIndex, plUInt64 uiCurrentSemaphoreValue)
{
  if (m_uiCurrentTexture != plMath::MaxValue<plUInt32>())
  {
    // We did not use the previous texture index.
    m_Desc.m_OnPresent(m_uiCurrentTexture, m_uiCurrentSemaphoreValue);
  }
  m_uiCurrentTexture = uiTextureIndex;
  m_uiCurrentSemaphoreValue = uiCurrentSemaphoreValue;

  m_RenderTargets.m_hRTs[0] = m_SharedTextureHandles[m_uiCurrentTexture];
}

void plGALSharedTextureSwapChain::AcquireNextRenderTarget(plGALDevice* pDevice)
{
  PLASMA_ASSERT_DEV(m_uiCurrentTexture != plMath::MaxValue<plUInt32>(), "Acquire called without calling Arm first.");

  m_RenderTargets.m_hRTs[0] = m_SharedTextureHandles[m_uiCurrentTexture];
  m_SharedTextureInterfaces[m_uiCurrentTexture]->WaitSemaphoreGPU(m_uiCurrentSemaphoreValue);
}

void plGALSharedTextureSwapChain::PresentRenderTarget(plGALDevice* pDevice)
{
  m_RenderTargets.m_hRTs[0].Invalidate();

  PLASMA_ASSERT_DEV(m_uiCurrentTexture != plMath::MaxValue<plUInt32>(), "Present called without calling Arm first.");

  m_SharedTextureInterfaces[m_uiCurrentTexture]->SignalSemaphoreGPU(m_uiCurrentSemaphoreValue + 1);
  m_Desc.m_OnPresent(m_uiCurrentTexture, m_uiCurrentSemaphoreValue + 1);

  pDevice->Flush();

  m_uiCurrentTexture = plMath::MaxValue<plUInt32>();
}

plResult plGALSharedTextureSwapChain::UpdateSwapChain(plGALDevice* pDevice, plEnum<plGALPresentMode> newPresentMode)
{
  return PLASMA_SUCCESS;
}

plResult plGALSharedTextureSwapChain::InitPlatform(plGALDevice* pDevice)
{
  // Create textures
  for (plUInt32 i = 0; i < m_Desc.m_Textures.GetCount(); ++i)
  {
    plGALPlatformSharedHandle handle = m_Desc.m_Textures[i];
    plGALTextureHandle hTexture = pDevice->OpenSharedTexture(m_Desc.m_TextureDesc, handle);
    if (hTexture.IsInvalidated())
    {
      plLog::Error("Failed to open shared texture");
      return PLASMA_FAILURE;
    }
    m_SharedTextureHandles.PushBack(hTexture);
    const plGALSharedTexture* pSharedTexture = pDevice->GetSharedTexture(hTexture);
    if (pSharedTexture == nullptr)
    {
      plLog::Error("Created texture is not a shared texture");
      return PLASMA_FAILURE;
    }
    m_SharedTextureInterfaces.PushBack(pSharedTexture);
    m_CurrentSemaphoreValue.PushBack(0);
  }
  m_RenderTargets.m_hRTs[0] = m_SharedTextureHandles[0];
  m_CurrentSize = {m_Desc.m_TextureDesc.m_uiWidth, m_Desc.m_TextureDesc.m_uiHeight};
  return PLASMA_SUCCESS;
}

plResult plGALSharedTextureSwapChain::DeInitPlatform(plGALDevice* pDevice)
{
  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_SharedTextureSwapChain);
