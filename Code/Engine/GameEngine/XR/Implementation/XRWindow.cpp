#include <GameEngine/GameEnginePCH.h>

#include "../../../../../Data/Base/Shaders/Pipeline/VRCompanionViewConstants.h"
#include <Core/ResourceManager/ResourceManager.h>
#include <GameEngine/XR/XRInterface.h>
#include <GameEngine/XR/XRWindow.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plActorPluginWindowXR, 1, plRTTINoAllocator);
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


//////////////////////////////////////////////////////////////////////////

plWindowXR::plWindowXR(plXRInterface* pVrInterface, plUniquePtr<plWindowBase> pCompanionWindow)
  : m_pVrInterface(pVrInterface)
  , m_pCompanionWindow(std::move(pCompanionWindow))
{
}

plWindowXR::~plWindowXR()
{
  PLASMA_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call plGALDevice::WaitIdle before destroying a window.");
}

plSizeU32 plWindowXR::GetClientAreaSize() const
{
  return m_pVrInterface->GetHmdInfo().m_vEyeRenderTargetSize;
}

plWindowHandle plWindowXR::GetNativeWindowHandle() const
{
  if (m_pCompanionWindow)
  {
    m_pCompanionWindow->GetNativeWindowHandle();
  }
  return plWindowHandle();
}

bool plWindowXR::IsFullscreenWindow(bool bOnlyProperFullscreenMode) const
{
  return true;
}

void plWindowXR::ProcessWindowMessages()
{
  if (m_pCompanionWindow)
  {
    m_pCompanionWindow->ProcessWindowMessages();
  }
}

const plWindowBase* plWindowXR::GetCompanionWindow() const
{
  return m_pCompanionWindow.Borrow();
}

//////////////////////////////////////////////////////////////////////////

plWindowOutputTargetXR::plWindowOutputTargetXR(plXRInterface* pXrInterface, plUniquePtr<plWindowOutputTargetGAL> pCompanionWindowOutputTarget)
  : m_pXrInterface(pXrInterface)
  , m_pCompanionWindowOutputTarget(std::move(pCompanionWindowOutputTarget))
{
  if (m_pCompanionWindowOutputTarget)
  {
    // Create companion resources.
    m_hCompanionShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/VRCompanionView.plShader");
    PLASMA_ASSERT_DEV(m_hCompanionShader.IsValid(), "Could not load VR companion view shader!");
    m_hCompanionConstantBuffer = plRenderContext::CreateConstantBufferStorage<plVRCompanionViewConstants>();
  }
}

plWindowOutputTargetXR::~plWindowOutputTargetXR()
{
  // Delete companion resources.
  plRenderContext::DeleteConstantBufferStorage(m_hCompanionConstantBuffer);
  m_hCompanionConstantBuffer.Invalidate();
}

void plWindowOutputTargetXR::Present(bool bEnableVSync)
{
  // Swapchain present is handled by the rendering of the view automatically and RenderCompanionView is called by the plXRInterface now.
}

void plWindowOutputTargetXR::RenderCompanionView(bool bThrottleCompanionView)
{
  plTime currentTime = plTime::Now();
  if (bThrottleCompanionView && currentTime < (m_LastPresent + plTime::Milliseconds(16)))
    return;

  m_LastPresent = currentTime;

  PLASMA_PROFILE_SCOPE("RenderCompanionView");
  plGALTextureHandle m_hColorRT = m_pXrInterface->GetCurrentTexture();
  if (m_hColorRT.IsInvalidated() || !m_pCompanionWindowOutputTarget)
    return;

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plRenderContext* m_pRenderContext = plRenderContext::GetDefaultInstance();

  {
    pDevice->BeginPipeline("VR CompanionView", m_pCompanionWindowOutputTarget->m_hSwapChain);

    auto pPass = pDevice->BeginPass("Blit CompanionView");

    const plGALSwapChain* pSwapChain = plGALDevice::GetDefaultDevice()->GetSwapChain(m_pCompanionWindowOutputTarget->m_hSwapChain);
    plGALTextureHandle hCompanionRenderTarget = pSwapChain->GetBackBufferTexture();
    const plGALTexture* tex = pDevice->GetTexture(hCompanionRenderTarget);
    auto hRenderTargetView = plGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(hCompanionRenderTarget);
    plVec2 targetSize = plVec2((float)tex->GetDescription().m_uiWidth, (float)tex->GetDescription().m_uiHeight);

    plGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, hRenderTargetView);

    m_pRenderContext->BeginRendering(pPass, renderingSetup, plRectFloat(targetSize.x, targetSize.y));

    m_pRenderContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, 1);
    m_pRenderContext->BindConstantBuffer("plVRCompanionViewConstants", m_hCompanionConstantBuffer);
    m_pRenderContext->BindShader(m_hCompanionShader);

    auto* constants = plRenderContext::GetConstantBufferData<plVRCompanionViewConstants>(m_hCompanionConstantBuffer);
    constants->TargetSize = targetSize;

    plGALResourceViewHandle hInputView = pDevice->GetDefaultResourceView(m_hColorRT);
    m_pRenderContext->BindTexture2D("VRTexture", hInputView);
    m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    m_pRenderContext->EndRendering();

    pDevice->EndPass(pPass);

    pDevice->EndPipeline(m_pCompanionWindowOutputTarget->m_hSwapChain);
    m_pRenderContext->ResetContextState();
  }
}

plResult plWindowOutputTargetXR::CaptureImage(plImage& out_Image)
{
  if (m_pCompanionWindowOutputTarget)
  {
    return m_pCompanionWindowOutputTarget->CaptureImage(out_Image);
  }
  return PLASMA_FAILURE;
}

const plWindowOutputTargetBase* plWindowOutputTargetXR::GetCompanionWindowOutputTarget() const
{
  return m_pCompanionWindowOutputTarget.Borrow();
}

//////////////////////////////////////////////////////////////////////////

plActorPluginWindowXR::plActorPluginWindowXR(plXRInterface* pVrInterface, plUniquePtr<plWindowBase> companionWindow, plUniquePtr<plWindowOutputTargetGAL> companionWindowOutput)
  : m_pVrInterface(pVrInterface)
{
  m_pWindow = PLASMA_DEFAULT_NEW(plWindowXR, pVrInterface, std::move(companionWindow));
  m_pWindowOutputTarget = PLASMA_DEFAULT_NEW(plWindowOutputTargetXR, pVrInterface, std::move(companionWindowOutput));
}

plActorPluginWindowXR::~plActorPluginWindowXR()
{
  m_pVrInterface->OnActorDestroyed();
}

void plActorPluginWindowXR::Initialize() {}

plWindowBase* plActorPluginWindowXR::GetWindow() const
{
  return m_pWindow.Borrow();
}

plWindowOutputTargetBase* plActorPluginWindowXR::GetOutputTarget() const
{
  return m_pWindowOutputTarget.Borrow();
}

void plActorPluginWindowXR::Update()
{
  if (GetWindow())
  {
    GetWindow()->ProcessWindowMessages();
  }
}

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRWindow);
