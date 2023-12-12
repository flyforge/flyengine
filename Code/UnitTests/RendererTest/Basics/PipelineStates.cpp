#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <RendererTest/Basics/PipelineStates.h>

#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestConstants.h>
#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestInstancing.h>

PLASMA_DEFINE_AS_POD_TYPE(plTestShaderData);

namespace
{
  plTransform CreateTransform(const plUInt32 uiColumns, const plUInt32 uiRows, plUInt32 x, plUInt32 y)
  {
    plTransform t = plTransform::IdentityTransform();
    t.m_vScale = plVec3(1.0f / float(uiColumns), 1.0f / float(uiRows), 1);
    t.m_vPosition = plVec3(plMath::Lerp(-1.f, 1.f, (float(x) + 0.5f) / float(uiColumns)), plMath::Lerp(1.f, -1.f, (float(y) + 0.5f) / float(uiRows)), 0);
    if (plClipSpaceYMode::RenderToTextureDefault == plClipSpaceYMode::Flipped)
    {
      plTransform flipY = plTransform::IdentityTransform();
      flipY.m_vScale.y *= -1.0f;
      t = flipY * t;
    }
    return t;
  }

  void FillStructuredBuffer(plHybridArray<plTestShaderData, 16>& ref_instanceData, plUInt32 uiColorOffset = 0, plUInt32 uiSlotOffset = 0)
  {
    ref_instanceData.SetCount(16);
    const plUInt32 uiColumns = 4;
    const plUInt32 uiRows = 2;

    for (plUInt32 x = 0; x < uiColumns; ++x)
    {
      for (plUInt32 y = 0; y < uiRows; ++y)
      {
        plTestShaderData& instance = ref_instanceData[uiSlotOffset + x * uiRows + y];
        const float fColorIndex = float(uiColorOffset + x * uiRows + y) / 32.0f;
        instance.InstanceColor = plColorScheme::LightUI(fColorIndex).GetAsVec4();
        plTransform t = CreateTransform(uiColumns, uiRows, x, y);
        instance.InstanceTransform = t;
      }
    }
  }

  struct ImgColor
  {
    PLASMA_DECLARE_POD_TYPE();
    plUInt8 b;
    plUInt8 g;
    plUInt8 r;
    plUInt8 a;
  };

  void CreateImage(plImage& ref_image, plUInt32 uiWidth, plUInt32 uiHeight, plUInt32 uiMipLevelCount, bool bMipLevelIsBlue, plUInt8 uiFixedBlue = 0)
  {
    plImageHeader header;
    header.SetImageFormat(plImageFormat::B8G8R8A8_UNORM_SRGB);
    header.SetWidth(uiWidth);
    header.SetHeight(uiHeight);
    header.SetNumMipLevels(uiMipLevelCount);

    ref_image.ResetAndAlloc(header);
    for (plUInt32 m = 0; m < uiMipLevelCount; m++)
    {
      const plUInt32 uiHeight = ref_image.GetHeight(m);
      const plUInt32 uiWidth = ref_image.GetWidth(m);

      const plUInt8 uiBlue = bMipLevelIsBlue ? static_cast<plUInt8>(255.0f * float(m) / (uiMipLevelCount - 1)) : uiFixedBlue;
      for (plUInt32 y = 0; y < uiHeight; y++)
      {
        const plUInt8 uiGreen = static_cast<plUInt8>(255.0f * float(y) / (uiHeight - 1));
        for (plUInt32 x = 0; x < uiWidth; x++)
        {
          ImgColor* pColor = ref_image.GetPixelPointer<ImgColor>(m, 0u, 0u, x, y);
          pColor->a = 255;
          pColor->b = uiBlue;
          pColor->g = uiGreen;
          pColor->r = static_cast<plUInt8>(255.0f * float(x) / (uiWidth - 1));
        }
      }
    }
  }

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



plResult plRendererTestPipelineStates::InitializeSubTest(plInt32 iIdentifier)
{
  m_iFrame = -1;
  m_bCaptureImage = false;
  m_ImgCompFrames.Clear();

  {
    m_bTimestampsValid = false;
    m_CPUTime[0] = {};
    m_CPUTime[1] = {};
    m_GPUTime[0] = {};
    m_GPUTime[1] = {};
    m_timestamps[0] = {};
    m_timestamps[1] = {};
  }

  PLASMA_SUCCEED_OR_RETURN(plGraphicsTest::InitializeSubTest(iIdentifier));
  PLASMA_SUCCEED_OR_RETURN(SetupRenderer());
  PLASMA_SUCCEED_OR_RETURN(CreateWindow(320, 240));
  m_hMostBasicTriangleShader = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/MostBasicTriangle.plShader");
  m_hNDCPositionOnlyShader = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/NDCPositionOnly.plShader");
  m_hConstantBufferShader = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/ConstantBuffer.plShader");
  m_hInstancingShader = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/Instancing.plShader");

  {
    plMeshBufferResourceDescriptor desc;
    desc.AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);
    desc.AllocateStreams(3);

    if (plClipSpaceYMode::RenderToTextureDefault == plClipSpaceYMode::Flipped)
    {
      desc.SetVertexData<plVec3>(0, 0, plVec3(1.f, 1.f, 0.0f));
      desc.SetVertexData<plVec3>(0, 1, plVec3(-1.f, 1.f, 0.0f));
      desc.SetVertexData<plVec3>(0, 2, plVec3(0.f, -1.f, 0.0f));
    }
    else
    {
      desc.SetVertexData<plVec3>(0, 0, plVec3(1.f, -1.f, 0.0f));
      desc.SetVertexData<plVec3>(0, 1, plVec3(-1.f, -1.f, 0.0f));
      desc.SetVertexData<plVec3>(0, 2, plVec3(0.f, 1.f, 0.0f));
    }

    m_hTriangleMesh = plResourceManager::CreateResource<plMeshBufferResource>("UnitTest-TriangleMesh", std::move(desc), "TriangleMesh");
  }
  {
    plGeometry geom;
    geom.AddSphere(0.5f, 16, 16);

    plMeshBufferResourceDescriptor desc;
    desc.AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);

    m_hSphereMesh = plResourceManager::CreateResource<plMeshBufferResource>("UnitTest-SphereMesh", std::move(desc), "SphereMesh");
  }
  m_hTestColorsConstantBuffer = plRenderContext::CreateConstantBufferStorage<plTestColors>();
  m_hTestPositionsConstantBuffer = plRenderContext::CreateConstantBufferStorage<plTestPositions>();

  {
    plGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(plTestShaderData);
    desc.m_uiTotalSize = 16 * desc.m_uiStructSize;
    desc.m_BufferType = plGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    // We only fill the first 8 elements with data. The rest is dynamically updated during testing.
    plHybridArray<plTestShaderData, 16> instanceData;
    FillStructuredBuffer(instanceData);
    m_hInstancingData = m_pDevice->CreateBuffer(desc, instanceData.GetByteArrayPtr());

    plGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hBuffer = m_hInstancingData;
    viewDesc.m_uiFirstElement = 8;
    viewDesc.m_uiNumElements = 4;
    m_hInstancingDataView_8_4 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiFirstElement = 12;
    m_hInstancingDataView_12_4 = m_pDevice->CreateResourceView(viewDesc);
  }

  {
    // Texture2D
    plGALTextureCreationDescription desc;
    desc.m_uiWidth = 8;
    desc.m_uiHeight = 8;
    desc.m_uiMipLevelCount = 4;
    desc.m_Format = plGALResourceFormat::BGRAUByteNormalizedsRGB;

    plImage coloredMips;
    CreateImage(coloredMips, desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, true);

    if (iIdentifier == SubTests::ST_GenerateMipMaps)
    {
      // Clear all mips except the fist one and let them be regenerated.
      desc.m_ResourceAccess.m_bImmutable = false;
      desc.m_bAllowDynamicMipGeneration = true;
      for (plUInt32 m = 1; m < desc.m_uiMipLevelCount; m++)
      {
        const plUInt32 uiHeight = coloredMips.GetHeight(m);
        const plUInt32 uiWidth = coloredMips.GetWidth(m);
        for (plUInt32 y = 0; y < uiHeight; y++)
        {
          for (plUInt32 x = 0; x < uiWidth; x++)
          {
            ImgColor* pColor = coloredMips.GetPixelPointer<ImgColor>(m, 0u, 0u, x, y);
            pColor->a = 255;
            pColor->b = 0;
            pColor->g = 0;
            pColor->r = 0;
          }
        }
      }
    }

    plHybridArray<plGALSystemMemoryDescription, 4> initialData;
    initialData.SetCount(desc.m_uiMipLevelCount);
    for (plUInt32 m = 0; m < desc.m_uiMipLevelCount; m++)
    {
      plGALSystemMemoryDescription& memoryDesc = initialData[m];
      memoryDesc.m_pData = coloredMips.GetPixelPointer<plUInt8>(m);
      memoryDesc.m_uiRowPitch = static_cast<plUInt32>(coloredMips.GetRowPitch(m));
      memoryDesc.m_uiSlicePitch = static_cast<plUInt32>(coloredMips.GetDepthPitch(m));
    }
    m_hTexture2D = m_pDevice->CreateTexture(desc, initialData);

    plGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture = m_hTexture2D;
    viewDesc.m_uiMostDetailedMipLevel = 0;
    viewDesc.m_uiMipLevelsToUse = 1;
    m_hTexture2D_Mip0 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 1;
    m_hTexture2D_Mip1 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 2;
    m_hTexture2D_Mip2 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 3;
    m_hTexture2D_Mip3 = m_pDevice->CreateResourceView(viewDesc);
  }

  {
    // Texture2DArray
    plGALTextureCreationDescription desc;
    desc.m_uiWidth = 8;
    desc.m_uiHeight = 8;
    desc.m_uiMipLevelCount = 4;
    desc.m_uiArraySize = 2;
    desc.m_Type = plGALTextureType::Texture2D;
    desc.m_Format = plGALResourceFormat::BGRAUByteNormalizedsRGB;

    plImage coloredMips[2];
    CreateImage(coloredMips[0], desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, false, 0);
    CreateImage(coloredMips[1], desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, false, 255);

    plHybridArray<plGALSystemMemoryDescription, 8> initialData;
    initialData.SetCount(desc.m_uiArraySize * desc.m_uiMipLevelCount);
    for (plUInt32 l = 0; l < desc.m_uiArraySize; l++)
    {
      for (plUInt32 m = 0; m < desc.m_uiMipLevelCount; m++)
      {
        plGALSystemMemoryDescription& memoryDesc = initialData[m + l * desc.m_uiMipLevelCount];
        memoryDesc.m_pData = coloredMips[l].GetPixelPointer<plUInt8>(m);
        memoryDesc.m_uiRowPitch = static_cast<plUInt32>(coloredMips[l].GetRowPitch(m));
        memoryDesc.m_uiSlicePitch = static_cast<plUInt32>(coloredMips[l].GetDepthPitch(m));
      }
    }
    m_hTexture2DArray = m_pDevice->CreateTexture(desc, initialData);

    plGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture = m_hTexture2DArray;
    viewDesc.m_uiMipLevelsToUse = 1;
    viewDesc.m_uiFirstArraySlice = 0;
    viewDesc.m_uiMostDetailedMipLevel = 0;
    m_hTexture2DArray_Layer0_Mip0 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 1;
    m_hTexture2DArray_Layer0_Mip1 = m_pDevice->CreateResourceView(viewDesc);

    viewDesc.m_uiFirstArraySlice = 1;
    viewDesc.m_uiMostDetailedMipLevel = 0;
    m_hTexture2DArray_Layer1_Mip0 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 1;
    m_hTexture2DArray_Layer1_Mip1 = m_pDevice->CreateResourceView(viewDesc);
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
    case SubTests::ST_MostBasicShader:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_ViewportScissor:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_VertexBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_IndexBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_ConstantBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_StructuredBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_InitialData);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_Discard);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_NoOverwrite);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_CopyToTempStorage);
      break;
    case SubTests::ST_GenerateMipMaps:
    case SubTests::ST_Texture2D:
    {
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_hShader = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/Texture2D.plShader");
    }
    break;
    case SubTests::ST_Texture2DArray:
    {
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_hShader = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/Texture2DArray.plShader");
    }
    break;
    case SubTests::ST_Timestamps:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::Timestamps_MaxWaitTime);
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return PLASMA_SUCCESS;
}

plResult plRendererTestPipelineStates::DeInitializeSubTest(plInt32 iIdentifier)
{
  m_hTriangleMesh.Invalidate();
  m_hSphereMesh.Invalidate();

  m_hMostBasicTriangleShader.Invalidate();
  m_hNDCPositionOnlyShader.Invalidate();
  m_hConstantBufferShader.Invalidate();
  m_hInstancingShader.Invalidate();

  m_hTestColorsConstantBuffer.Invalidate();
  m_hTestPositionsConstantBuffer.Invalidate();

  if (!m_hInstancingData.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hInstancingData);
    m_hInstancingData.Invalidate();
  }
  m_hInstancingDataView_8_4.Invalidate();
  m_hInstancingDataView_12_4.Invalidate();

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
  m_hTexture2D_Mip0.Invalidate();
  m_hTexture2D_Mip1.Invalidate();
  m_hTexture2D_Mip2.Invalidate();
  m_hTexture2D_Mip3.Invalidate();
  m_hCubeUV.Invalidate();
  m_hShader.Invalidate();

  DestroyWindow();
  ShutdownRenderer();
  PLASMA_SUCCEED_OR_RETURN(plGraphicsTest::DeInitializeSubTest(iIdentifier));
  return PLASMA_SUCCESS;
}

plTestAppRun plRendererTestPipelineStates::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;
  m_pDevice->BeginFrame(uiInvocationCount);
  m_pDevice->BeginPipeline("GraphicsTest", m_hSwapChain);

  switch (iIdentifier)
  {
    case SubTests::ST_MostBasicShader:
      MostBasicTriangleTest();
      break;
    case SubTests::ST_ViewportScissor:
      ViewportScissorTest();
      break;
    case SubTests::ST_VertexBuffer:
      VertexBufferTest();
      break;
    case SubTests::ST_IndexBuffer:
      IndexBufferTest();
      break;
    case SubTests::ST_ConstantBuffer:
      ConstantBufferTest();
      break;
    case SubTests::ST_StructuredBuffer:
      StructuredBufferTest();
      break;
    case SubTests::ST_Texture2D:
      Texture2D();
      break;
    case SubTests::ST_Texture2DArray:
      Texture2DArray();
      break;
    case SubTests::ST_GenerateMipMaps:
      GenerateMipMaps();
      break;
    case SubTests::ST_Timestamps:
      Timestamps();
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  plRenderContext::GetDefaultInstance()->ResetContextState();
  m_pDevice->EndPipeline(m_hSwapChain);
  m_pDevice->EndFrame();

  plTaskSystem::FinishFrameTasks();

  if (m_ImgCompFrames.IsEmpty() || m_ImgCompFrames.PeekBack() == m_iFrame)
  {
    return plTestAppRun::Quit;
  }
  return plTestAppRun::Continue;
}

void plRendererTestPipelineStates::RenderBlock(plMeshBufferResourceHandle mesh, plColor clearColor, plUInt32 uiRenderTargetClearMask, plRectFloat* pViewport, plRectU32* pScissor)
{
  m_pPass = m_pDevice->BeginPass("MostBasicTriangle");
  {
    plGALRenderCommandEncoder* pCommandEncoder = BeginRendering(clearColor, uiRenderTargetClearMask, pViewport, pScissor);
    {

      if (mesh.IsValid())
      {
        plRenderContext::GetDefaultInstance()->BindShader(m_hNDCPositionOnlyShader);
        plRenderContext::GetDefaultInstance()->BindMeshBuffer(mesh);
        plRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();
      }
      else
      {
        plRenderContext::GetDefaultInstance()->BindShader(m_hMostBasicTriangleShader);
        plRenderContext::GetDefaultInstance()->BindNullMeshBuffer(plGALPrimitiveTopology::Triangles, 1);
        plRenderContext::GetDefaultInstance()->DrawMeshBuffer(1).AssertSuccess();
      }
    }

    if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
    {
      PLASMA_TEST_IMAGE(m_iFrame, 100);
    }
    EndRendering();
  }

  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

plGALRenderCommandEncoder* plRendererTestPipelineStates::BeginRendering(plColor clearColor, plUInt32 uiRenderTargetClearMask, plRectFloat* pViewport, plRectU32* pScissor)
{
  const plGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

  plGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture()));
  renderingSetup.m_ClearColor = clearColor;
  renderingSetup.m_uiRenderTargetClearMask = uiRenderTargetClearMask;
  if (!m_hDepthStencilTexture.IsInvalidated())
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
    renderingSetup.m_bClearDepth = true;
    renderingSetup.m_bClearStencil = true;
  }
  plRectFloat viewport = plRectFloat(0.0f, 0.0f, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);
  if (pViewport)
  {
    viewport = *pViewport;
  }

  plGALRenderCommandEncoder* pCommandEncoder = plRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
  plRectU32 scissor = plRectU32(0, 0, m_pWindow->GetClientAreaSize().width, m_pWindow->GetClientAreaSize().height);
  if (pScissor)
  {
    scissor = *pScissor;
  }
  pCommandEncoder->SetScissorRect(scissor);

  SetClipSpace();
  return pCommandEncoder;
}

void plRendererTestPipelineStates::EndRendering()
{
  plRenderContext::GetDefaultInstance()->EndRendering();
  m_pWindow->ProcessWindowMessages();
}

void plRendererTestPipelineStates::MostBasicTriangleTest()
{
  m_bCaptureImage = true;
  RenderBlock({}, plColor::RebeccaPurple);
}

void plRendererTestPipelineStates::ViewportScissorTest()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const plUInt32 uiColumns = 2;
  const plUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  plRectFloat viewport = plRectFloat(0, 0, fElementWidth, fElementHeight);
  RenderBlock({}, plColor::CornflowerBlue, 0xFFFFFFFF, &viewport);

  viewport = plRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
  RenderBlock({}, plColor::Green, 0, &viewport);

  viewport = plRectFloat(0, 0, fElementWidth, fHeight);
  plRectU32 scissor = plRectU32(0, (plUInt32)fElementHeight, (plUInt32)fElementWidth, (plUInt32)fElementHeight);
  RenderBlock({}, plColor::Green, 0, &viewport, &scissor);

  m_bCaptureImage = true;
  viewport = plRectFloat(0, 0, fWidth, fHeight);
  scissor = plRectU32((plUInt32)fElementWidth, 0, (plUInt32)fElementWidth, (plUInt32)fElementHeight);
  RenderBlock({}, plColor::Green, 0, &viewport, &scissor);
}

void plRendererTestPipelineStates::VertexBufferTest()
{
  m_bCaptureImage = true;
  RenderBlock(m_hTriangleMesh, plColor::RebeccaPurple);
}

void plRendererTestPipelineStates::IndexBufferTest()
{
  m_bCaptureImage = true;
  RenderBlock(m_hSphereMesh, plColor::Orange);
}

void plRendererTestPipelineStates::ConstantBufferTest()
{
  const plUInt32 uiColumns = 4;
  const plUInt32 uiRows = 2;

  m_pPass = m_pDevice->BeginPass("ConstantBufferTest");
  {
    plGALRenderCommandEncoder* pCommandEncoder = BeginRendering(plColor::CornflowerBlue, 0xFFFFFFFF);
    plRenderContext* pContext = plRenderContext::GetDefaultInstance();
    {
      pContext->BindConstantBuffer("plTestColors", m_hTestColorsConstantBuffer);
      pContext->BindConstantBuffer("plTestPositions", m_hTestPositionsConstantBuffer);
      pContext->BindShader(m_hConstantBufferShader);
      pContext->BindNullMeshBuffer(plGALPrimitiveTopology::Triangles, 1);

      for (plUInt32 x = 0; x < uiColumns; ++x)
      {
        for (plUInt32 y = 0; y < uiRows; ++y)
        {
          {
            auto constants = plRenderContext::GetConstantBufferData<plTestColors>(m_hTestColorsConstantBuffer);
            constants->VertexColor = plColorScheme::LightUI(float(x * uiRows + y) / (uiColumns * uiRows)).GetAsVec4();
          }
          {
            plTransform t = CreateTransform(uiColumns, uiRows, x, y);
            auto constants = plRenderContext::GetConstantBufferData<plTestPositions>(m_hTestPositionsConstantBuffer);
            constants->Vertex0 = (t * plVec3(1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
            constants->Vertex1 = (t * plVec3(-1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
            constants->Vertex2 = (t * plVec3(-0.f, 1.f, 0.0f)).GetAsVec4(1.0f);
          }
          pContext->DrawMeshBuffer(1).AssertSuccess();
        }
      }
    }
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      PLASMA_TEST_IMAGE(m_iFrame, 100);
    }
    EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}


void plRendererTestPipelineStates::StructuredBufferTest()
{
  m_pPass = m_pDevice->BeginPass("InstancingTest");
  {

    plGALRenderCommandEncoder* pCommandEncoder = BeginRendering(plColor::CornflowerBlue, 0xFFFFFFFF);
    if (m_iFrame == ImageCaptureFrames::StructuredBuffer_Discard)
    {
      // Discard previous buffer.
      plHybridArray<plTestShaderData, 16> instanceData;
      FillStructuredBuffer(instanceData, 16);
      pCommandEncoder->UpdateBuffer(m_hInstancingData, 0, instanceData.GetArrayPtr().ToByteArray(), plGALUpdateMode::Discard);
    }
    else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_NoOverwrite)
    {
      // Nothing has touched the second half of the new buffer yet. Fill it with the original data of the first 8 elements.
      plHybridArray<plTestShaderData, 16> instanceData;
      FillStructuredBuffer(instanceData);
      instanceData.SetCount(8);
      pCommandEncoder->UpdateBuffer(m_hInstancingData, 8 * sizeof(plTestShaderData), instanceData.GetArrayPtr().ToByteArray(), plGALUpdateMode::NoOverwrite);
    }
    else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_CopyToTempStorage)
    {
      // Now we replace the first 4 elements of the second half of the buffer.
      plHybridArray<plTestShaderData, 16> instanceData;
      FillStructuredBuffer(instanceData, 16);
      instanceData.SetCount(4);
      pCommandEncoder->UpdateBuffer(m_hInstancingData, 8 * sizeof(plTestShaderData), instanceData.GetArrayPtr().ToByteArray(), plGALUpdateMode::CopyToTempStorage);
    }

    plRenderContext* pContext = plRenderContext::GetDefaultInstance();
    {
      pContext->BindShader(m_hInstancingShader);
      pContext->BindMeshBuffer(m_hTriangleMesh);

      if (m_iFrame < ImageCaptureFrames::StructuredBuffer_NoOverwrite)
      {
        pContext->BindBuffer("instancingData", m_pDevice->GetDefaultResourceView(m_hInstancingData));
        pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
      }
      else if (m_iFrame >= ImageCaptureFrames::StructuredBuffer_NoOverwrite)
      {
        pContext->BindBuffer("instancingData", m_hInstancingDataView_8_4);
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
        pContext->BindBuffer("instancingData", m_hInstancingDataView_12_4);
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
      }
    }
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      PLASMA_TEST_IMAGE(m_iFrame, 100);
    }
    EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

void plRendererTestPipelineStates::RenderCube(plRectFloat viewport, plMat4 mMVP, plUInt32 uiRenderTargetClearMask, plGALResourceViewHandle hSRV)
{
  plGALRenderCommandEncoder* pCommandEncoder = BeginRendering(plColor::RebeccaPurple, uiRenderTargetClearMask, &viewport);

  plRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", hSRV);
  RenderObject(m_hCubeUV, mMVP, plColor(1, 1, 1, 1), plShaderBindFlags::None);
  if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
  {
    PLASMA_TEST_IMAGE(m_iFrame, 100);
  }
  EndRendering();
};

void plRendererTestPipelineStates::Texture2D()
{
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
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D_Mip0);
    viewport = plRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip1);
    viewport = plRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip2);
    m_bCaptureImage = true;
    viewport = plRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip3);
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

void plRendererTestPipelineStates::Texture2DArray()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const plUInt32 uiColumns = 2;
  const plUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const plMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);

  m_pPass = m_pDevice->BeginPass("Texture2DArray");
  {
    plRectFloat viewport = plRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2DArray_Layer0_Mip0);
    viewport = plRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray_Layer0_Mip1);
    viewport = plRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray_Layer1_Mip0);
    m_bCaptureImage = true;
    viewport = plRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray_Layer1_Mip1);
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

void plRendererTestPipelineStates::GenerateMipMaps()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const plUInt32 uiColumns = 2;
  const plUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const plMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  m_pPass = m_pDevice->BeginPass("GenerateMipMaps");
  {
    plRectFloat viewport = plRectFloat(0, 0, fElementWidth, fElementHeight);
    plGALRenderCommandEncoder* pCommandEncoder = BeginRendering(plColor::RebeccaPurple, 0, &viewport);
    pCommandEncoder->GenerateMipMaps(m_pDevice->GetDefaultResourceView(m_hTexture2D));
    EndRendering();

    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D_Mip0);
    viewport = plRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip1);
    viewport = plRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip2);
    m_bCaptureImage = true;
    viewport = plRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip3);
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

void plRendererTestPipelineStates::Timestamps()
{
  m_pPass = m_pDevice->BeginPass("Timestamps");
  {
    plGALRenderCommandEncoder* pCommandEncoder = BeginRendering(plColor::RebeccaPurple, 0xFFFFFFFF);

    if (m_iFrame == 2)
    {
      m_CPUTime[0] = plTime::Now();
      m_timestamps[0] = pCommandEncoder->InsertTimestamp();
    }
    plRenderContext::GetDefaultInstance()->BindShader(m_hNDCPositionOnlyShader);
    plRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hSphereMesh);
    plRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    if (m_iFrame == 2)
      m_timestamps[1] = pCommandEncoder->InsertTimestamp();
    EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;

  if (m_iFrame > 2 && !m_bTimestampsValid)
  {
    if ((m_bTimestampsValid = m_pDevice->GetTimestampResult(m_timestamps[0], m_GPUTime[0]).Succeeded() && m_pDevice->GetTimestampResult(m_timestamps[1], m_GPUTime[1]).Succeeded()))
    {
      m_CPUTime[1] = plTime::Now();
      PLASMA_TEST_BOOL_MSG(m_CPUTime[0] <= m_GPUTime[0], "%.6f < %.6f", m_CPUTime[0].GetSeconds(), m_GPUTime[0].GetSeconds());
      PLASMA_TEST_BOOL_MSG(m_GPUTime[0] <= m_GPUTime[1], "%.6f < %.6f", m_GPUTime[0].GetSeconds(), m_GPUTime[1].GetSeconds());
      PLASMA_TEST_BOOL_MSG(m_GPUTime[1] <= m_CPUTime[1], "%.6f < %.6f", m_GPUTime[1].GetSeconds(), m_CPUTime[1].GetSeconds());
      plTestFramework::GetInstance()->Output(plTestOutput::Message, "Timestamp results received after %d frames and %.3f seconds.", m_iFrame, (plTime::Now() - m_CPUTime[0]).AsFloatInSeconds());
      m_ImgCompFrames.Clear();
    }
  }
  plThreadUtils::Sleep(plTime::Milliseconds(16));
  if (m_iFrame > 2 && (plTime::Now() - m_CPUTime[0]).AsFloatInSeconds() > 10.0f)
  {
    PLASMA_TEST_BOOL_MSG(m_bTimestampsValid, "Timestamp results are not present after 10 seconds.");
    m_ImgCompFrames.Clear();
  }
}

static plRendererTestPipelineStates g_PipelineStatesTest;
