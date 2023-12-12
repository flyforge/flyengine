#include <RendererTest/RendererTestPCH.h>

#include "TestClass.h"
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>


plGraphicsTest::plGraphicsTest() = default;

plResult plGraphicsTest::InitializeSubTest(plInt32 iIdentifier)
{
  // initialize everything up to 'core'
  plStartup::StartupCoreSystems();
  return PLASMA_SUCCESS;
}

plResult plGraphicsTest::DeInitializeSubTest(plInt32 iIdentifier)
{
  // shut down completely
  plStartup::ShutdownCoreSystems();
  plMemoryTracker::DumpMemoryLeaks();
  return PLASMA_SUCCESS;
}

plSizeU32 plGraphicsTest::GetResolution() const
{
  return m_pWindow->GetClientAreaSize();
}

plResult plGraphicsTest::SetupRenderer()
{
  {
    plFileSystem::SetSpecialDirectory("testout", plTestFramework::GetInstance()->GetAbsOutputPath());

    plStringBuilder sBaseDir = ">sdk/Data/Base/";
    plStringBuilder sReadDir(">sdk/", plTestFramework::GetInstance()->GetRelTestDataPath());
    sReadDir.PathParentDirectory();

    PLASMA_SUCCEED_OR_RETURN(plFileSystem::AddDataDirectory(">appdir/", "ShaderCache", "shadercache", plFileSystem::AllowWrites)); // for shader files

    PLASMA_SUCCEED_OR_RETURN(plFileSystem::AddDataDirectory(sBaseDir, "Base"));

    PLASMA_SUCCEED_OR_RETURN(plFileSystem::AddDataDirectory(">pltest/", "ImageComparisonDataDir", "imgout", plFileSystem::AllowWrites));

    PLASMA_SUCCEED_OR_RETURN(plFileSystem::AddDataDirectory(sReadDir, "UnitTestData"));

    sReadDir.Set(">sdk/", plTestFramework::GetInstance()->GetRelTestDataPath());
    PLASMA_SUCCEED_OR_RETURN(plFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir"));
  }

//#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
  //  constexpr const char* szDefaultRenderer = "Vulkan";
//#else
  constexpr const char* szDefaultRenderer = "DX11";
//#endif

  plStringView sRendererName = plCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);
  const char* szShaderModel = "";
  const char* szShaderCompiler = "";
  plGALDeviceFactory::GetShaderModelAndCompiler(sRendererName, szShaderModel, szShaderCompiler);

  plShaderManager::Configure(szShaderModel, true);
  PLASMA_VERIFY(plPlugin::LoadPlugin(szShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", szShaderCompiler);

  // Create a device
  {
    plGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bDebugDevice = false;
    m_pDevice = plGALDeviceFactory::CreateDevice(sRendererName, plFoundation::GetDefaultAllocator(), DeviceInit);
    if (m_pDevice->Init().Failed())
      return PLASMA_FAILURE;

    plGALDevice::SetDefaultDevice(m_pDevice);
  }

  if (sRendererName.IsEqual_NoCase("DX11"))
  {
    if (m_pDevice->GetCapabilities().m_sAdapterName == "Microsoft Basic Render Driver" || m_pDevice->GetCapabilities().m_sAdapterName.StartsWith_NoCase("Intel(R) UHD Graphics"))
    {
      // Use different images for comparison when running the D3D11 Reference Device
      plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_D3D11Ref");
    }
    else if (m_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("AMD") || m_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Radeon"))
    {
      // Line rendering is different on AMD and requires separate images for tests rendering lines.
      plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_AMD");
    }
    else
    {
      plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("");
    }
  }
  else if (sRendererName.IsEqual_NoCase("Vulkan"))
  {
    if (m_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("llvmpipe"))
    {
      plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_LLVMPIPE");
    }
    else
    {
      plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_Vulkan");
    }
  }

  m_hObjectTransformCB = plRenderContext::CreateConstantBufferStorage<ObjectCB>();
  m_hShader = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/Default.plShader");

  plStartup::StartupHighLevelSystems();
  return PLASMA_SUCCESS;
}

plResult plGraphicsTest::CreateWindow(plUInt32 uiResolutionX, plUInt32 uiResolutionY)
{
  // Create a window for rendering
  {
    plWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width = uiResolutionX;
    WindowCreationDesc.m_Resolution.height = uiResolutionY;
    WindowCreationDesc.m_bShowMouseCursor = true;
    m_pWindow = PLASMA_DEFAULT_NEW(plWindow);
    if (m_pWindow->Initialize(WindowCreationDesc).Failed())
      return PLASMA_FAILURE;
  }

  // Create a Swapchain
  {
    plGALWindowSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow = m_pWindow;
    swapChainDesc.m_SampleCount = plGALMSAASampleCount::None;
    swapChainDesc.m_bAllowScreenshots = true;
    m_hSwapChain = plGALWindowSwapChain::Create(swapChainDesc);
    if (m_hSwapChain.IsInvalidated())
    {
      return PLASMA_FAILURE;
    }
  }

  {
    plGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth = uiResolutionX;
    texDesc.m_uiHeight = uiResolutionY;
    texDesc.m_Format = plGALResourceFormat::D24S8;
    texDesc.m_bCreateRenderTarget = true;

    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
    if (m_hDepthStencilTexture.IsInvalidated())
    {
      return PLASMA_FAILURE;
    }
  }

  return PLASMA_SUCCESS;
}

void plGraphicsTest::ShutdownRenderer()
{
  PLASMA_ASSERT_DEV(m_pWindow == nullptr, "DestroyWindow needs to be called before ShutdownRenderer");
  m_hShader.Invalidate();

  plRenderContext::DeleteConstantBufferStorage(m_hObjectTransformCB);
  m_hObjectTransformCB.Invalidate();

  plStartup::ShutdownHighLevelSystems();

  plResourceManager::FreeAllUnusedResources();

  if (m_pDevice)
  {
    m_pDevice->Shutdown().IgnoreResult();
    PLASMA_DEFAULT_DELETE(m_pDevice);
  }

  plFileSystem::RemoveDataDirectoryGroup("ImageComparisonDataDir");
}

void plGraphicsTest::DestroyWindow()
{
  if (m_pDevice)
  {
    if (!m_hSwapChain.IsInvalidated())
    {
      m_pDevice->DestroySwapChain(m_hSwapChain);
      m_hSwapChain.Invalidate();
    }

    if (!m_hDepthStencilTexture.IsInvalidated())
    {
      m_pDevice->DestroyTexture(m_hDepthStencilTexture);
      m_hDepthStencilTexture.Invalidate();
    }
    m_pDevice->WaitIdle();
  }


  if (m_pWindow)
  {
    m_pWindow->Destroy().IgnoreResult();
    PLASMA_DEFAULT_DELETE(m_pWindow);
  }
}

void plGraphicsTest::BeginFrame()
{
  m_pDevice->BeginFrame();
  m_pDevice->BeginPipeline("GraphicsTest", m_hSwapChain);
}

void plGraphicsTest::EndFrame()
{
  m_pWindow->ProcessWindowMessages();

  plRenderContext::GetDefaultInstance()->EndRendering();
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;

  plRenderContext::GetDefaultInstance()->ResetContextState();
  m_pDevice->EndPipeline(m_hSwapChain);

  m_pDevice->EndFrame();

  plTaskSystem::FinishFrameTasks();
}

plResult plGraphicsTest::GetImage(plImage& ref_img)
{
  auto pCommandEncoder = plRenderContext::GetDefaultInstance()->GetCommandEncoder();

  plGALTextureHandle hBBTexture = m_pDevice->GetSwapChain(m_hSwapChain)->GetBackBufferTexture();
  const plGALTexture* pBackbuffer = plGALDevice::GetDefaultDevice()->GetTexture(hBBTexture);
  pCommandEncoder->ReadbackTexture(hBBTexture);
  const plEnum<plGALResourceFormat> format = pBackbuffer->GetDescription().m_Format;

  plImageHeader header;
  header.SetWidth(m_pWindow->GetClientAreaSize().width);
  header.SetHeight(m_pWindow->GetClientAreaSize().height);
  header.SetImageFormat(plTextureUtils::GalFormatToImageFormat(format, true));
  ref_img.ResetAndAlloc(header);

  plGALSystemMemoryDescription MemDesc;
  MemDesc.m_pData = ref_img.GetPixelPointer<plUInt8>();
  MemDesc.m_uiRowPitch = 4 * m_pWindow->GetClientAreaSize().width;
  MemDesc.m_uiSlicePitch = 4 * m_pWindow->GetClientAreaSize().width * m_pWindow->GetClientAreaSize().height;

  plArrayPtr<plGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);
  plGALTextureSubresource sourceSubResource;
  plArrayPtr<plGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
  pCommandEncoder->CopyTextureReadbackResult(hBBTexture, sourceSubResources, SysMemDescs);

  return PLASMA_SUCCESS;
}

void plGraphicsTest::ClearScreen(const plColor& color)
{
  m_pPass = m_pDevice->BeginPass("RendererTest");

  const plGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

  plGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture())).SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
  renderingSetup.m_ClearColor = color;
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_bClearDepth = true;
  renderingSetup.m_bClearStencil = true;

  plRectFloat viewport = plRectFloat(0.0f, 0.0f, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

  plRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
}

void plGraphicsTest::SetClipSpace()
{
  static plHashedString sClipSpaceFlipped = plMakeHashedString("CLIP_SPACE_FLIPPED");
  static plHashedString sTrue = plMakeHashedString("TRUE");
  static plHashedString sFalse = plMakeHashedString("FALSE");
  plClipSpaceYMode::Enum clipSpace = plClipSpaceYMode::RenderToTextureDefault;
  plRenderContext::GetDefaultInstance()->SetShaderPermutationVariable(sClipSpaceFlipped, clipSpace == plClipSpaceYMode::Flipped ? sTrue : sFalse);
}

plMeshBufferResourceHandle plGraphicsTest::CreateMesh(const plGeometry& geom, const char* szResourceName)
{
  plMeshBufferResourceHandle hMesh;
  hMesh = plResourceManager::GetExistingResource<plMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  plGALPrimitiveTopology::Enum Topology = plGALPrimitiveTopology::Triangles;
  if (geom.GetLines().GetCount() > 0)
    Topology = plGALPrimitiveTopology::Lines;

  plMeshBufferResourceDescriptor desc;
  desc.AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);
  desc.AddStream(plGALVertexAttributeSemantic::Color0, plGALResourceFormat::RGBAUByteNormalized);
  desc.AllocateStreamsFromGeometry(geom, Topology);

  hMesh = plResourceManager::GetOrCreateResource<plMeshBufferResource>(szResourceName, std::move(desc), szResourceName);

  return hMesh;
}

plMeshBufferResourceHandle plGraphicsTest::CreateSphere(plInt32 iSubDivs, float fRadius)
{
  plGeometry geom;
  geom.AddGeodesicSphere(fRadius, static_cast<plUInt8>(iSubDivs));

  plStringBuilder sName;
  sName.Format("Sphere_{0}", iSubDivs);

  return CreateMesh(geom, sName);
}

plMeshBufferResourceHandle plGraphicsTest::CreateTorus(plInt32 iSubDivs, float fInnerRadius, float fOuterRadius)
{
  plGeometry geom;
  geom.AddTorus(fInnerRadius, fOuterRadius, static_cast<plUInt16>(iSubDivs), static_cast<plUInt16>(iSubDivs), true);

  plStringBuilder sName;
  sName.Format("Torus_{0}", iSubDivs);

  return CreateMesh(geom, sName);
}

plMeshBufferResourceHandle plGraphicsTest::CreateBox(float fWidth, float fHeight, float fDepth)
{
  plGeometry geom;
  geom.AddBox(plVec3(fWidth, fHeight, fDepth), false);

  plStringBuilder sName;
  sName.Format("Box_{0}_{1}_{2}", plArgF(fWidth, 1), plArgF(fHeight, 1), plArgF(fDepth, 1));

  return CreateMesh(geom, sName);
}

plMeshBufferResourceHandle plGraphicsTest::CreateLineBox(float fWidth, float fHeight, float fDepth)
{
  plGeometry geom;
  geom.AddLineBox(plVec3(fWidth, fHeight, fDepth));

  plStringBuilder sName;
  sName.Format("LineBox_{0}_{1}_{2}", plArgF(fWidth, 1), plArgF(fHeight, 1), plArgF(fDepth, 1));

  return CreateMesh(geom, sName);
}

void plGraphicsTest::RenderObject(plMeshBufferResourceHandle hObject, const plMat4& mTransform, const plColor& color, plBitflags<plShaderBindFlags> ShaderBindFlags)
{
  plRenderContext::GetDefaultInstance()->BindShader(m_hShader, ShaderBindFlags);

  // plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  // plGALContext* pContext = pDevice->GetPrimaryContext();

  ObjectCB* ocb = plRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
  ocb->m_MVP = mTransform;
  ocb->m_Color = color;

  plRenderContext::GetDefaultInstance()->BindConstantBuffer("PerObject", m_hObjectTransformCB);

  plRenderContext::GetDefaultInstance()->BindMeshBuffer(hObject);
  plRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();
}
