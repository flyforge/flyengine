#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <RendererTest/Basics/SwapChain.h>

plResult plRendererTestSwapChain::InitializeSubTest(plInt32 iIdentifier)
{
  m_iFrame = -1;

  if (plGraphicsTest::InitializeSubTest(iIdentifier).Failed())
    return PLASMA_FAILURE;

  if (SetupRenderer().Failed())
    return PLASMA_FAILURE;

  m_CurrentWindowSize = plSizeU32(320, 240);

  // Window
  {
    plWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width = m_CurrentWindowSize.width;
    WindowCreationDesc.m_Resolution.height = m_CurrentWindowSize.height;
    WindowCreationDesc.m_WindowMode = (iIdentifier == SubTests::ST_ResizeWindow) ? plWindowMode::WindowResizable : plWindowMode::WindowFixedResolution;
    // plGameStateWindow will write any window size changes into the config.
    m_pWindow = PLASMA_DEFAULT_NEW(plGameStateWindow, WindowCreationDesc);
  }

  // SwapChain
  {
    plGALWindowSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow = m_pWindow;
    swapChainDesc.m_SampleCount = plGALMSAASampleCount::None;
    swapChainDesc.m_bAllowScreenshots = true;
    swapChainDesc.m_InitialPresentMode = (iIdentifier == SubTests::ST_NoVSync) ? plGALPresentMode::Immediate : plGALPresentMode::VSync;
    m_hSwapChain = plGALWindowSwapChain::Create(swapChainDesc);
  }

  // Depth Texture
  if (iIdentifier != SubTests::ST_ColorOnly)
  {
    plGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth = m_CurrentWindowSize.width;
    texDesc.m_uiHeight = m_CurrentWindowSize.height;
    switch (iIdentifier)
    {
      case SubTests::ST_D16:
        texDesc.m_Format = plGALResourceFormat::D16;
        break;
      case SubTests::ST_D24S8:
        texDesc.m_Format = plGALResourceFormat::D24S8;
        break;
      default:
      case SubTests::ST_D32:
        texDesc.m_Format = plGALResourceFormat::DFloat;
        break;
    }

    texDesc.m_bCreateRenderTarget = true;
    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
  }

  return PLASMA_SUCCESS;
}

plResult plRendererTestSwapChain::DeInitializeSubTest(plInt32 iIdentifier)
{
  DestroyWindow();
  ShutdownRenderer();

  if (plGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}


void plRendererTestSwapChain::ResizeTest(plUInt32 uiInvocationCount)
{
  if (uiInvocationCount == 4)
  {
    // Not implemented on all platforms,  so we ignore the result here.
    m_pWindow->Resize(plSizeU32(640, 480)).IgnoreResult();
  }

  if (m_pWindow->GetClientAreaSize() != m_CurrentWindowSize)
  {
    m_CurrentWindowSize = m_pWindow->GetClientAreaSize();
    m_pDevice->DestroyTexture(m_hDepthStencilTexture);
    m_hDepthStencilTexture.Invalidate();

    // Swap Chain
    {
      auto presentMode = m_pDevice->GetSwapChain<plGALWindowSwapChain>(m_hSwapChain)->GetWindowDescription().m_InitialPresentMode;
      PLASMA_TEST_RESULT(m_pDevice->UpdateSwapChain(m_hSwapChain, presentMode));
    }

    // Depth Texture
    {
      plGALTextureCreationDescription texDesc;
      texDesc.m_uiWidth = m_CurrentWindowSize.width;
      texDesc.m_uiHeight = m_CurrentWindowSize.height;
      texDesc.m_Format = plGALResourceFormat::DFloat;
      texDesc.m_bCreateRenderTarget = true;
      m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
    }
  }
}

plTestAppRun plRendererTestSwapChain::BasicRenderLoop(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  m_pDevice->BeginFrame(uiInvocationCount);
  m_pDevice->BeginPipeline("GraphicsTest", m_hSwapChain);
  m_pPass = m_pDevice->BeginPass("SwapChainTest");
  {
    const plGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

    plGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture()));
    renderingSetup.m_ClearColor = plColor::CornflowerBlue;
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
    if (!m_hDepthStencilTexture.IsInvalidated())
    {
      renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
      renderingSetup.m_bClearDepth = true;
      renderingSetup.m_bClearStencil = true;
    }
    plRectFloat viewport = plRectFloat(0.0f, 0.0f, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

    plRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
    m_pWindow->ProcessWindowMessages();

    plRenderContext::GetDefaultInstance()->EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
  m_pDevice->EndPipeline(m_hSwapChain);
  m_pDevice->EndFrame();

  plTaskSystem::FinishFrameTasks();

  return m_iFrame < 120 ? plTestAppRun::Continue : plTestAppRun::Quit;
}

static plRendererTestSwapChain g_SwapChainTest;
