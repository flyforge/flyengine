#include <RendererDX11/RendererDX11PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/PassDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/QueryDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/SharedTextureDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <d3d11.h>
#include <d3d11_3.h>
#include <dxgidebug.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  include <d3d11_1.h>
#endif

plInternal::NewInstance<plGALDevice> CreateDX11Device(plAllocatorBase* pAllocator, const plGALDeviceCreationDescription& description)
{
  return PLASMA_NEW(pAllocator, plGALDeviceDX11, description);
}

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(RendererDX11, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  plGALDeviceFactory::RegisterCreatorFunc("DX11", &CreateDX11Device, "DX11_SM50", "plasmaShaderCompilerHLSL");
}

ON_CORESYSTEMS_SHUTDOWN
{
  plGALDeviceFactory::UnregisterCreatorFunc("DX11");
}

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plGALDeviceDX11::plGALDeviceDX11(const plGALDeviceCreationDescription& Description)
  : plGALDevice(Description)
  // NOLINTNEXTLINE
  , m_uiFeatureLevel(D3D_FEATURE_LEVEL_9_1)
{
}

plGALDeviceDX11::~plGALDeviceDX11() = default;

// Init & shutdown functions

plResult plGALDeviceDX11::InitPlatform(DWORD dwFlags, IDXGIAdapter* pUsedAdapter)
{
  PLASMA_LOG_BLOCK("plGALDeviceDX11::InitPlatform");

retry:

  if (m_Description.m_bDebugDevice)
    dwFlags |= D3D11_CREATE_DEVICE_DEBUG;
  else
    dwFlags &= ~D3D11_CREATE_DEVICE_DEBUG;

  D3D_FEATURE_LEVEL FeatureLevels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3};
  ID3D11DeviceContext* pImmediateContext = nullptr;

  D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
  // driverType = D3D_DRIVER_TYPE_REFERENCE; // enables the Reference Device

  if (pUsedAdapter != nullptr)
  {
    // required by the specification
    driverType = D3D_DRIVER_TYPE_UNKNOWN;
  }

  // Manually step through feature levels - if a Win 7 system doesn't have the 11.1 runtime installed
  // The create device call will fail even though the 11.0 (or lower) level could've been
  // initialized successfully
  int FeatureLevelIdx = 0;
  for (FeatureLevelIdx = 0; FeatureLevelIdx < PLASMA_ARRAY_SIZE(FeatureLevels); FeatureLevelIdx++)
  {
    if (SUCCEEDED(D3D11CreateDevice(pUsedAdapter, driverType, nullptr, dwFlags, &FeatureLevels[FeatureLevelIdx], 1, D3D11_SDK_VERSION, &m_pDevice, (D3D_FEATURE_LEVEL*)&m_uiFeatureLevel, &pImmediateContext)))
    {
      break;
    }
  }

  // Nothing could be initialized:
  if (pImmediateContext == nullptr)
  {
    if (m_Description.m_bDebugDevice)
    {
      plLog::Warning("Couldn't initialize D3D11 debug device!");

      m_Description.m_bDebugDevice = false;
      goto retry;
    }

    plLog::Error("Couldn't initialize D3D11 device!");
    return PLASMA_FAILURE;
  }
  else
  {
    m_pImmediateContext = pImmediateContext;

    const char* FeatureLevelNames[] = {"11.1", "11.0", "10.1", "10", "9.3"};

    PLASMA_CHECK_AT_COMPILETIME(PLASMA_ARRAY_SIZE(FeatureLevels) == PLASMA_ARRAY_SIZE(FeatureLevelNames));

    plLog::Success("Initialized D3D11 device with feature level {0}.", FeatureLevelNames[FeatureLevelIdx]);
  }

  if (m_Description.m_bDebugDevice)
  {
    if (SUCCEEDED(m_pDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&m_pDebug)))
    {
      ID3D11InfoQueue* pInfoQueue = nullptr;
      if (SUCCEEDED(m_pDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue)))
      {
        // only do this when a debugger is attached, otherwise the app would crash on every DX error
        if (IsDebuggerPresent())
        {
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);
        }

        // Ignore list.
        {
          D3D11_MESSAGE_ID hide[] = {
            // Hide messages about abandoned query results. This can easily happen when a GPUStopwatch is suddenly unused.
            D3D11_MESSAGE_ID_QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS, D3D11_MESSAGE_ID_QUERY_END_ABANDONING_PREVIOUS_RESULTS,
            // Don't break on invalid input assembly. This can easily happen when using the wrong mesh-material combination.
            D3D11_MESSAGE_ID_CREATEINPUTLAYOUT_MISSINGELEMENT,
            // Add more message IDs here as needed
          };
          D3D11_INFO_QUEUE_FILTER filter;
          plMemoryUtils::ZeroFill(&filter, 1);
          filter.DenyList.NumIDs = _countof(hide);
          filter.DenyList.pIDList = hide;
          pInfoQueue->AddStorageFilterEntries(&filter);
        }

        pInfoQueue->Release();
      }
    }
  }


  // Create default pass
  m_pDefaultPass = PLASMA_NEW(&m_Allocator, plGALPassDX11, *this);

  if (FAILED(m_pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)&m_pDXGIDevice)))
  {
    plLog::Error("Couldn't get the DXGIDevice1 interface of the D3D11 device - this may happen when running on Windows Vista without SP2 "
                 "installed!");
    return PLASMA_FAILURE;
  }

  if (FAILED(m_pDevice->QueryInterface(__uuidof(ID3D11Device3), (void**)&m_pDevice3)))
  {
    plLog::Info("D3D device doesn't support ID3D11Device3, some features might be unavailable.");
  }

  if (FAILED(m_pDXGIDevice->SetMaximumFrameLatency(1)))
  {
    plLog::Warning("Failed to set max frames latency");
  }

  if (FAILED(m_pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&m_pDXGIAdapter)))
  {
    return PLASMA_FAILURE;
  }

  if (FAILED(m_pDXGIAdapter->GetParent(__uuidof(IDXGIFactory1), (void**)&m_pDXGIFactory)))
  {
    return PLASMA_FAILURE;
  }

  // Fill lookup table
  FillFormatLookupTable();

  plClipSpaceDepthRange::Default = plClipSpaceDepthRange::ZeroToOne;
  plClipSpaceYMode::RenderToTextureDefault = plClipSpaceYMode::Regular;

  // Per frame data & timer data
  D3D11_QUERY_DESC disjointQueryDesc;
  disjointQueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
  disjointQueryDesc.MiscFlags = 0;

  D3D11_QUERY_DESC timerQueryDesc;
  timerQueryDesc.Query = D3D11_QUERY_TIMESTAMP;
  timerQueryDesc.MiscFlags = 0;

  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    auto& perFrameData = m_PerFrameData[i];

    D3D11_QUERY_DESC QueryDesc;
    QueryDesc.Query = D3D11_QUERY_EVENT;
    QueryDesc.MiscFlags = 0;
    if (SUCCEEDED(GetDXDevice()->CreateQuery(&QueryDesc, &perFrameData.m_pFence)))

      if (FAILED(m_pDevice->CreateQuery(&disjointQueryDesc, &perFrameData.m_pDisjointTimerQuery)))
      {
        plLog::Error("Creation of native DirectX query for disjoint query has failed!");
        return PLASMA_FAILURE;
      }
  }

  // #TODO_DX11 Replace ring buffer with proper pool like in Vulkan to prevent buffer overrun.
  m_Timestamps.SetCountUninitialized(2048);
  for (plUInt32 i = 0; i < m_Timestamps.GetCount(); ++i)
  {
    if (FAILED(m_pDevice->CreateQuery(&timerQueryDesc, &m_Timestamps[i])))
    {
      plLog::Error("Creation of native DirectX query for timestamp has failed!");
      return PLASMA_FAILURE;
    }
  }

  m_SyncTimeDiff = plTime::MakeZero();

  plGALWindowSwapChain::SetFactoryMethod([this](const plGALWindowSwapChainCreationDescription& desc) -> plGALSwapChainHandle { return CreateSwapChain([this, &desc](plAllocatorBase* pAllocator) -> plGALSwapChain* { return PLASMA_NEW(pAllocator, plGALSwapChainDX11, desc); }); });

  return PLASMA_SUCCESS;
}

plResult plGALDeviceDX11::InitPlatform()
{
  return InitPlatform(0, nullptr);
}

void plGALDeviceDX11::ReportLiveGpuObjects()
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  // not implemented
  return;

#else

  const HMODULE hDxgiDebugDLL = LoadLibraryW(L"Dxgidebug.dll");

  if (hDxgiDebugDLL == nullptr)
    return;

  using FnGetDebugInterfacePtr = HRESULT(WINAPI*)(REFIID, void**);
  FnGetDebugInterfacePtr GetDebugInterfacePtr = (FnGetDebugInterfacePtr)GetProcAddress(hDxgiDebugDLL, "DXGIGetDebugInterface");

  if (GetDebugInterfacePtr == nullptr)
    return;

  IDXGIDebug* dxgiDebug = nullptr;
  GetDebugInterfacePtr(IID_PPV_ARGS(&dxgiDebug));

  if (dxgiDebug == nullptr)
    return;

  OutputDebugStringW(L" +++++ Live DX11 Objects: +++++\n");

  // prints to OutputDebugString
  dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);

  OutputDebugStringW(L" ----- Live DX11 Objects: -----\n");

  dxgiDebug->Release();

#endif
}

void plGALDeviceDX11::FlushDeadObjects()
{
  DestroyDeadObjects();
}

plResult plGALDeviceDX11::ShutdownPlatform()
{
  plGALWindowSwapChain::SetFactoryMethod({});
  for (plUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    for (auto it = m_FreeTempResources[type].GetIterator(); it.IsValid(); ++it)
    {
      plDynamicArray<ID3D11Resource*>& resources = it.Value();
      for (auto pResource : resources)
      {
        PLASMA_GAL_DX11_RELEASE(pResource);
      }
    }
    m_FreeTempResources[type].Clear();

    for (auto& tempResource : m_UsedTempResources[type])
    {
      PLASMA_GAL_DX11_RELEASE(tempResource.m_pResource);
    }
    m_UsedTempResources[type].Clear();
  }

  for (auto& timestamp : m_Timestamps)
  {
    PLASMA_GAL_DX11_RELEASE(timestamp);
  }
  m_Timestamps.Clear();

  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    auto& perFrameData = m_PerFrameData[i];

    PLASMA_GAL_DX11_RELEASE(perFrameData.m_pFence);
    perFrameData.m_pFence = nullptr;

    PLASMA_GAL_DX11_RELEASE(perFrameData.m_pDisjointTimerQuery);
  }

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  // Force immediate destruction of all objects destroyed so far.
  // This is necessary if we want to create a new primary swap chain/device right after this.
  // See: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476425(v=vs.85).aspx#Defer_Issues_with_Flip
  // Strictly speaking we should do this right after we destroy the swap chain and flush all contexts that are affected.
  // However, the particular usecase where this problem comes up is usually a restart scenario.
  if (m_pImmediateContext != nullptr)
  {
    m_pImmediateContext->ClearState();
    m_pImmediateContext->Flush();
  }
#endif

  m_pDefaultPass = nullptr;

  PLASMA_GAL_DX11_RELEASE(m_pImmediateContext);
  PLASMA_GAL_DX11_RELEASE(m_pDevice3);
  PLASMA_GAL_DX11_RELEASE(m_pDevice);
  PLASMA_GAL_DX11_RELEASE(m_pDebug);
  PLASMA_GAL_DX11_RELEASE(m_pDXGIFactory);
  PLASMA_GAL_DX11_RELEASE(m_pDXGIAdapter);
  PLASMA_GAL_DX11_RELEASE(m_pDXGIDevice);

  ReportLiveGpuObjects();

  return PLASMA_SUCCESS;
}

// Pipeline & Pass functions

void plGALDeviceDX11::BeginPipelinePlatform(const char* szName, plGALSwapChain* pSwapChain)
{
#if PLASMA_ENABLED(PLASMA_USE_PROFILING)
  m_pPipelineTimingScope = plProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif

  if (pSwapChain)
  {
    pSwapChain->AcquireNextRenderTarget(this);
  }
}

void plGALDeviceDX11::EndPipelinePlatform(plGALSwapChain* pSwapChain)
{
  if (pSwapChain)
  {
    pSwapChain->PresentRenderTarget(this);
  }

#if PLASMA_ENABLED(PLASMA_USE_PROFILING)
  plProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPipelineTimingScope);
#endif
}

plGALPass* plGALDeviceDX11::BeginPassPlatform(const char* szName)
{
#if PLASMA_ENABLED(PLASMA_USE_PROFILING)
  m_pPassTimingScope = plProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif

  m_pDefaultPass->BeginPass(szName);

  return m_pDefaultPass.Borrow();
}

void plGALDeviceDX11::EndPassPlatform(plGALPass* pPass)
{
  PLASMA_ASSERT_DEV(m_pDefaultPass.Borrow() == pPass, "Invalid pass");

#if PLASMA_ENABLED(PLASMA_USE_PROFILING)
  plProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPassTimingScope);
#endif

  m_pDefaultPass->EndPass();
}

void plGALDeviceDX11::FlushPlatform()
{
  m_pImmediateContext->Flush();
}

// State creation functions

plGALBlendState* plGALDeviceDX11::CreateBlendStatePlatform(const plGALBlendStateCreationDescription& Description)
{
  plGALBlendStateDX11* pState = PLASMA_NEW(&m_Allocator, plGALBlendStateDX11, Description);

  if (pState->InitPlatform(this).Succeeded())
  {
    return pState;
  }
  else
  {
    PLASMA_DELETE(&m_Allocator, pState);
    return nullptr;
  }
}

void plGALDeviceDX11::DestroyBlendStatePlatform(plGALBlendState* pBlendState)
{
  plGALBlendStateDX11* pState = static_cast<plGALBlendStateDX11*>(pBlendState);
  pState->DeInitPlatform(this).IgnoreResult();
  PLASMA_DELETE(&m_Allocator, pState);
}

plGALDepthStencilState* plGALDeviceDX11::CreateDepthStencilStatePlatform(const plGALDepthStencilStateCreationDescription& Description)
{
  plGALDepthStencilStateDX11* pDX11DepthStencilState = PLASMA_NEW(&m_Allocator, plGALDepthStencilStateDX11, Description);

  if (pDX11DepthStencilState->InitPlatform(this).Succeeded())
  {
    return pDX11DepthStencilState;
  }
  else
  {
    PLASMA_DELETE(&m_Allocator, pDX11DepthStencilState);
    return nullptr;
  }
}

void plGALDeviceDX11::DestroyDepthStencilStatePlatform(plGALDepthStencilState* pDepthStencilState)
{
  plGALDepthStencilStateDX11* pDX11DepthStencilState = static_cast<plGALDepthStencilStateDX11*>(pDepthStencilState);
  pDX11DepthStencilState->DeInitPlatform(this).IgnoreResult();
  PLASMA_DELETE(&m_Allocator, pDX11DepthStencilState);
}

plGALRasterizerState* plGALDeviceDX11::CreateRasterizerStatePlatform(const plGALRasterizerStateCreationDescription& Description)
{
  plGALRasterizerStateDX11* pDX11RasterizerState = PLASMA_NEW(&m_Allocator, plGALRasterizerStateDX11, Description);

  if (pDX11RasterizerState->InitPlatform(this).Succeeded())
  {
    return pDX11RasterizerState;
  }
  else
  {
    PLASMA_DELETE(&m_Allocator, pDX11RasterizerState);
    return nullptr;
  }
}

void plGALDeviceDX11::DestroyRasterizerStatePlatform(plGALRasterizerState* pRasterizerState)
{
  plGALRasterizerStateDX11* pDX11RasterizerState = static_cast<plGALRasterizerStateDX11*>(pRasterizerState);
  pDX11RasterizerState->DeInitPlatform(this).IgnoreResult();
  PLASMA_DELETE(&m_Allocator, pDX11RasterizerState);
}

plGALSamplerState* plGALDeviceDX11::CreateSamplerStatePlatform(const plGALSamplerStateCreationDescription& Description)
{
  plGALSamplerStateDX11* pDX11SamplerState = PLASMA_NEW(&m_Allocator, plGALSamplerStateDX11, Description);

  if (pDX11SamplerState->InitPlatform(this).Succeeded())
  {
    return pDX11SamplerState;
  }
  else
  {
    PLASMA_DELETE(&m_Allocator, pDX11SamplerState);
    return nullptr;
  }
}

void plGALDeviceDX11::DestroySamplerStatePlatform(plGALSamplerState* pSamplerState)
{
  plGALSamplerStateDX11* pDX11SamplerState = static_cast<plGALSamplerStateDX11*>(pSamplerState);
  pDX11SamplerState->DeInitPlatform(this).IgnoreResult();
  PLASMA_DELETE(&m_Allocator, pDX11SamplerState);
}


// Resource creation functions

plGALShader* plGALDeviceDX11::CreateShaderPlatform(const plGALShaderCreationDescription& Description)
{
  plGALShaderDX11* pShader = PLASMA_NEW(&m_Allocator, plGALShaderDX11, Description);

  if (!pShader->InitPlatform(this).Succeeded())
  {
    PLASMA_DELETE(&m_Allocator, pShader);
    return nullptr;
  }

  return pShader;
}

void plGALDeviceDX11::DestroyShaderPlatform(plGALShader* pShader)
{
  plGALShaderDX11* pDX11Shader = static_cast<plGALShaderDX11*>(pShader);
  pDX11Shader->DeInitPlatform(this).IgnoreResult();
  PLASMA_DELETE(&m_Allocator, pDX11Shader);
}

plGALBuffer* plGALDeviceDX11::CreateBufferPlatform(const plGALBufferCreationDescription& Description, plArrayPtr<const plUInt8> pInitialData)
{
  plGALBufferDX11* pBuffer = PLASMA_NEW(&m_Allocator, plGALBufferDX11, Description);

  if (!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    PLASMA_DELETE(&m_Allocator, pBuffer);
    return nullptr;
  }

  return pBuffer;
}

void plGALDeviceDX11::DestroyBufferPlatform(plGALBuffer* pBuffer)
{
  plGALBufferDX11* pDX11Buffer = static_cast<plGALBufferDX11*>(pBuffer);
  pDX11Buffer->DeInitPlatform(this).IgnoreResult();
  PLASMA_DELETE(&m_Allocator, pDX11Buffer);
}

plGALTexture* plGALDeviceDX11::CreateTexturePlatform(const plGALTextureCreationDescription& Description, plArrayPtr<plGALSystemMemoryDescription> pInitialData)
{
  plGALTextureDX11* pTexture = PLASMA_NEW(&m_Allocator, plGALTextureDX11, Description);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    PLASMA_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void plGALDeviceDX11::DestroyTexturePlatform(plGALTexture* pTexture)
{
  plGALTextureDX11* pDX11Texture = static_cast<plGALTextureDX11*>(pTexture);
  pDX11Texture->DeInitPlatform(this).IgnoreResult();
  PLASMA_DELETE(&m_Allocator, pDX11Texture);
}

plGALTexture* plGALDeviceDX11::CreateSharedTexturePlatform(const plGALTextureCreationDescription& Description, plArrayPtr<plGALSystemMemoryDescription> pInitialData, plEnum<plGALSharedTextureType> sharedType, plGALPlatformSharedHandle handle)
{
  plGALSharedTextureDX11* pTexture = PLASMA_NEW(&m_Allocator, plGALSharedTextureDX11, Description, sharedType, handle);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    PLASMA_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void plGALDeviceDX11::DestroySharedTexturePlatform(plGALTexture* pTexture)
{
  plGALSharedTextureDX11* pDX11Texture = static_cast<plGALSharedTextureDX11*>(pTexture);
  pDX11Texture->DeInitPlatform(this).IgnoreResult();
  PLASMA_DELETE(&m_Allocator, pDX11Texture);
}

plGALResourceView* plGALDeviceDX11::CreateResourceViewPlatform(plGALResourceBase* pResource, const plGALResourceViewCreationDescription& Description)
{
  plGALResourceViewDX11* pResourceView = PLASMA_NEW(&m_Allocator, plGALResourceViewDX11, pResource, Description);

  if (!pResourceView->InitPlatform(this).Succeeded())
  {
    PLASMA_DELETE(&m_Allocator, pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void plGALDeviceDX11::DestroyResourceViewPlatform(plGALResourceView* pResourceView)
{
  plGALResourceViewDX11* pDX11ResourceView = static_cast<plGALResourceViewDX11*>(pResourceView);
  pDX11ResourceView->DeInitPlatform(this).IgnoreResult();
  PLASMA_DELETE(&m_Allocator, pDX11ResourceView);
}

plGALRenderTargetView* plGALDeviceDX11::CreateRenderTargetViewPlatform(plGALTexture* pTexture, const plGALRenderTargetViewCreationDescription& Description)
{
  plGALRenderTargetViewDX11* pRTView = PLASMA_NEW(&m_Allocator, plGALRenderTargetViewDX11, pTexture, Description);

  if (!pRTView->InitPlatform(this).Succeeded())
  {
    PLASMA_DELETE(&m_Allocator, pRTView);
    return nullptr;
  }

  return pRTView;
}

void plGALDeviceDX11::DestroyRenderTargetViewPlatform(plGALRenderTargetView* pRenderTargetView)
{
  plGALRenderTargetViewDX11* pDX11RenderTargetView = static_cast<plGALRenderTargetViewDX11*>(pRenderTargetView);
  pDX11RenderTargetView->DeInitPlatform(this).IgnoreResult();
  PLASMA_DELETE(&m_Allocator, pDX11RenderTargetView);
}

plGALUnorderedAccessView* plGALDeviceDX11::CreateUnorderedAccessViewPlatform(plGALResourceBase* pTextureOfBuffer, const plGALUnorderedAccessViewCreationDescription& Description)
{
  plGALUnorderedAccessViewDX11* pUnorderedAccessView = PLASMA_NEW(&m_Allocator, plGALUnorderedAccessViewDX11, pTextureOfBuffer, Description);

  if (!pUnorderedAccessView->InitPlatform(this).Succeeded())
  {
    PLASMA_DELETE(&m_Allocator, pUnorderedAccessView);
    return nullptr;
  }

  return pUnorderedAccessView;
}

void plGALDeviceDX11::DestroyUnorderedAccessViewPlatform(plGALUnorderedAccessView* pUnorderedAccessView)
{
  plGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<plGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  pUnorderedAccessViewDX11->DeInitPlatform(this).IgnoreResult();
  PLASMA_DELETE(&m_Allocator, pUnorderedAccessViewDX11);
}



// Other rendering creation functions

plGALQuery* plGALDeviceDX11::CreateQueryPlatform(const plGALQueryCreationDescription& Description)
{
  plGALQueryDX11* pQuery = PLASMA_NEW(&m_Allocator, plGALQueryDX11, Description);

  if (!pQuery->InitPlatform(this).Succeeded())
  {
    PLASMA_DELETE(&m_Allocator, pQuery);
    return nullptr;
  }

  return pQuery;
}

void plGALDeviceDX11::DestroyQueryPlatform(plGALQuery* pQuery)
{
  plGALQueryDX11* pQueryDX11 = static_cast<plGALQueryDX11*>(pQuery);
  pQueryDX11->DeInitPlatform(this).IgnoreResult();
  PLASMA_DELETE(&m_Allocator, pQueryDX11);
}

plGALVertexDeclaration* plGALDeviceDX11::CreateVertexDeclarationPlatform(const plGALVertexDeclarationCreationDescription& Description)
{
  plGALVertexDeclarationDX11* pVertexDeclaration = PLASMA_NEW(&m_Allocator, plGALVertexDeclarationDX11, Description);

  if (pVertexDeclaration->InitPlatform(this).Succeeded())
  {
    return pVertexDeclaration;
  }
  else
  {
    PLASMA_DELETE(&m_Allocator, pVertexDeclaration);
    return nullptr;
  }
}

void plGALDeviceDX11::DestroyVertexDeclarationPlatform(plGALVertexDeclaration* pVertexDeclaration)
{
  plGALVertexDeclarationDX11* pVertexDeclarationDX11 = static_cast<plGALVertexDeclarationDX11*>(pVertexDeclaration);
  pVertexDeclarationDX11->DeInitPlatform(this).IgnoreResult();
  PLASMA_DELETE(&m_Allocator, pVertexDeclarationDX11);
}

plGALTimestampHandle plGALDeviceDX11::GetTimestampPlatform()
{
  plUInt32 uiIndex = m_uiNextTimestamp;
  m_uiNextTimestamp = (m_uiNextTimestamp + 1) % m_Timestamps.GetCount();
  return {uiIndex, m_uiFrameCounter};
}

plResult plGALDeviceDX11::GetTimestampResultPlatform(plGALTimestampHandle hTimestamp, plTime& result)
{
  // Check whether frequency and sync timer are already available for the frame of the timestamp
  plUInt64 uiFrameCounter = hTimestamp.m_uiFrameCounter;

  PerFrameData* pPerFrameData = nullptr;
  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    if (m_PerFrameData[i].m_uiFrame == uiFrameCounter && m_PerFrameData[i].m_fInvTicksPerSecond >= 0.0)
    {
      pPerFrameData = &m_PerFrameData[i];
      break;
    }
  }

  if (pPerFrameData == nullptr)
  {
    return PLASMA_FAILURE;
  }

  ID3D11Query* pQuery = GetTimestamp(hTimestamp);

  plUInt64 uiTimestamp;
  if (FAILED(m_pImmediateContext->GetData(pQuery, &uiTimestamp, sizeof(uiTimestamp), D3D11_ASYNC_GETDATA_DONOTFLUSH)))
  {
    return PLASMA_FAILURE;
  }

  if (pPerFrameData->m_fInvTicksPerSecond == 0.0)
  {
    result = plTime::MakeZero();
  }
  else
  {
    result = plTime::MakeFromSeconds(double(uiTimestamp) * pPerFrameData->m_fInvTicksPerSecond) + m_SyncTimeDiff;
  }
  return PLASMA_SUCCESS;
}

// Swap chain functions

void plGALDeviceDX11::PresentPlatform(const plGALSwapChain* pSwapChain, bool bVSync)
{
}

// Misc functions

void plGALDeviceDX11::BeginFramePlatform(const plUInt64 uiRenderFrame)
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

  plStringBuilder sb;
  sb.Format("Frame {}", uiRenderFrame);

#if PLASMA_ENABLED(PLASMA_USE_PROFILING)
  m_pFrameTimingScope = plProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), sb);
#endif

  // check if fence is reached and wait if the disjoint timer is about to be re-used
  {
    auto& perFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    if (perFrameData.m_uiFrame != ((plUInt64)-1))
    {

      bool bFenceReached = IsFenceReachedPlatform(GetDXImmediateContext(), perFrameData.m_pFence);
      if (!bFenceReached && m_uiNextPerFrameData == m_uiCurrentPerFrameData)
      {
        WaitForFencePlatform(GetDXImmediateContext(), perFrameData.m_pFence);
      }
    }
  }

  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    m_pImmediateContext->Begin(perFrameData.m_pDisjointTimerQuery);

    perFrameData.m_fInvTicksPerSecond = -1.0f;
  }
}

void plGALDeviceDX11::EndFramePlatform()
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

#if PLASMA_ENABLED(PLASMA_USE_PROFILING)
  plProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pFrameTimingScope);
#endif

  // end disjoint query
  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    m_pImmediateContext->End(perFrameData.m_pDisjointTimerQuery);
  }

  // check if fence is reached and update per frame data
  {
    auto& perFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    if (perFrameData.m_uiFrame != ((plUInt64)-1))
    {
      if (IsFenceReachedPlatform(GetDXImmediateContext(), perFrameData.m_pFence))
      {
        FreeTempResources(perFrameData.m_uiFrame);

        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
        if (FAILED(m_pImmediateContext->GetData(perFrameData.m_pDisjointTimerQuery, &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH)) || data.Disjoint)
        {
          perFrameData.m_fInvTicksPerSecond = 0.0f;
        }
        else
        {
          perFrameData.m_fInvTicksPerSecond = 1.0 / (double)data.Frequency;

          if (m_bSyncTimeNeeded)
          {
            plGALTimestampHandle hTimestamp = m_pDefaultPass->m_pRenderCommandEncoder->InsertTimestamp();
            ID3D11Query* pQuery = GetTimestamp(hTimestamp);

            plUInt64 uiTimestamp;
            while (m_pImmediateContext->GetData(pQuery, &uiTimestamp, sizeof(uiTimestamp), 0) != S_OK)
            {
              plThreadUtils::YieldTimeSlice();
            }

            m_SyncTimeDiff = plTime::Now() - plTime::MakeFromSeconds(double(uiTimestamp) * perFrameData.m_fInvTicksPerSecond);
            m_bSyncTimeNeeded = false;
          }
        }

        m_uiCurrentPerFrameData = (m_uiCurrentPerFrameData + 1) % PLASMA_ARRAY_SIZE(m_PerFrameData);
      }
    }
  }

  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    perFrameData.m_uiFrame = m_uiFrameCounter;

    // insert fence
    InsertFencePlatform(GetDXImmediateContext(), perFrameData.m_pFence);

    m_uiNextPerFrameData = (m_uiNextPerFrameData + 1) % PLASMA_ARRAY_SIZE(m_PerFrameData);
  }

  ++m_uiFrameCounter;
}

void plGALDeviceDX11::FillCapabilitiesPlatform()
{
  {
    DXGI_ADAPTER_DESC1 adapterDesc;
    m_pDXGIAdapter->GetDesc1(&adapterDesc);

    m_Capabilities.m_sAdapterName = plStringUtf8(adapterDesc.Description).GetData();
    m_Capabilities.m_uiDedicatedVRAM = static_cast<plUInt64>(adapterDesc.DedicatedVideoMemory);
    m_Capabilities.m_uiDedicatedSystemRAM = static_cast<plUInt64>(adapterDesc.DedicatedSystemMemory);
    m_Capabilities.m_uiSharedSystemRAM = static_cast<plUInt64>(adapterDesc.SharedSystemMemory);
    m_Capabilities.m_bHardwareAccelerated = (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0;
  }

  m_Capabilities.m_bMultithreadedResourceCreation = true;

  switch (m_uiFeatureLevel)
  {
    case D3D_FEATURE_LEVEL_11_1:
      m_Capabilities.m_bNoOverwriteBufferUpdate = true;
      [[fallthrough]];

    case D3D_FEATURE_LEVEL_11_0:
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::VertexShader] = true;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::HullShader] = true;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::DomainShader] = true;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::GeometryShader] = true;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::PixelShader] = true;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::ComputeShader] = true;
      m_Capabilities.m_bInstancing = true;
      m_Capabilities.m_b32BitIndices = true;
      m_Capabilities.m_bIndirectDraw = true;
      m_Capabilities.m_uiMaxConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
      m_Capabilities.m_bTextureArrays = true;
      m_Capabilities.m_bCubemapArrays = true;
      m_Capabilities.m_bSharedTextures = true;
      m_Capabilities.m_uiMaxTextureDimension = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
      m_Capabilities.m_uiMaxCubemapDimension = D3D11_REQ_TEXTURECUBE_DIMENSION;
      m_Capabilities.m_uiMax3DTextureDimension = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
      m_Capabilities.m_uiMaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
      m_Capabilities.m_uiMaxRendertargets = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
      m_Capabilities.m_uiUAVCount = (m_uiFeatureLevel == D3D_FEATURE_LEVEL_11_1 ? 64 : 8);
      m_Capabilities.m_bAlphaToCoverage = true;
      break;

    case D3D_FEATURE_LEVEL_10_1:
    case D3D_FEATURE_LEVEL_10_0:
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::VertexShader] = true;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::HullShader] = false;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::DomainShader] = false;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::GeometryShader] = true;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::PixelShader] = true;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::ComputeShader] = false;
      m_Capabilities.m_bInstancing = true;
      m_Capabilities.m_b32BitIndices = true;
      m_Capabilities.m_bIndirectDraw = false;
      m_Capabilities.m_uiMaxConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
      m_Capabilities.m_bTextureArrays = true;
      m_Capabilities.m_bCubemapArrays = (m_uiFeatureLevel == D3D_FEATURE_LEVEL_10_1 ? true : false);
      m_Capabilities.m_uiMaxTextureDimension = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
      m_Capabilities.m_uiMaxCubemapDimension = D3D10_REQ_TEXTURECUBE_DIMENSION;
      m_Capabilities.m_uiMax3DTextureDimension = D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
      m_Capabilities.m_uiMaxAnisotropy = D3D10_REQ_MAXANISOTROPY;
      m_Capabilities.m_uiMaxRendertargets = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
      m_Capabilities.m_uiUAVCount = 0;
      m_Capabilities.m_bAlphaToCoverage = true;
      break;

    case D3D_FEATURE_LEVEL_9_3:
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::VertexShader] = true;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::HullShader] = false;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::DomainShader] = false;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::GeometryShader] = false;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::PixelShader] = true;
      m_Capabilities.m_bShaderStageSupported[plGALShaderStage::ComputeShader] = false;
      m_Capabilities.m_bInstancing = true;
      m_Capabilities.m_b32BitIndices = true;
      m_Capabilities.m_bIndirectDraw = false;
      m_Capabilities.m_uiMaxConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
      m_Capabilities.m_bTextureArrays = false;
      m_Capabilities.m_bCubemapArrays = false;
      m_Capabilities.m_uiMaxTextureDimension = D3D_FL9_3_REQ_TEXTURE1D_U_DIMENSION;
      m_Capabilities.m_uiMaxCubemapDimension = D3D_FL9_3_REQ_TEXTURECUBE_DIMENSION;
      m_Capabilities.m_uiMax3DTextureDimension = 0;
      m_Capabilities.m_uiMaxAnisotropy = 16;
      m_Capabilities.m_uiMaxRendertargets = D3D_FL9_3_SIMULTANEOUS_RENDER_TARGET_COUNT;
      m_Capabilities.m_uiUAVCount = 0;
      m_Capabilities.m_bAlphaToCoverage = false;
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  if (m_pDevice3)
  {
    D3D11_FEATURE_DATA_D3D11_OPTIONS2 featureOpts2;
    if (SUCCEEDED(m_pDevice3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &featureOpts2, sizeof(featureOpts2))))
    {
      m_Capabilities.m_bConservativeRasterization = (featureOpts2.ConservativeRasterizationTier != D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED);
    }

    D3D11_FEATURE_DATA_D3D11_OPTIONS3 featureOpts3;
    if (SUCCEEDED(m_pDevice3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &featureOpts3, sizeof(featureOpts3))))
    {
      m_Capabilities.m_bVertexShaderRenderTargetArrayIndex = featureOpts3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer != 0;
    }
  }

  m_Capabilities.m_FormatSupport.SetCount(plGALResourceFormat::ENUM_COUNT);
  for (plUInt32 i = 0; i < plGALResourceFormat::ENUM_COUNT; i++)
  {
    plGALResourceFormat::Enum format = (plGALResourceFormat::Enum)i;
    const plGALFormatLookupEntryDX11& entry = m_FormatLookupTable.GetFormatInfo(format);
    const bool bIsDepth = plGALResourceFormat::IsDepthFormat(format);
    if (bIsDepth)
    {
      UINT uiSampleSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eDepthOnlyType, &uiSampleSupport)))
      {
        if (uiSampleSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_SHADER_SAMPLE)
          m_Capabilities.m_FormatSupport[i].Add(plGALResourceFormatSupport::Sample);
      }

      UINT uiRenderSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eDepthStencilType, &uiRenderSupport)))
      {
        if (uiRenderSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_DEPTH_STENCIL)
          m_Capabilities.m_FormatSupport[i].Add(plGALResourceFormatSupport::Render);
      }
    }
    else
    {
      UINT uiSampleSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eResourceViewType, &uiSampleSupport)))
      {
        UINT uiSampleFlag = plGALResourceFormat::IsIntegerFormat(format) ? D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_SHADER_LOAD : D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;
        if (uiSampleSupport & uiSampleFlag)
          m_Capabilities.m_FormatSupport[i].Add(plGALResourceFormatSupport::Sample);
      }

      UINT uiVertexSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eVertexAttributeType, &uiVertexSupport)))
      {
        if (uiVertexSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER)
          m_Capabilities.m_FormatSupport[i].Add(plGALResourceFormatSupport::VertexAttribute);
      }

      UINT uiRenderSupport;
      if (SUCCEEDED(m_pDevice3->CheckFormatSupport(entry.m_eRenderTarget, &uiRenderSupport)))
      {
        if (uiRenderSupport & D3D11_FORMAT_SUPPORT::D3D11_FORMAT_SUPPORT_RENDER_TARGET)
          m_Capabilities.m_FormatSupport[i].Add(plGALResourceFormatSupport::Render);
      }
    }
  }
}

void plGALDeviceDX11::WaitIdlePlatform()
{
  m_pImmediateContext->Flush();
  DestroyDeadObjects();
}

const plGALSharedTexture* plGALDeviceDX11::GetSharedTexture(plGALTextureHandle hTexture) const
{
  auto pTexture = GetTexture(hTexture);
  if (pTexture == nullptr)
  {
    return nullptr;
  }

  // Resolve proxy texture if any
  return static_cast<const plGALSharedTextureDX11*>(pTexture->GetParentResource());
}

ID3D11Resource* plGALDeviceDX11::FindTempBuffer(plUInt32 uiSize)
{
  const plUInt32 uiExpGrowthLimit = 16 * 1024 * 1024;

  uiSize = plMath::Max(uiSize, 256U);
  if (uiSize < uiExpGrowthLimit)
  {
    uiSize = plMath::PowerOfTwo_Ceil(uiSize);
  }
  else
  {
    uiSize = plMemoryUtils::AlignSize(uiSize, uiExpGrowthLimit);
  }

  ID3D11Resource* pResource = nullptr;
  auto it = m_FreeTempResources[TempResourceType::Buffer].Find(uiSize);
  if (it.IsValid())
  {
    plDynamicArray<ID3D11Resource*>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      pResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (pResource == nullptr)
  {
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = uiSize;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    ID3D11Buffer* pBuffer = nullptr;
    if (!SUCCEEDED(m_pDevice->CreateBuffer(&desc, nullptr, &pBuffer)))
    {
      return nullptr;
    }

    pResource = pBuffer;
  }

  auto& tempResource = m_UsedTempResources[TempResourceType::Buffer].ExpandAndGetRef();
  tempResource.m_pResource = pResource;
  tempResource.m_uiFrame = m_uiFrameCounter;
  tempResource.m_uiHash = uiSize;

  return pResource;
}


ID3D11Resource* plGALDeviceDX11::FindTempTexture(plUInt32 uiWidth, plUInt32 uiHeight, plUInt32 uiDepth, plGALResourceFormat::Enum format)
{
  plUInt32 data[] = {uiWidth, uiHeight, uiDepth, (plUInt32)format};
  plUInt32 uiHash = plHashingUtils::xxHash32(data, sizeof(data));

  ID3D11Resource* pResource = nullptr;
  auto it = m_FreeTempResources[TempResourceType::Texture].Find(uiHash);
  if (it.IsValid())
  {
    plDynamicArray<ID3D11Resource*>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      pResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (pResource == nullptr)
  {
    if (uiDepth == 1)
    {
      D3D11_TEXTURE2D_DESC desc;
      desc.Width = uiWidth;
      desc.Height = uiHeight;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = GetFormatLookupTable().GetFormatInfo(format).m_eStorage;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags = 0;

      ID3D11Texture2D* pTexture = nullptr;
      if (!SUCCEEDED(m_pDevice->CreateTexture2D(&desc, nullptr, &pTexture)))
      {
        return nullptr;
      }

      pResource = pTexture;
    }
    else
    {
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      return nullptr;
    }
  }

  auto& tempResource = m_UsedTempResources[TempResourceType::Texture].ExpandAndGetRef();
  tempResource.m_pResource = pResource;
  tempResource.m_uiFrame = m_uiFrameCounter;
  tempResource.m_uiHash = uiHash;

  return pResource;
}

void plGALDeviceDX11::FreeTempResources(plUInt64 uiFrame)
{
  for (plUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    while (!m_UsedTempResources[type].IsEmpty())
    {
      auto& usedTempResource = m_UsedTempResources[type].PeekFront();
      if (usedTempResource.m_uiFrame == uiFrame)
      {
        auto it = m_FreeTempResources[type].Find(usedTempResource.m_uiHash);
        if (!it.IsValid())
        {
          it = m_FreeTempResources[type].Insert(usedTempResource.m_uiHash, plDynamicArray<ID3D11Resource*>(&m_Allocator));
        }

        it.Value().PushBack(usedTempResource.m_pResource);
        m_UsedTempResources[type].PopFront();
      }
      else
      {
        break;
      }
    }
  }
}

void plGALDeviceDX11::FillFormatLookupTable()
{
  ///       The list below is in the same order as the plGALResourceFormat enum. No format should be missing except the ones that are just
  ///       different names for the same enum value.

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAFloat, plGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_FLOAT).VA(DXGI_FORMAT_R32G32B32A32_FLOAT).RV(DXGI_FORMAT_R32G32B32A32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAUInt, plGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_UINT).VA(DXGI_FORMAT_R32G32B32A32_UINT).RV(DXGI_FORMAT_R32G32B32A32_UINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAInt, plGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_SINT).VA(DXGI_FORMAT_R32G32B32A32_SINT).RV(DXGI_FORMAT_R32G32B32A32_SINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBFloat, plGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_FLOAT).VA(DXGI_FORMAT_R32G32B32_FLOAT).RV(DXGI_FORMAT_R32G32B32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBUInt, plGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_UINT).VA(DXGI_FORMAT_R32G32B32_UINT).RV(DXGI_FORMAT_R32G32B32_UINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBInt, plGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_SINT).VA(DXGI_FORMAT_R32G32B32_SINT).RV(DXGI_FORMAT_R32G32B32_SINT));

  // Supported with DX 11.1
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::B5G6R5UNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_B5G6R5_UNORM).RT(DXGI_FORMAT_B5G6R5_UNORM).VA(DXGI_FORMAT_B5G6R5_UNORM).RV(DXGI_FORMAT_B5G6R5_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BGRAUByteNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_B8G8R8A8_TYPELESS).RT(DXGI_FORMAT_B8G8R8A8_UNORM).VA(DXGI_FORMAT_B8G8R8A8_UNORM).RV(DXGI_FORMAT_B8G8R8A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BGRAUByteNormalizedsRGB, plGALFormatLookupEntryDX11(DXGI_FORMAT_B8G8R8A8_TYPELESS).RT(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB).RV(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAHalf, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_FLOAT).VA(DXGI_FORMAT_R16G16B16A16_FLOAT).RV(DXGI_FORMAT_R16G16B16A16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAUShort, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_UINT).VA(DXGI_FORMAT_R16G16B16A16_UINT).RV(DXGI_FORMAT_R16G16B16A16_UINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAUShortNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_UNORM).VA(DXGI_FORMAT_R16G16B16A16_UNORM).RV(DXGI_FORMAT_R16G16B16A16_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAShort, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_SINT).VA(DXGI_FORMAT_R16G16B16A16_SINT).RV(DXGI_FORMAT_R16G16B16A16_SINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAShortNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_SNORM).VA(DXGI_FORMAT_R16G16B16A16_SNORM).RV(DXGI_FORMAT_R16G16B16A16_SNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGFloat, plGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_FLOAT).VA(DXGI_FORMAT_R32G32_FLOAT).RV(DXGI_FORMAT_R32G32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGUInt, plGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_UINT).VA(DXGI_FORMAT_R32G32_UINT).RV(DXGI_FORMAT_R32G32_UINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGInt, plGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_SINT).VA(DXGI_FORMAT_R32G32_SINT).RV(DXGI_FORMAT_R32G32_SINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGB10A2UInt, plGALFormatLookupEntryDX11(DXGI_FORMAT_R10G10B10A2_TYPELESS).RT(DXGI_FORMAT_R10G10B10A2_UINT).VA(DXGI_FORMAT_R10G10B10A2_UINT).RV(DXGI_FORMAT_R10G10B10A2_UINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGB10A2UIntNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_R10G10B10A2_TYPELESS).RT(DXGI_FORMAT_R10G10B10A2_UNORM).VA(DXGI_FORMAT_R10G10B10A2_UNORM).RV(DXGI_FORMAT_R10G10B10A2_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RG11B10Float, plGALFormatLookupEntryDX11(DXGI_FORMAT_R11G11B10_FLOAT).RT(DXGI_FORMAT_R11G11B10_FLOAT).VA(DXGI_FORMAT_R11G11B10_FLOAT).RV(DXGI_FORMAT_R11G11B10_FLOAT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAUByteNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM).VA(DXGI_FORMAT_R8G8B8A8_UNORM).RV(DXGI_FORMAT_R8G8B8A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAUByteNormalizedsRGB, plGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB).RV(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAUByte, plGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UINT).VA(DXGI_FORMAT_R8G8B8A8_UINT).RV(DXGI_FORMAT_R8G8B8A8_UINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAByteNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_SNORM).VA(DXGI_FORMAT_R8G8B8A8_SNORM).RV(DXGI_FORMAT_R8G8B8A8_SNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAByte, plGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_SINT).VA(DXGI_FORMAT_R8G8B8A8_SINT).RV(DXGI_FORMAT_R8G8B8A8_SINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGHalf, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_FLOAT).VA(DXGI_FORMAT_R16G16_FLOAT).RV(DXGI_FORMAT_R16G16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGUShort, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_UINT).VA(DXGI_FORMAT_R16G16_UINT).RV(DXGI_FORMAT_R16G16_UINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGUShortNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_UNORM).VA(DXGI_FORMAT_R16G16_UNORM).RV(DXGI_FORMAT_R16G16_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGShort, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_SINT).VA(DXGI_FORMAT_R16G16_SINT).RV(DXGI_FORMAT_R16G16_SINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGShortNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_SNORM).VA(DXGI_FORMAT_R16G16_SNORM).RV(DXGI_FORMAT_R16G16_SNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGUByte, plGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_UINT).VA(DXGI_FORMAT_R8G8_UINT).RV(DXGI_FORMAT_R8G8_UINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGUByteNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_UNORM).VA(DXGI_FORMAT_R8G8_UNORM).RV(DXGI_FORMAT_R8G8_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGByte, plGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_SINT).VA(DXGI_FORMAT_R8G8_SINT).RV(DXGI_FORMAT_R8G8_SINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGByteNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_SNORM).VA(DXGI_FORMAT_R8G8_SNORM).RV(DXGI_FORMAT_R8G8_SNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::DFloat, plGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RV(DXGI_FORMAT_R32_FLOAT).D(DXGI_FORMAT_R32_FLOAT).DS(DXGI_FORMAT_D32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RFloat, plGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_FLOAT).VA(DXGI_FORMAT_R32_FLOAT).RV(DXGI_FORMAT_R32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RUInt, plGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_UINT).VA(DXGI_FORMAT_R32_UINT).RV(DXGI_FORMAT_R32_UINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RInt, plGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_SINT).VA(DXGI_FORMAT_R32_SINT).RV(DXGI_FORMAT_R32_SINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RHalf, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_FLOAT).VA(DXGI_FORMAT_R16_FLOAT).RV(DXGI_FORMAT_R16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RUShort, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_UINT).VA(DXGI_FORMAT_R16_UINT).RV(DXGI_FORMAT_R16_UINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RUShortNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_UNORM).VA(DXGI_FORMAT_R16_UNORM).RV(DXGI_FORMAT_R16_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RShort, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_SINT).VA(DXGI_FORMAT_R16_SINT).RV(DXGI_FORMAT_R16_SINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RShortNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_SNORM).VA(DXGI_FORMAT_R16_SNORM).RV(DXGI_FORMAT_R16_SNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RUByte, plGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_UINT).VA(DXGI_FORMAT_R8_UINT).RV(DXGI_FORMAT_R8_UINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RUByteNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_UNORM).VA(DXGI_FORMAT_R8_UNORM).RV(DXGI_FORMAT_R8_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RByte, plGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_SINT).VA(DXGI_FORMAT_R8_SINT).RV(DXGI_FORMAT_R8_SINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RByteNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_SNORM).VA(DXGI_FORMAT_R8_SNORM).RV(DXGI_FORMAT_R8_SNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::AUByteNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_A8_UNORM).RT(DXGI_FORMAT_A8_UNORM).VA(DXGI_FORMAT_A8_UNORM).RV(DXGI_FORMAT_A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::D16, plGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RV(DXGI_FORMAT_R16_UNORM).DS(DXGI_FORMAT_D16_UNORM).D(DXGI_FORMAT_R16_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::D24S8, plGALFormatLookupEntryDX11(DXGI_FORMAT_R24G8_TYPELESS).DS(DXGI_FORMAT_D24_UNORM_S8_UINT).D(DXGI_FORMAT_R24_UNORM_X8_TYPELESS).S(DXGI_FORMAT_X24_TYPELESS_G8_UINT));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC1, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC1_TYPELESS).RV(DXGI_FORMAT_BC1_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC1sRGB, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC1_TYPELESS).RV(DXGI_FORMAT_BC1_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC2, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC2_TYPELESS).RV(DXGI_FORMAT_BC2_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC2sRGB, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC2_TYPELESS).RV(DXGI_FORMAT_BC2_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC3, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC3_TYPELESS).RV(DXGI_FORMAT_BC3_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC3sRGB, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC3_TYPELESS).RV(DXGI_FORMAT_BC3_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC4UNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC4_TYPELESS).RV(DXGI_FORMAT_BC4_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC4Normalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC4_TYPELESS).RV(DXGI_FORMAT_BC4_SNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC5UNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC5_TYPELESS).RV(DXGI_FORMAT_BC5_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC5Normalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC5_TYPELESS).RV(DXGI_FORMAT_BC5_SNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC6UFloat, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC6H_TYPELESS).RV(DXGI_FORMAT_BC6H_UF16));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC6Float, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC6H_TYPELESS).RV(DXGI_FORMAT_BC6H_SF16));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC7UNormalized, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC7_TYPELESS).RV(DXGI_FORMAT_BC7_UNORM));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC7UNormalizedsRGB, plGALFormatLookupEntryDX11(DXGI_FORMAT_BC7_TYPELESS).RV(DXGI_FORMAT_BC7_UNORM_SRGB));
}

plGALRenderCommandEncoder* plGALDeviceDX11::GetRenderCommandEncoder() const
{
  return m_pDefaultPass->m_pRenderCommandEncoder.Borrow();
}

void plGALDeviceDX11::InsertFencePlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence)
{
  pContext->End(pFence);
}

bool plGALDeviceDX11::IsFenceReachedPlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence)
{
  BOOL data = FALSE;
  if (pContext->GetData(pFence, &data, sizeof(data), 0) == S_OK)
  {
    PLASMA_ASSERT_DEV(data != FALSE, "Implementation error");
    return true;
  }

  return false;
}

void plGALDeviceDX11::WaitForFencePlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence)
{
  BOOL data = FALSE;
  while (pContext->GetData(pFence, &data, sizeof(data), 0) != S_OK)
  {
    plThreadUtils::YieldTimeSlice();
  }

  PLASMA_ASSERT_DEV(data != FALSE, "Implementation error");
}

PLASMA_STATICLINK_FILE(RendererDX11, RendererDX11_Device_Implementation_DeviceDX11);
