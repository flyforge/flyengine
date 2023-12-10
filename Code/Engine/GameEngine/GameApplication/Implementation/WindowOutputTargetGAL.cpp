#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Image.h>

plWindowOutputTargetGAL::plWindowOutputTargetGAL(OnSwapChainChanged onSwapChainChanged)
  : m_OnSwapChainChanged(onSwapChainChanged)
{
}

plWindowOutputTargetGAL::~plWindowOutputTargetGAL()
{
  plGALDevice::GetDefaultDevice()->DestroySwapChain(m_hSwapChain);
  m_hSwapChain.Invalidate();
  // After the swapchain is destroyed it can still be used in the renderer. As right after this usually the window is destroyed we must ensure that nothing still renders to it.
  plGALDevice::GetDefaultDevice()->WaitIdle();
}

void plWindowOutputTargetGAL::CreateSwapchain(const plGALWindowSwapChainCreationDescription& desc)
{
  m_currentDesc = desc;
  // plWindowOutputTargetGAL takes over the present mode and keeps it up to date with cvar_AppVSync.
  m_Size = desc.m_pWindow->GetClientAreaSize();
  m_currentDesc.m_InitialPresentMode = plGameApplication::cvar_AppVSync ? plGALPresentMode::VSync : plGALPresentMode::Immediate;

  const bool bSwapChainExisted = !m_hSwapChain.IsInvalidated();
  if (bSwapChainExisted)
  {
    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
    auto* pSwapchain = pDevice->GetSwapChain<plGALWindowSwapChain>(m_hSwapChain);
    pDevice->UpdateSwapChain(m_hSwapChain, plGameApplication::cvar_AppVSync ? plGALPresentMode::VSync : plGALPresentMode::Immediate).AssertSuccess("");
    if (bSwapChainExisted && m_OnSwapChainChanged.IsValid())
    {
      // The swapchain may have a different size than the window advertised, e.g. if the window has been resized further in the meantime.
      plSizeU32 currentSize = pSwapchain->GetCurrentSize();
      m_OnSwapChainChanged(m_hSwapChain, currentSize);
    }
  }
  else
  {
    m_hSwapChain = plGALWindowSwapChain::Create(m_currentDesc);
  }
}

void plWindowOutputTargetGAL::Present(bool bEnableVSync)
{
  // Only re-create the swapchain if somebody is listening to changes.
  if (m_OnSwapChainChanged.IsValid())
  {
    plEnum<plGALPresentMode> presentMode = plGameApplication::cvar_AppVSync ? plGALPresentMode::VSync : plGALPresentMode::Immediate;

    // The actual present call is done by setting the swapchain to an plView.
    // This call is only used to recreate the swapchain at a safe location.
    if (m_Size != m_currentDesc.m_pWindow->GetClientAreaSize() || presentMode != m_currentDesc.m_InitialPresentMode)
    {
      CreateSwapchain(m_currentDesc);
    }
  }
}

plResult plWindowOutputTargetGAL::CaptureImage(plImage& out_Image)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  auto pGALPass = pDevice->BeginPass("CaptureImage");
  PLASMA_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  auto pGALCommandEncoder = pGALPass->BeginRendering(plGALRenderingSetup());
  PLASMA_SCOPE_EXIT(pGALPass->EndRendering(pGALCommandEncoder));

  const plGALSwapChain* pSwapChain = pDevice->GetSwapChain(m_hSwapChain);
  plGALTextureHandle hBackbuffer = pSwapChain ? pSwapChain->GetRenderTargets().m_hRTs[0] : plGALTextureHandle();

  pGALCommandEncoder->ReadbackTexture(hBackbuffer);

  const plGALTexture* pBackbuffer = plGALDevice::GetDefaultDevice()->GetTexture(hBackbuffer);
  const plUInt32 uiWidth = pBackbuffer->GetDescription().m_uiWidth;
  const plUInt32 uiHeight = pBackbuffer->GetDescription().m_uiHeight;
  const plEnum<plGALResourceFormat> format = pBackbuffer->GetDescription().m_Format;

  plDynamicArray<plUInt8> backbufferData;
  backbufferData.SetCountUninitialized(uiWidth * uiHeight * 4);

  plGALSystemMemoryDescription MemDesc;
  MemDesc.m_uiRowPitch = 4 * uiWidth;
  MemDesc.m_uiSlicePitch = 4 * uiWidth * uiHeight;

  /// \todo Make this more efficient
  MemDesc.m_pData = backbufferData.GetData();
  plArrayPtr<plGALSystemMemoryDescription> SysMemDescsDepth(&MemDesc, 1);
  plGALTextureSubresource sourceSubResource;
  plArrayPtr<plGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
  pGALCommandEncoder->CopyTextureReadbackResult(hBackbuffer, sourceSubResources, SysMemDescsDepth);

  plImageHeader header;
  header.SetWidth(uiWidth);
  header.SetHeight(uiHeight);
  header.SetImageFormat(plTextureUtils::GalFormatToImageFormat(format, true));
  out_Image.ResetAndAlloc(header);
  plUInt8* pData = out_Image.GetPixelPointer<plUInt8>();

  plMemoryUtils::Copy(pData, backbufferData.GetData(), backbufferData.GetCount());

  return PLASMA_SUCCESS;
}



PLASMA_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_WindowOutputTargetGAL);
