#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <RendererTest/Advanced/AdvancedFeatures.h>

namespace
{
  plMat4 CreateSimpleMVP(float fAspectRatio)
  {
    plCamera cam;
    cam.SetCameraMode(plCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
    cam.LookAt(plVec3(0, 0, 0), plVec3(0, 0, -1), plVec3(0, 1, 0));
    plMat4 mProj;
    cam.GetProjectionMatrix(fAspectRatio, mProj);
    plMat4 mView = cam.GetViewMatrix();

    plMat4 mTransform;
    mTransform.SetTranslationMatrix(plVec3(0.0f, 0.0f, -1.2f));
    return mProj * mView * mTransform;
  }
} // namespace



plResult plRendererTestAdvancedFeatures::InitializeSubTest(plInt32 iIdentifier)
{
  m_iFrame = -1;
  m_bCaptureImage = false;
  m_ImgCompFrames.Clear();

  PLASMA_SUCCEED_OR_RETURN(plGraphicsTest::InitializeSubTest(iIdentifier));
  PLASMA_SUCCEED_OR_RETURN(SetupRenderer());
  PLASMA_SUCCEED_OR_RETURN(CreateWindow(320, 240));

  if (iIdentifier == ST_ReadRenderTarget)
  {
    // Texture2D
    plGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, plGALResourceFormat::BGRAUByteNormalizedsRGB, plGALMSAASampleCount::None);
    m_hTexture2D = m_pDevice->CreateTexture(desc);

    plGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture = m_hTexture2D;
    viewDesc.m_uiMipLevelsToUse = 1;
    for (plUInt32 i = 0; i < 4; i++)
    {
      viewDesc.m_uiMostDetailedMipLevel = 0;
      m_hTexture2DMips[i] = m_pDevice->CreateResourceView(viewDesc);
    }

    m_hShader2 = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/UVColor.plShader");
    m_hShader = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/Texture2D.plShader");
  }

  if (iIdentifier == ST_VertexShaderRenderTargetArrayIndex)
  {
    if (!m_pDevice->GetCapabilities().m_bVertexShaderRenderTargetArrayIndex)
    {
      plTestFramework::GetInstance()->Output(plTestOutput::Warning, "VertexShaderRenderTargetArrayIndex capability not supported, skipping test.");
      return PLASMA_SUCCESS;
    }
    // Texture2DArray
    plGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(320 / 2, 240, plGALResourceFormat::BGRAUByteNormalizedsRGB, plGALMSAASampleCount::None);
    desc.m_uiArraySize = 2;
    m_hTexture2DArray = m_pDevice->CreateTexture(desc);

    m_hShader = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/Stereo.plShader");
    m_hShader2 = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/StereoPreview.plShader");
  }

  {
    // Cube mesh
    plGeometry geom;
    geom.AddBox(plVec3(1.0f), true);

    plGALPrimitiveTopology::Enum Topology = plGALPrimitiveTopology::Triangles;
    plMeshBufferResourceDescriptor desc;
    desc.AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);
    desc.AddStream(plGALVertexAttributeSemantic::TexCoord0, plGALResourceFormat::RGFloat);
    desc.AllocateStreamsFromGeometry(geom, Topology);

    m_hCubeUV = plResourceManager::GetOrCreateResource<plMeshBufferResource>("Texture2DBox", std::move(desc), "Texture2DBox");
  }

  switch (iIdentifier)
  {
    case SubTests::ST_ReadRenderTarget:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_VertexShaderRenderTargetArrayIndex:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return PLASMA_SUCCESS;
}

plResult plRendererTestAdvancedFeatures::DeInitializeSubTest(plInt32 iIdentifier)
{
  m_hShader2.Invalidate();

  if (!m_hTexture2D.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2D);
    m_hTexture2D.Invalidate();
  }

  if (!m_hTexture2DArray.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2DArray);
    m_hTexture2DArray.Invalidate();
  }

  m_hCubeUV.Invalidate();
  m_hShader.Invalidate();

  DestroyWindow();
  ShutdownRenderer();
  PLASMA_SUCCEED_OR_RETURN(plGraphicsTest::DeInitializeSubTest(iIdentifier));
  return PLASMA_SUCCESS;
}

plTestAppRun plRendererTestAdvancedFeatures::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;
  m_pDevice->BeginFrame(uiInvocationCount);
  m_pDevice->BeginPipeline("GraphicsTest", m_hSwapChain);

  switch (iIdentifier)
  {
    case SubTests::ST_ReadRenderTarget:
      ReadRenderTarget();
      break;
    case SubTests::ST_VertexShaderRenderTargetArrayIndex:
      if (!m_pDevice->GetCapabilities().m_bVertexShaderRenderTargetArrayIndex)
        return plTestAppRun::Quit;
      VertexShaderRenderTargetArrayIndex();
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  plRenderContext::GetDefaultInstance()->ResetContextState();
  m_pDevice->EndPipeline(m_hSwapChain);
  m_pDevice->EndFrame();
  m_pWindow->ProcessWindowMessages();

  plTaskSystem::FinishFrameTasks();

  if (m_ImgCompFrames.IsEmpty() || m_ImgCompFrames.PeekBack() == m_iFrame)
  {
    return plTestAppRun::Quit;
  }
  return plTestAppRun::Continue;
}

void plRendererTestAdvancedFeatures::RenderToScreen(plUInt32 uiRenderTargetClearMask, plRectFloat viewport, plDelegate<void(plGALRenderCommandEncoder*)> func)
{
  const plGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

  plGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture()));
  renderingSetup.m_ClearColor = plColor::RebeccaPurple;
  renderingSetup.m_uiRenderTargetClearMask = uiRenderTargetClearMask;
  if (!m_hDepthStencilTexture.IsInvalidated())
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
    renderingSetup.m_bClearDepth = true;
    renderingSetup.m_bClearStencil = true;
  }

  plGALRenderCommandEncoder* pCommandEncoder = plRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);

  SetClipSpace();

  func(pCommandEncoder);

  plRenderContext::GetDefaultInstance()->EndRendering();
}

void plRendererTestAdvancedFeatures::RenderCube(plRectFloat viewport, plMat4 mMVP, plUInt32 uiRenderTargetClearMask, plGALResourceViewHandle hSRV)
{
  RenderToScreen(uiRenderTargetClearMask, viewport, [&](plGALRenderCommandEncoder* pEncoder) {
    plRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", hSRV);
    RenderObject(m_hCubeUV, mMVP, plColor(1, 1, 1, 1), plShaderBindFlags::None);
    if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
    {
      PLASMA_TEST_IMAGE(m_iFrame, 100);
    }
  });
}

void plRendererTestAdvancedFeatures::ReadRenderTarget()
{
  m_pPass = m_pDevice->BeginPass("Offscreen");
  {
    plGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2D));
    renderingSetup.m_ClearColor = plColor::RebeccaPurple;
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

    plRectFloat viewport = plRectFloat(0, 0, 8, 8);
    plGALRenderCommandEncoder* pCommandEncoder = plRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
    SetClipSpace();

    plRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    plRenderContext::GetDefaultInstance()->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, 1);
    plRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    plRenderContext::GetDefaultInstance()->EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;

  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const plUInt32 uiColumns = 2;
  const plUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const plMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  m_pPass = m_pDevice->BeginPass("Texture2D");
  {
    plRectFloat viewport = plRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2DMips[0]);
    viewport = plRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DMips[0]);
    viewport = plRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DMips[0]);
    m_bCaptureImage = true;
    viewport = plRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DMips[0]);
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

void plRendererTestAdvancedFeatures::VertexShaderRenderTargetArrayIndex()
{
  m_bCaptureImage = true;
  const plMat4 mMVP = CreateSimpleMVP((m_pWindow->GetClientAreaSize().width / 2.0f) / (float)m_pWindow->GetClientAreaSize().height);
  m_pPass = m_pDevice->BeginPass("Offscreen Stereo");
  {
    plGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2DArray));
    renderingSetup.m_ClearColor = plColor::RebeccaPurple;
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

    plRectFloat viewport = plRectFloat(0, 0, m_pWindow->GetClientAreaSize().width / 2.0f, (float)m_pWindow->GetClientAreaSize().height);
    plGALRenderCommandEncoder* pCommandEncoder = plRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
    SetClipSpace();

    plRenderContext::GetDefaultInstance()->BindShader(m_hShader, plShaderBindFlags::None);
    ObjectCB* ocb = plRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
    ocb->m_MVP = mMVP;
    ocb->m_Color = plColor(1, 1, 1, 1);
    plRenderContext::GetDefaultInstance()->BindConstantBuffer("PerObject", m_hObjectTransformCB);
    plRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hCubeUV);
    plRenderContext::GetDefaultInstance()->DrawMeshBuffer(0xFFFFFFFF, 0, 2).IgnoreResult();

    plRenderContext::GetDefaultInstance()->EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;

  m_pPass = m_pDevice->BeginPass("Texture2DArray");
  {
    plRectFloat viewport = plRectFloat(0, 0, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

    RenderToScreen(0xFFFFFFFF, viewport, [&](plGALRenderCommandEncoder* pEncoder) {
      plRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", m_pDevice->GetDefaultResourceView(m_hTexture2DArray));

      plRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
      plRenderContext::GetDefaultInstance()->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, 1);
      plRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

      if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
      {
        PLASMA_TEST_IMAGE(m_iFrame, 100);
      }
    });
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

static plRendererTestAdvancedFeatures g_AdvancedFeaturesTest;
