#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SharedTextureSwapChain.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ProxyTexture.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererFoundation/State/State.h>

namespace
{
  struct GALObjectType
  {
    enum Enum
    {
      BlendState,
      DepthStencilState,
      RasterizerState,
      SamplerState,
      Shader,
      Buffer,
      Texture,
      ResourceView,
      RenderTargetView,
      UnorderedAccessView,
      SwapChain,
      Query,
      VertexDeclaration
    };
  };

  PLASMA_CHECK_AT_COMPILETIME(sizeof(plGALBlendStateHandle) == sizeof(plUInt32));
  PLASMA_CHECK_AT_COMPILETIME(sizeof(plGALDepthStencilStateHandle) == sizeof(plUInt32));
  PLASMA_CHECK_AT_COMPILETIME(sizeof(plGALRasterizerStateHandle) == sizeof(plUInt32));
  PLASMA_CHECK_AT_COMPILETIME(sizeof(plGALSamplerStateHandle) == sizeof(plUInt32));
  PLASMA_CHECK_AT_COMPILETIME(sizeof(plGALShaderHandle) == sizeof(plUInt32));
  PLASMA_CHECK_AT_COMPILETIME(sizeof(plGALBufferHandle) == sizeof(plUInt32));
  PLASMA_CHECK_AT_COMPILETIME(sizeof(plGALTextureHandle) == sizeof(plUInt32));
  PLASMA_CHECK_AT_COMPILETIME(sizeof(plGALResourceViewHandle) == sizeof(plUInt32));
  PLASMA_CHECK_AT_COMPILETIME(sizeof(plGALRenderTargetViewHandle) == sizeof(plUInt32));
  PLASMA_CHECK_AT_COMPILETIME(sizeof(plGALUnorderedAccessViewHandle) == sizeof(plUInt32));
  PLASMA_CHECK_AT_COMPILETIME(sizeof(plGALSwapChainHandle) == sizeof(plUInt32));
  PLASMA_CHECK_AT_COMPILETIME(sizeof(plGALQueryHandle) == sizeof(plUInt32));
  PLASMA_CHECK_AT_COMPILETIME(sizeof(plGALVertexDeclarationHandle) == sizeof(plUInt32));
} // namespace

plGALDevice* plGALDevice::s_pDefaultDevice = nullptr;


plGALDevice::plGALDevice(const plGALDeviceCreationDescription& desc)
  : m_Allocator("GALDevice", plFoundation::GetDefaultAllocator())
  , m_AllocatorWrapper(&m_Allocator)
  , m_Description(desc)
{
}

plGALDevice::~plGALDevice()
{
  // Check for object leaks
  {
    PLASMA_LOG_BLOCK("plGALDevice object leak report");

    if (!m_Shaders.IsEmpty())
      plLog::Warning("{0} shaders have not been cleaned up", m_Shaders.GetCount());

    if (!m_BlendStates.IsEmpty())
      plLog::Warning("{0} blend states have not been cleaned up", m_BlendStates.GetCount());

    if (!m_DepthStencilStates.IsEmpty())
      plLog::Warning("{0} depth stencil states have not been cleaned up", m_DepthStencilStates.GetCount());

    if (!m_RasterizerStates.IsEmpty())
      plLog::Warning("{0} rasterizer states have not been cleaned up", m_RasterizerStates.GetCount());

    if (!m_Buffers.IsEmpty())
      plLog::Warning("{0} buffers have not been cleaned up", m_Buffers.GetCount());

    if (!m_Textures.IsEmpty())
      plLog::Warning("{0} textures have not been cleaned up", m_Textures.GetCount());

    if (!m_ResourceViews.IsEmpty())
      plLog::Warning("{0} resource views have not been cleaned up", m_ResourceViews.GetCount());

    if (!m_RenderTargetViews.IsEmpty())
      plLog::Warning("{0} render target views have not been cleaned up", m_RenderTargetViews.GetCount());

    if (!m_UnorderedAccessViews.IsEmpty())
      plLog::Warning("{0} unordered access views have not been cleaned up", m_UnorderedAccessViews.GetCount());

    if (!m_SwapChains.IsEmpty())
      plLog::Warning("{0} swap chains have not been cleaned up", m_SwapChains.GetCount());

    if (!m_Queries.IsEmpty())
      plLog::Warning("{0} queries have not been cleaned up", m_Queries.GetCount());

    if (!m_VertexDeclarations.IsEmpty())
      plLog::Warning("{0} vertex declarations have not been cleaned up", m_VertexDeclarations.GetCount());
  }
}

plResult plGALDevice::Init()
{
  PLASMA_LOG_BLOCK("plGALDevice::Init");

  plResult PlatformInitResult = InitPlatform();

  if (PlatformInitResult == PLASMA_FAILURE)
  {
    return PLASMA_FAILURE;
  }

  plGALSharedTextureSwapChain::SetFactoryMethod([this](const plGALSharedTextureSwapChainCreationDescription& desc) -> plGALSwapChainHandle { return CreateSwapChain([this, &desc](plAllocatorBase* pAllocator) -> plGALSwapChain* { return PLASMA_NEW(pAllocator, plGALSharedTextureSwapChain, desc); }); });

  // Fill the capabilities
  FillCapabilitiesPlatform();

  plLog::Info("Adapter: '{}' - {} VRAM, {} Sys RAM, {} Shared RAM", m_Capabilities.m_sAdapterName, plArgFileSize(m_Capabilities.m_uiDedicatedVRAM),
    plArgFileSize(m_Capabilities.m_uiDedicatedSystemRAM), plArgFileSize(m_Capabilities.m_uiSharedSystemRAM));

  if (!m_Capabilities.m_bHardwareAccelerated)
  {
    plLog::Warning("Selected graphics adapter has no hardware acceleration.");
  }

  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plProfilingSystem::InitializeGPUData();

  {
    plGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = plGALDeviceEvent::AfterInit;
    m_Events.Broadcast(e);
  }

  return PLASMA_SUCCESS;
}

plResult plGALDevice::Shutdown()
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  PLASMA_LOG_BLOCK("plGALDevice::Shutdown");

  {
    plGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = plGALDeviceEvent::BeforeShutdown;
    m_Events.Broadcast(e);
  }

  DestroyDeadObjects();

  // make sure we are not listed as the default device anymore
  if (plGALDevice::HasDefaultDevice() && plGALDevice::GetDefaultDevice() == this)
  {
    plGALDevice::SetDefaultDevice(nullptr);
  }

  return ShutdownPlatform();
}

void plGALDevice::BeginPipeline(const char* szName, plGALSwapChainHandle hSwapChain)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  PLASMA_ASSERT_DEV(!m_bBeginPipelineCalled, "Nested Pipelines are not allowed: You must call plGALDevice::EndPipeline before you can call plGALDevice::BeginPipeline again");
  m_bBeginPipelineCalled = true;

  plGALSwapChain* pSwapChain = nullptr;
  m_SwapChains.TryGetValue(hSwapChain, pSwapChain);
  BeginPipelinePlatform(szName, pSwapChain);
}

void plGALDevice::EndPipeline(plGALSwapChainHandle hSwapChain)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  PLASMA_ASSERT_DEV(m_bBeginPipelineCalled, "You must have called plGALDevice::BeginPipeline before you can call plGALDevice::EndPipeline");
  m_bBeginPipelineCalled = false;

  plGALSwapChain* pSwapChain = nullptr;
  m_SwapChains.TryGetValue(hSwapChain, pSwapChain);
  EndPipelinePlatform(pSwapChain);
}

plGALPass* plGALDevice::BeginPass(const char* szName)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  PLASMA_ASSERT_DEV(!m_bBeginPassCalled, "Nested Passes are not allowed: You must call plGALDevice::EndPass before you can call plGALDevice::BeginPass again");
  m_bBeginPassCalled = true;

  return BeginPassPlatform(szName);
}

void plGALDevice::EndPass(plGALPass* pPass)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  PLASMA_ASSERT_DEV(m_bBeginPassCalled, "You must have called plGALDevice::BeginPass before you can call plGALDevice::EndPass");
  m_bBeginPassCalled = false;

  EndPassPlatform(pPass);
}

plGALBlendStateHandle plGALDevice::CreateBlendState(const plGALBlendStateCreationDescription& desc)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  // Hash desc and return potential existing one (including inc. refcount)
  plUInt32 uiHash = desc.CalculateHash();

  {
    plGALBlendStateHandle hBlendState;
    if (m_BlendStateTable.TryGetValue(uiHash, hBlendState))
    {
      plGALBlendState* pBlendState = m_BlendStates[hBlendState];
      if (pBlendState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::BlendState, hBlendState);
      }

      pBlendState->AddRef();
      return hBlendState;
    }
  }

  plGALBlendState* pBlendState = CreateBlendStatePlatform(desc);

  if (pBlendState != nullptr)
  {
    PLASMA_ASSERT_DEBUG(pBlendState->GetDescription().CalculateHash() == uiHash, "BlendState hash doesn't match");

    pBlendState->AddRef();

    plGALBlendStateHandle hBlendState(m_BlendStates.Insert(pBlendState));
    m_BlendStateTable.Insert(uiHash, hBlendState);

    return hBlendState;
  }

  return plGALBlendStateHandle();
}

void plGALDevice::DestroyBlendState(plGALBlendStateHandle hBlendState)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALBlendState* pBlendState = nullptr;

  if (m_BlendStates.TryGetValue(hBlendState, pBlendState))
  {
    pBlendState->ReleaseRef();

    if (pBlendState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::BlendState, hBlendState);
    }
  }
  else
  {
    plLog::Warning("DestroyBlendState called on invalid handle (double free?)");
  }
}

plGALDepthStencilStateHandle plGALDevice::CreateDepthStencilState(const plGALDepthStencilStateCreationDescription& desc)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  // Hash desc and return potential existing one (including inc. refcount)
  plUInt32 uiHash = desc.CalculateHash();

  {
    plGALDepthStencilStateHandle hDepthStencilState;
    if (m_DepthStencilStateTable.TryGetValue(uiHash, hDepthStencilState))
    {
      plGALDepthStencilState* pDepthStencilState = m_DepthStencilStates[hDepthStencilState];
      if (pDepthStencilState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::DepthStencilState, hDepthStencilState);
      }

      pDepthStencilState->AddRef();
      return hDepthStencilState;
    }
  }

  plGALDepthStencilState* pDepthStencilState = CreateDepthStencilStatePlatform(desc);

  if (pDepthStencilState != nullptr)
  {
    PLASMA_ASSERT_DEBUG(pDepthStencilState->GetDescription().CalculateHash() == uiHash, "DepthStencilState hash doesn't match");

    pDepthStencilState->AddRef();

    plGALDepthStencilStateHandle hDepthStencilState(m_DepthStencilStates.Insert(pDepthStencilState));
    m_DepthStencilStateTable.Insert(uiHash, hDepthStencilState);

    return hDepthStencilState;
  }

  return plGALDepthStencilStateHandle();
}

void plGALDevice::DestroyDepthStencilState(plGALDepthStencilStateHandle hDepthStencilState)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALDepthStencilState* pDepthStencilState = nullptr;

  if (m_DepthStencilStates.TryGetValue(hDepthStencilState, pDepthStencilState))
  {
    pDepthStencilState->ReleaseRef();

    if (pDepthStencilState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::DepthStencilState, hDepthStencilState);
    }
  }
  else
  {
    plLog::Warning("DestroyDepthStencilState called on invalid handle (double free?)");
  }
}

plGALRasterizerStateHandle plGALDevice::CreateRasterizerState(const plGALRasterizerStateCreationDescription& desc)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  // Hash desc and return potential existing one (including inc. refcount)
  plUInt32 uiHash = desc.CalculateHash();

  {
    plGALRasterizerStateHandle hRasterizerState;
    if (m_RasterizerStateTable.TryGetValue(uiHash, hRasterizerState))
    {
      plGALRasterizerState* pRasterizerState = m_RasterizerStates[hRasterizerState];
      if (pRasterizerState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::RasterizerState, hRasterizerState);
      }

      pRasterizerState->AddRef();
      return hRasterizerState;
    }
  }

  plGALRasterizerState* pRasterizerState = CreateRasterizerStatePlatform(desc);

  if (pRasterizerState != nullptr)
  {
    PLASMA_ASSERT_DEBUG(pRasterizerState->GetDescription().CalculateHash() == uiHash, "RasterizerState hash doesn't match");

    pRasterizerState->AddRef();

    plGALRasterizerStateHandle hRasterizerState(m_RasterizerStates.Insert(pRasterizerState));
    m_RasterizerStateTable.Insert(uiHash, hRasterizerState);

    return hRasterizerState;
  }

  return plGALRasterizerStateHandle();
}

void plGALDevice::DestroyRasterizerState(plGALRasterizerStateHandle hRasterizerState)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALRasterizerState* pRasterizerState = nullptr;

  if (m_RasterizerStates.TryGetValue(hRasterizerState, pRasterizerState))
  {
    pRasterizerState->ReleaseRef();

    if (pRasterizerState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::RasterizerState, hRasterizerState);
    }
  }
  else
  {
    plLog::Warning("DestroyRasterizerState called on invalid handle (double free?)");
  }
}

plGALSamplerStateHandle plGALDevice::CreateSamplerState(const plGALSamplerStateCreationDescription& desc)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation

  // Hash desc and return potential existing one (including inc. refcount)
  plUInt32 uiHash = desc.CalculateHash();

  {
    plGALSamplerStateHandle hSamplerState;
    if (m_SamplerStateTable.TryGetValue(uiHash, hSamplerState))
    {
      plGALSamplerState* pSamplerState = m_SamplerStates[hSamplerState];
      if (pSamplerState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::SamplerState, hSamplerState);
      }

      pSamplerState->AddRef();
      return hSamplerState;
    }
  }

  plGALSamplerState* pSamplerState = CreateSamplerStatePlatform(desc);

  if (pSamplerState != nullptr)
  {
    PLASMA_ASSERT_DEBUG(pSamplerState->GetDescription().CalculateHash() == uiHash, "SamplerState hash doesn't match");

    pSamplerState->AddRef();

    plGALSamplerStateHandle hSamplerState(m_SamplerStates.Insert(pSamplerState));
    m_SamplerStateTable.Insert(uiHash, hSamplerState);

    return hSamplerState;
  }

  return plGALSamplerStateHandle();
}

void plGALDevice::DestroySamplerState(plGALSamplerStateHandle hSamplerState)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALSamplerState* pSamplerState = nullptr;

  if (m_SamplerStates.TryGetValue(hSamplerState, pSamplerState))
  {
    pSamplerState->ReleaseRef();

    if (pSamplerState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::SamplerState, hSamplerState);
    }
  }
  else
  {
    plLog::Warning("DestroySamplerState called on invalid handle (double free?)");
  }
}



plGALShaderHandle plGALDevice::CreateShader(const plGALShaderCreationDescription& desc)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  bool bHasByteCodes = false;

  for (plUInt32 uiStage = 0; uiStage < plGALShaderStage::ENUM_COUNT; uiStage++)
  {
    if (desc.HasByteCodeForStage((plGALShaderStage::Enum)uiStage))
    {
      bHasByteCodes = true;
      break;
    }
  }

  if (!bHasByteCodes)
  {
    plLog::Error("Can't create a shader which supplies no bytecodes at all!");
    return plGALShaderHandle();
  }

  plGALShader* pShader = CreateShaderPlatform(desc);

  if (pShader == nullptr)
  {
    return plGALShaderHandle();
  }
  else
  {
    return plGALShaderHandle(m_Shaders.Insert(pShader));
  }
}

void plGALDevice::DestroyShader(plGALShaderHandle hShader)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALShader* pShader = nullptr;

  if (m_Shaders.TryGetValue(hShader, pShader))
  {
    AddDeadObject(GALObjectType::Shader, hShader);
  }
  else
  {
    plLog::Warning("DestroyShader called on invalid handle (double free?)");
  }
}


plGALBufferHandle plGALDevice::CreateBuffer(const plGALBufferCreationDescription& desc, plArrayPtr<const plUInt8> initialData)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  if (desc.m_uiTotalSize == 0)
  {
    plLog::Error("Trying to create a buffer with size of 0 is not possible!");
    return plGALBufferHandle();
  }

  if (desc.m_ResourceAccess.IsImmutable())
  {
    if (initialData.IsEmpty())
    {
      plLog::Error("Trying to create an immutable buffer but not supplying initial data is not possible!");
      return plGALBufferHandle();
    }

    plUInt32 uiBufferSize = desc.m_uiTotalSize;
    if (uiBufferSize != initialData.GetCount())
    {
      plLog::Error("Trying to create a buffer with invalid initial data!");
      return plGALBufferHandle();
    }
  }

  /// \todo Platform independent validation (buffer type supported)

  plGALBuffer* pBuffer = CreateBufferPlatform(desc, initialData);

  return FinalizeBufferInternal(desc, pBuffer);
}

plGALBufferHandle plGALDevice::FinalizeBufferInternal(const plGALBufferCreationDescription& desc, plGALBuffer* pBuffer)
{
  if (pBuffer != nullptr)
  {
    plGALBufferHandle hBuffer(m_Buffers.Insert(pBuffer));

    // Create default resource view
    if (desc.m_bAllowShaderResourceView && desc.m_BufferType == plGALBufferType::Generic)
    {
      plGALResourceViewCreationDescription viewDesc;
      viewDesc.m_hBuffer = hBuffer;
      viewDesc.m_uiFirstElement = 0;
      viewDesc.m_uiNumElements = (desc.m_uiStructSize != 0) ? (desc.m_uiTotalSize / desc.m_uiStructSize) : desc.m_uiTotalSize;

      pBuffer->m_hDefaultResourceView = CreateResourceView(viewDesc);
    }

    return hBuffer;
  }

  return plGALBufferHandle();
}

void plGALDevice::DestroyBuffer(plGALBufferHandle hBuffer)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALBuffer* pBuffer = nullptr;

  if (m_Buffers.TryGetValue(hBuffer, pBuffer))
  {
    AddDeadObject(GALObjectType::Buffer, hBuffer);
  }
  else
  {
    plLog::Warning("DestroyBuffer called on invalid handle (double free?)");
  }
}

// Helper functions for buffers (for common, simple use cases)
plGALBufferHandle plGALDevice::CreateVertexBuffer(plUInt32 uiVertexSize, plUInt32 uiVertexCount, plArrayPtr<const plUInt8> initialData, bool bDataIsMutable /*= false */)
{
  plGALBufferCreationDescription desc;
  desc.m_uiStructSize = uiVertexSize;
  desc.m_uiTotalSize = uiVertexSize * uiVertexCount;
  desc.m_BufferType = plGALBufferType::VertexBuffer;
  desc.m_ResourceAccess.m_bImmutable = !initialData.IsEmpty() && !bDataIsMutable;

  return CreateBuffer(desc, initialData);
}

plGALBufferHandle plGALDevice::CreateIndexBuffer(plGALIndexType::Enum indexType, plUInt32 uiIndexCount, plArrayPtr<const plUInt8> initialData, bool bDataIsMutable /*= false*/)
{
  plGALBufferCreationDescription desc;
  desc.m_uiStructSize = plGALIndexType::GetSize(indexType);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiIndexCount;
  desc.m_BufferType = plGALBufferType::IndexBuffer;
  desc.m_ResourceAccess.m_bImmutable = !bDataIsMutable && !initialData.IsEmpty();

  return CreateBuffer(desc, initialData);
}

plGALBufferHandle plGALDevice::CreateConstantBuffer(plUInt32 uiBufferSize)
{
  plGALBufferCreationDescription desc;
  desc.m_uiStructSize = 0;
  desc.m_uiTotalSize = uiBufferSize;
  desc.m_BufferType = plGALBufferType::ConstantBuffer;
  desc.m_ResourceAccess.m_bImmutable = false;

  return CreateBuffer(desc);
}


plGALTextureHandle plGALDevice::CreateTexture(const plGALTextureCreationDescription& desc, plArrayPtr<plGALSystemMemoryDescription> initialData)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation (desc width & height < platform maximum, format, etc.)

  if (desc.m_ResourceAccess.IsImmutable() && (initialData.IsEmpty() || initialData.GetCount() < desc.m_uiMipLevelCount) &&
      !desc.m_bCreateRenderTarget)
  {
    plLog::Error("Trying to create an immutable texture but not supplying initial data (or not enough data pointers) is not possible!");
    return plGALTextureHandle();
  }

  if (desc.m_uiWidth == 0 || desc.m_uiHeight == 0)
  {
    plLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return plGALTextureHandle();
  }

  plGALTexture* pTexture = CreateTexturePlatform(desc, initialData);

  return FinalizeTextureInternal(desc, pTexture);
}

plGALTextureHandle plGALDevice::FinalizeTextureInternal(const plGALTextureCreationDescription& desc, plGALTexture* pTexture)
{
  if (pTexture != nullptr)
  {
    plGALTextureHandle hTexture(m_Textures.Insert(pTexture));

    // Create default resource view
    if (desc.m_bAllowShaderResourceView)
    {
      plGALResourceViewCreationDescription viewDesc;
      viewDesc.m_hTexture = hTexture;
      viewDesc.m_uiArraySize = desc.m_uiArraySize;
      pTexture->m_hDefaultResourceView = CreateResourceView(viewDesc);
    }

    // Create default render target view
    if (desc.m_bCreateRenderTarget)
    {
      plGALRenderTargetViewCreationDescription rtDesc;
      rtDesc.m_hTexture = hTexture;
      rtDesc.m_uiFirstSlice = 0;
      rtDesc.m_uiSliceCount = desc.m_uiArraySize;

      pTexture->m_hDefaultRenderTargetView = CreateRenderTargetView(rtDesc);
    }

    return hTexture;
  }

  return plGALTextureHandle();
}

void plGALDevice::DestroyTexture(plGALTextureHandle hTexture)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(hTexture, pTexture))
  {
    AddDeadObject(GALObjectType::Texture, hTexture);
  }
  else
  {
    plLog::Warning("DestroyTexture called on invalid handle (double free?)");
  }
}

plGALTextureHandle plGALDevice::CreateProxyTexture(plGALTextureHandle hParentTexture, plUInt32 uiSlice)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALTexture* pParentTexture = nullptr;

  if (!hParentTexture.IsInvalidated())
  {
    pParentTexture = Get<TextureTable, plGALTexture>(hParentTexture, m_Textures);
  }

  if (pParentTexture == nullptr)
  {
    plLog::Error("No valid texture handle given for proxy texture creation!");
    return plGALTextureHandle();
  }

  const auto& parentDesc = pParentTexture->GetDescription();
  PLASMA_ASSERT_DEV(parentDesc.m_Type != plGALTextureType::Texture2DProxy, "Can't create a proxy texture of a proxy texture.");
  PLASMA_ASSERT_DEV(parentDesc.m_Type == plGALTextureType::TextureCube || parentDesc.m_uiArraySize > 1,
    "Proxy textures can only be created for cubemaps or array textures.");

  plGALProxyTexture* pProxyTexture = PLASMA_NEW(&m_Allocator, plGALProxyTexture, *pParentTexture);
  plGALTextureHandle hProxyTexture(m_Textures.Insert(pProxyTexture));

  const auto& desc = pProxyTexture->GetDescription();

  // Create default resource view
  if (desc.m_bAllowShaderResourceView)
  {
    plGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture = hProxyTexture;
    viewDesc.m_uiFirstArraySlice = uiSlice;
    viewDesc.m_uiArraySize = 1;

    pProxyTexture->m_hDefaultResourceView = CreateResourceView(viewDesc);
  }

  // Create default render target view
  if (desc.m_bCreateRenderTarget)
  {
    plGALRenderTargetViewCreationDescription rtDesc;
    rtDesc.m_hTexture = hProxyTexture;
    rtDesc.m_uiFirstSlice = uiSlice;
    rtDesc.m_uiSliceCount = 1;

    pProxyTexture->m_hDefaultRenderTargetView = CreateRenderTargetView(rtDesc);
  }

  return hProxyTexture;
}

void plGALDevice::DestroyProxyTexture(plGALTextureHandle hProxyTexture)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(hProxyTexture, pTexture))
  {
    PLASMA_ASSERT_DEV(pTexture->GetDescription().m_Type == plGALTextureType::Texture2DProxy, "Given texture is not a proxy texture");

    AddDeadObject(GALObjectType::Texture, hProxyTexture);
  }
  else
  {
    plLog::Warning("DestroyProxyTexture called on invalid handle (double free?)");
  }
}

plGALTextureHandle plGALDevice::CreateSharedTexture(const plGALTextureCreationDescription& desc, plArrayPtr<plGALSystemMemoryDescription> initialData)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation (desc width & height < platform maximum, format, etc.)

  if (desc.m_ResourceAccess.IsImmutable() && (initialData.IsEmpty() || initialData.GetCount() < desc.m_uiMipLevelCount) &&
      !desc.m_bCreateRenderTarget)
  {
    plLog::Error("Trying to create an immutable texture but not supplying initial data (or not enough data pointers) is not possible!");
    return plGALTextureHandle();
  }

  if (desc.m_uiWidth == 0 || desc.m_uiHeight == 0)
  {
    plLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return plGALTextureHandle();
  }

  if (desc.m_pExisitingNativeObject != nullptr)
  {
    plLog::Error("Shared textures cannot be created on exiting native objects!");
    return plGALTextureHandle();
  }

  if (desc.m_Type != plGALTextureType::Texture2DShared)
  {
    plLog::Error("Only plGALTextureType::Texture2DShared is supported for shared textures!");
    return plGALTextureHandle();
  }

  plGALTexture* pTexture = CreateSharedTexturePlatform(desc, initialData, plGALSharedTextureType::Exported, {});

  return FinalizeTextureInternal(desc, pTexture);
}

plGALTextureHandle plGALDevice::OpenSharedTexture(const plGALTextureCreationDescription& desc, plGALPlatformSharedHandle hSharedHandle)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  if (desc.m_pExisitingNativeObject != nullptr)
  {
    plLog::Error("Shared textures cannot be created on exiting native objects!");
    return plGALTextureHandle();
  }

  if (desc.m_Type != plGALTextureType::Texture2DShared)
  {
    plLog::Error("Only plGALTextureType::Texture2DShared is supported for shared textures!");
    return plGALTextureHandle();
  }

  if (desc.m_uiWidth == 0 || desc.m_uiHeight == 0)
  {
    plLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return plGALTextureHandle();
  }

  plGALTexture* pTexture = CreateSharedTexturePlatform(desc, {}, plGALSharedTextureType::Imported, hSharedHandle);

  return FinalizeTextureInternal(desc, pTexture);
}

void plGALDevice::DestroySharedTexture(plGALTextureHandle hSharedTexture)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(hSharedTexture, pTexture))
  {
    PLASMA_ASSERT_DEV(pTexture->GetDescription().m_Type == plGALTextureType::Texture2DShared, "Given texture is not a shared texture texture");

    AddDeadObject(GALObjectType::Texture, hSharedTexture);
  }
  else
  {
    plLog::Warning("DestroySharedTexture called on invalid handle (double free?)");
  }
}

plGALResourceViewHandle plGALDevice::GetDefaultResourceView(plGALTextureHandle hTexture)
{
  if (const plGALTexture* pTexture = GetTexture(hTexture))
  {
    return pTexture->m_hDefaultResourceView;
  }

  return plGALResourceViewHandle();
}

plGALResourceViewHandle plGALDevice::GetDefaultResourceView(plGALBufferHandle hBuffer)
{
  if (const plGALBuffer* pBuffer = GetBuffer(hBuffer))
  {
    return pBuffer->m_hDefaultResourceView;
  }

  return plGALResourceViewHandle();
}

plGALResourceViewHandle plGALDevice::CreateResourceView(const plGALResourceViewCreationDescription& desc)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALResourceBase* pResource = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
    pResource = Get<TextureTable, plGALTexture>(desc.m_hTexture, m_Textures);

  if (!desc.m_hBuffer.IsInvalidated())
    pResource = Get<BufferTable, plGALBuffer>(desc.m_hBuffer, m_Buffers);

  if (pResource == nullptr)
  {
    plLog::Error("No valid texture handle or buffer handle given for resource view creation!");
    return plGALResourceViewHandle();
  }

  // Hash desc and return potential existing one
  plUInt32 uiHash = desc.CalculateHash();

  {
    plGALResourceViewHandle hResourceView;
    if (pResource->m_ResourceViews.TryGetValue(uiHash, hResourceView))
    {
      return hResourceView;
    }
  }

  plGALResourceView* pResourceView = CreateResourceViewPlatform(pResource, desc);

  if (pResourceView != nullptr)
  {
    plGALResourceViewHandle hResourceView(m_ResourceViews.Insert(pResourceView));
    pResource->m_ResourceViews.Insert(uiHash, hResourceView);

    return hResourceView;
  }

  return plGALResourceViewHandle();
}

void plGALDevice::DestroyResourceView(plGALResourceViewHandle hResourceView)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALResourceView* pResourceView = nullptr;

  if (m_ResourceViews.TryGetValue(hResourceView, pResourceView))
  {
    AddDeadObject(GALObjectType::ResourceView, hResourceView);
  }
  else
  {
    plLog::Warning("DestroyResourceView called on invalid handle (double free?)");
  }
}

plGALRenderTargetViewHandle plGALDevice::GetDefaultRenderTargetView(plGALTextureHandle hTexture)
{
  if (const plGALTexture* pTexture = GetTexture(hTexture))
  {
    return pTexture->m_hDefaultRenderTargetView;
  }

  return plGALRenderTargetViewHandle();
}

plGALRenderTargetViewHandle plGALDevice::CreateRenderTargetView(const plGALRenderTargetViewCreationDescription& desc)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALTexture* pTexture = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
    pTexture = Get<TextureTable, plGALTexture>(desc.m_hTexture, m_Textures);

  if (pTexture == nullptr)
  {
    plLog::Error("No valid texture handle given for render target view creation!");
    return plGALRenderTargetViewHandle();
  }

  /// \todo Platform independent validation

  // Hash desc and return potential existing one
  plUInt32 uiHash = desc.CalculateHash();

  {
    plGALRenderTargetViewHandle hRenderTargetView;
    if (pTexture->m_RenderTargetViews.TryGetValue(uiHash, hRenderTargetView))
    {
      return hRenderTargetView;
    }
  }

  plGALRenderTargetView* pRenderTargetView = CreateRenderTargetViewPlatform(pTexture, desc);

  if (pRenderTargetView != nullptr)
  {
    plGALRenderTargetViewHandle hRenderTargetView(m_RenderTargetViews.Insert(pRenderTargetView));
    pTexture->m_RenderTargetViews.Insert(uiHash, hRenderTargetView);

    return hRenderTargetView;
  }

  return plGALRenderTargetViewHandle();
}

void plGALDevice::DestroyRenderTargetView(plGALRenderTargetViewHandle hRenderTargetView)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALRenderTargetView* pRenderTargetView = nullptr;

  if (m_RenderTargetViews.TryGetValue(hRenderTargetView, pRenderTargetView))
  {
    AddDeadObject(GALObjectType::RenderTargetView, hRenderTargetView);
  }
  else
  {
    plLog::Warning("DestroyRenderTargetView called on invalid handle (double free?)");
  }
}

plGALUnorderedAccessViewHandle plGALDevice::CreateUnorderedAccessView(const plGALUnorderedAccessViewCreationDescription& desc)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  if (!desc.m_hTexture.IsInvalidated() && !desc.m_hBuffer.IsInvalidated())
  {
    plLog::Error("Can't pass both a texture and buffer to a plGALUnorderedAccessViewCreationDescription.");
    return plGALUnorderedAccessViewHandle();
  }

  plGALResourceBase* pResource = nullptr;
  plGALTexture* pTexture = nullptr;
  plGALBuffer* pBuffer = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
  {
    pResource = pTexture = Get<TextureTable, plGALTexture>(desc.m_hTexture, m_Textures);
  }
  else if (!desc.m_hBuffer.IsInvalidated())
  {
    pResource = pBuffer = Get<BufferTable, plGALBuffer>(desc.m_hBuffer, m_Buffers);
  }

  if (pResource == nullptr)
  {
    plLog::Error("No valid texture handle or buffer handle given for unordered access view creation!");
    return plGALUnorderedAccessViewHandle();
  }

  // Some platform independent validation.
  {
    if (pTexture)
    {
      // Is this really platform independent?
      if (pTexture->GetDescription().m_SampleCount != plGALMSAASampleCount::None)
      {
        plLog::Error("Can't create unordered access view on textures with multisampling.");
        return plGALUnorderedAccessViewHandle();
      }
    }
    else
    {
      if (desc.m_OverrideViewFormat == plGALResourceFormat::Invalid)
      {
        plLog::Error("Invalid resource format is not allowed for buffer unordered access views!");
        return plGALUnorderedAccessViewHandle();
      }

      if (!pBuffer->GetDescription().m_bAllowRawViews && desc.m_bRawView)
      {
        plLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
        return plGALUnorderedAccessViewHandle();
      }
    }
  }

  // Hash desc and return potential existing one
  plUInt32 uiHash = desc.CalculateHash();

  {
    plGALUnorderedAccessViewHandle hUnorderedAccessView;
    if (pResource->m_UnorderedAccessViews.TryGetValue(uiHash, hUnorderedAccessView))
    {
      return hUnorderedAccessView;
    }
  }

  plGALUnorderedAccessView* pUnorderedAccessViewView = CreateUnorderedAccessViewPlatform(pResource, desc);

  if (pUnorderedAccessViewView != nullptr)
  {
    plGALUnorderedAccessViewHandle hUnorderedAccessView(m_UnorderedAccessViews.Insert(pUnorderedAccessViewView));
    pResource->m_UnorderedAccessViews.Insert(uiHash, hUnorderedAccessView);

    return hUnorderedAccessView;
  }

  return plGALUnorderedAccessViewHandle();
}

void plGALDevice::DestroyUnorderedAccessView(plGALUnorderedAccessViewHandle hUnorderedAccessViewHandle)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALUnorderedAccessView* pUnorderedAccesssView = nullptr;

  if (m_UnorderedAccessViews.TryGetValue(hUnorderedAccessViewHandle, pUnorderedAccesssView))
  {
    AddDeadObject(GALObjectType::UnorderedAccessView, hUnorderedAccessViewHandle);
  }
  else
  {
    plLog::Warning("DestroyUnorderedAccessView called on invalid handle (double free?)");
  }
}

plGALSwapChainHandle plGALDevice::CreateSwapChain(const SwapChainFactoryFunction& func)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  ///// \todo Platform independent validation
  //if (desc.m_pWindow == nullptr)
  //{
  //  plLog::Error("The desc for the swap chain creation contained an invalid (nullptr) window handle!");
  //  return plGALSwapChainHandle();
  //}

  plGALSwapChain* pSwapChain = func(&m_Allocator);
  //plGALSwapChainDX11* pSwapChain = PLASMA_NEW(&m_Allocator, plGALSwapChainDX11, Description);

  if (!pSwapChain->InitPlatform(this).Succeeded())
  {
    PLASMA_DELETE(&m_Allocator, pSwapChain);
    return plGALSwapChainHandle();
  }

  return plGALSwapChainHandle(m_SwapChains.Insert(pSwapChain));
}

plResult plGALDevice::UpdateSwapChain(plGALSwapChainHandle hSwapChain, plEnum<plGALPresentMode> newPresentMode)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    return pSwapChain->UpdateSwapChain(this, newPresentMode);
  }
  else
  {
    plLog::Warning("UpdateSwapChain called on invalid handle.");
    return PLASMA_FAILURE;
  }
}

void plGALDevice::DestroySwapChain(plGALSwapChainHandle hSwapChain)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    AddDeadObject(GALObjectType::SwapChain, hSwapChain);
  }
  else
  {
    plLog::Warning("DestroySwapChain called on invalid handle (double free?)");
  }
}

plGALQueryHandle plGALDevice::CreateQuery(const plGALQueryCreationDescription& desc)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALQuery* pQuery = CreateQueryPlatform(desc);

  if (pQuery == nullptr)
  {
    return plGALQueryHandle();
  }
  else
  {
    return plGALQueryHandle(m_Queries.Insert(pQuery));
  }
}

void plGALDevice::DestroyQuery(plGALQueryHandle hQuery)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALQuery* pQuery = nullptr;

  if (m_Queries.TryGetValue(hQuery, pQuery))
  {
    AddDeadObject(GALObjectType::Query, hQuery);
  }
  else
  {
    plLog::Warning("DestroyQuery called on invalid handle (double free?)");
  }
}

plGALVertexDeclarationHandle plGALDevice::CreateVertexDeclaration(const plGALVertexDeclarationCreationDescription& desc)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation

  // Hash desc and return potential existing one (including inc. refcount)
  plUInt32 uiHash = desc.CalculateHash();

  {
    plGALVertexDeclarationHandle hVertexDeclaration;
    if (m_VertexDeclarationTable.TryGetValue(uiHash, hVertexDeclaration))
    {
      plGALVertexDeclaration* pVertexDeclaration = m_VertexDeclarations[hVertexDeclaration];
      if (pVertexDeclaration->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::VertexDeclaration, hVertexDeclaration);
      }

      pVertexDeclaration->AddRef();
      return hVertexDeclaration;
    }
  }

  plGALVertexDeclaration* pVertexDeclaration = CreateVertexDeclarationPlatform(desc);

  if (pVertexDeclaration != nullptr)
  {
    pVertexDeclaration->AddRef();

    plGALVertexDeclarationHandle hVertexDeclaration(m_VertexDeclarations.Insert(pVertexDeclaration));
    m_VertexDeclarationTable.Insert(uiHash, hVertexDeclaration);

    return hVertexDeclaration;
  }

  return plGALVertexDeclarationHandle();
}

void plGALDevice::DestroyVertexDeclaration(plGALVertexDeclarationHandle hVertexDeclaration)
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  plGALVertexDeclaration* pVertexDeclaration = nullptr;

  if (m_VertexDeclarations.TryGetValue(hVertexDeclaration, pVertexDeclaration))
  {
    pVertexDeclaration->ReleaseRef();

    if (pVertexDeclaration->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::VertexDeclaration, hVertexDeclaration);
    }
  }
  else
  {
    plLog::Warning("DestroyVertexDeclaration called on invalid handle (double free?)");
  }
}

plGALTextureHandle plGALDevice::GetBackBufferTextureFromSwapChain(plGALSwapChainHandle hSwapChain)
{
  plGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    return pSwapChain->GetBackBufferTexture();
  }
  else
  {
    PLASMA_REPORT_FAILURE("Swap chain handle invalid");
    return plGALTextureHandle();
  }
}



// Misc functions

void plGALDevice::BeginFrame(const plUInt64 uiRenderFrame)
{
  {
    PLASMA_PROFILE_SCOPE("BeforeBeginFrame");
    plGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = plGALDeviceEvent::BeforeBeginFrame;
    m_Events.Broadcast(e);
  }

  {
    PLASMA_GALDEVICE_LOCK_AND_CHECK();
    PLASMA_ASSERT_DEV(!m_bBeginFrameCalled, "You must call plGALDevice::EndFrame before you can call plGALDevice::BeginFrame again");
    m_bBeginFrameCalled = true;

    BeginFramePlatform(uiRenderFrame);
  }

  // TODO: move to beginrendering/compute calls
  //m_pPrimaryContext->ClearStatisticsCounters();

  {
    plGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = plGALDeviceEvent::AfterBeginFrame;
    m_Events.Broadcast(e);
  }
}

void plGALDevice::EndFrame()
{
  {
    plGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = plGALDeviceEvent::BeforeEndFrame;
    m_Events.Broadcast(e);
  }

  {
    PLASMA_GALDEVICE_LOCK_AND_CHECK();
    PLASMA_ASSERT_DEV(m_bBeginFrameCalled, "You must have called plGALDevice::Begin before you can call plGALDevice::EndFrame");

    DestroyDeadObjects();

    EndFramePlatform();

    m_bBeginFrameCalled = false;
  }

  {
    plGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = plGALDeviceEvent::AfterEndFrame;
    m_Events.Broadcast(e);
  }
}

const plGALDeviceCapabilities& plGALDevice::GetCapabilities() const
{
  return m_Capabilities;
}

plUInt64 plGALDevice::GetMemoryConsumptionForTexture(const plGALTextureCreationDescription& desc) const
{
  // This generic implementation is only an approximation, but it can be overridden by specific devices
  // to give an accurate memory consumption figure.
  plUInt64 uiMemory = plUInt64(desc.m_uiWidth) * plUInt64(desc.m_uiHeight) * plUInt64(desc.m_uiDepth);
  uiMemory *= desc.m_uiArraySize;
  uiMemory *= plGALResourceFormat::GetBitsPerElement(desc.m_Format);
  uiMemory /= 8; // Bits per pixel
  uiMemory *= desc.m_SampleCount;

  // Also account for mip maps
  if (desc.m_uiMipLevelCount > 1)
  {
    uiMemory += static_cast<plUInt64>((1.0 / 3.0) * uiMemory);
  }

  return uiMemory;
}


plUInt64 plGALDevice::GetMemoryConsumptionForBuffer(const plGALBufferCreationDescription& desc) const
{
  return desc.m_uiTotalSize;
}

void plGALDevice::Flush()
{
  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  FlushPlatform();
}

void plGALDevice::WaitIdle()
{
  WaitIdlePlatform();
}

void plGALDevice::DestroyViews(plGALResourceBase* pResource)
{
  PLASMA_ASSERT_DEBUG(pResource != nullptr, "Must provide valid resource");

  PLASMA_GALDEVICE_LOCK_AND_CHECK();

  for (auto it = pResource->m_ResourceViews.GetIterator(); it.IsValid(); ++it)
  {
    plGALResourceViewHandle hResourceView = it.Value();
    plGALResourceView* pResourceView = m_ResourceViews[hResourceView];

    m_ResourceViews.Remove(hResourceView);

    DestroyResourceViewPlatform(pResourceView);
  }
  pResource->m_ResourceViews.Clear();
  pResource->m_hDefaultResourceView.Invalidate();

  for (auto it = pResource->m_RenderTargetViews.GetIterator(); it.IsValid(); ++it)
  {
    plGALRenderTargetViewHandle hRenderTargetView = it.Value();
    plGALRenderTargetView* pRenderTargetView = m_RenderTargetViews[hRenderTargetView];

    m_RenderTargetViews.Remove(hRenderTargetView);

    DestroyRenderTargetViewPlatform(pRenderTargetView);
  }
  pResource->m_RenderTargetViews.Clear();
  pResource->m_hDefaultRenderTargetView.Invalidate();

  for (auto it = pResource->m_UnorderedAccessViews.GetIterator(); it.IsValid(); ++it)
  {
    plGALUnorderedAccessViewHandle hUnorderedAccessView = it.Value();
    plGALUnorderedAccessView* pUnorderedAccessView = m_UnorderedAccessViews[hUnorderedAccessView];

    m_UnorderedAccessViews.Remove(hUnorderedAccessView);

    DestroyUnorderedAccessViewPlatform(pUnorderedAccessView);
  }
  pResource->m_UnorderedAccessViews.Clear();
}

void plGALDevice::DestroyDeadObjects()
{
  // Can't use range based for here since new objects might be added during iteration
  for (plUInt32 i = 0; i < m_DeadObjects.GetCount(); ++i)
  {
    const auto& deadObject = m_DeadObjects[i];

    switch (deadObject.m_uiType)
    {
      case GALObjectType::BlendState:
      {
        plGALBlendStateHandle hBlendState(plGAL::pl16_16Id(deadObject.m_uiHandle));
        plGALBlendState* pBlendState = nullptr;

        PLASMA_VERIFY(m_BlendStates.Remove(hBlendState, &pBlendState), "BlendState not found in idTable");
        PLASMA_VERIFY(m_BlendStateTable.Remove(pBlendState->GetDescription().CalculateHash()), "BlendState not found in de-duplication table");

        DestroyBlendStatePlatform(pBlendState);

        break;
      }
      case GALObjectType::DepthStencilState:
      {
        plGALDepthStencilStateHandle hDepthStencilState(plGAL::pl16_16Id(deadObject.m_uiHandle));
        plGALDepthStencilState* pDepthStencilState = nullptr;

        PLASMA_VERIFY(m_DepthStencilStates.Remove(hDepthStencilState, &pDepthStencilState), "DepthStencilState not found in idTable");
        PLASMA_VERIFY(m_DepthStencilStateTable.Remove(pDepthStencilState->GetDescription().CalculateHash()),
          "DepthStencilState not found in de-duplication table");

        DestroyDepthStencilStatePlatform(pDepthStencilState);

        break;
      }
      case GALObjectType::RasterizerState:
      {
        plGALRasterizerStateHandle hRasterizerState(plGAL::pl16_16Id(deadObject.m_uiHandle));
        plGALRasterizerState* pRasterizerState = nullptr;

        PLASMA_VERIFY(m_RasterizerStates.Remove(hRasterizerState, &pRasterizerState), "RasterizerState not found in idTable");
        PLASMA_VERIFY(
          m_RasterizerStateTable.Remove(pRasterizerState->GetDescription().CalculateHash()), "RasterizerState not found in de-duplication table");

        DestroyRasterizerStatePlatform(pRasterizerState);

        break;
      }
      case GALObjectType::SamplerState:
      {
        plGALSamplerStateHandle hSamplerState(plGAL::pl16_16Id(deadObject.m_uiHandle));
        plGALSamplerState* pSamplerState = nullptr;

        PLASMA_VERIFY(m_SamplerStates.Remove(hSamplerState, &pSamplerState), "SamplerState not found in idTable");
        PLASMA_VERIFY(m_SamplerStateTable.Remove(pSamplerState->GetDescription().CalculateHash()), "SamplerState not found in de-duplication table");

        DestroySamplerStatePlatform(pSamplerState);

        break;
      }
      case GALObjectType::Shader:
      {
        plGALShaderHandle hShader(plGAL::pl18_14Id(deadObject.m_uiHandle));
        plGALShader* pShader = nullptr;

        m_Shaders.Remove(hShader, &pShader);

        DestroyShaderPlatform(pShader);

        break;
      }
      case GALObjectType::Buffer:
      {
        plGALBufferHandle hBuffer(plGAL::pl18_14Id(deadObject.m_uiHandle));
        plGALBuffer* pBuffer = nullptr;

        m_Buffers.Remove(hBuffer, &pBuffer);

        DestroyViews(pBuffer);
        DestroyBufferPlatform(pBuffer);

        break;
      }
      case GALObjectType::Texture:
      {
        plGALTextureHandle hTexture(plGAL::pl18_14Id(deadObject.m_uiHandle));
        plGALTexture* pTexture = nullptr;

        PLASMA_VERIFY(m_Textures.Remove(hTexture, &pTexture), "Unexpected invalild texture handle");

        DestroyViews(pTexture);

        switch (pTexture->GetDescription().m_Type)
        {
          case plGALTextureType::Texture2DShared:
            DestroySharedTexturePlatform(pTexture);
            break;
          default:
            DestroyTexturePlatform(pTexture);
            break;
        }
        break;
      }
      case GALObjectType::ResourceView:
      {
        plGALResourceViewHandle hResourceView(plGAL::pl18_14Id(deadObject.m_uiHandle));
        plGALResourceView* pResourceView = nullptr;

        m_ResourceViews.Remove(hResourceView, &pResourceView);

        plGALResourceBase* pResource = pResourceView->m_pResource;
        PLASMA_ASSERT_DEBUG(pResource != nullptr, "");

        PLASMA_VERIFY(pResource->m_ResourceViews.Remove(pResourceView->GetDescription().CalculateHash()), "");
        pResourceView->m_pResource = nullptr;

        DestroyResourceViewPlatform(pResourceView);

        break;
      }
      case GALObjectType::RenderTargetView:
      {
        plGALRenderTargetViewHandle hRenderTargetView(plGAL::pl18_14Id(deadObject.m_uiHandle));
        plGALRenderTargetView* pRenderTargetView = nullptr;

        m_RenderTargetViews.Remove(hRenderTargetView, &pRenderTargetView);

        plGALTexture* pTexture = pRenderTargetView->m_pTexture;
        PLASMA_ASSERT_DEBUG(pTexture != nullptr, "");
        PLASMA_VERIFY(pTexture->m_RenderTargetViews.Remove(pRenderTargetView->GetDescription().CalculateHash()), "");
        pRenderTargetView->m_pTexture = nullptr;

        DestroyRenderTargetViewPlatform(pRenderTargetView);

        break;
      }
      case GALObjectType::UnorderedAccessView:
      {
        plGALUnorderedAccessViewHandle hUnorderedAccessViewHandle(plGAL::pl18_14Id(deadObject.m_uiHandle));
        plGALUnorderedAccessView* pUnorderedAccesssView = nullptr;

        m_UnorderedAccessViews.Remove(hUnorderedAccessViewHandle, &pUnorderedAccesssView);

        plGALResourceBase* pResource = pUnorderedAccesssView->m_pResource;
        PLASMA_ASSERT_DEBUG(pResource != nullptr, "");

        PLASMA_VERIFY(pResource->m_UnorderedAccessViews.Remove(pUnorderedAccesssView->GetDescription().CalculateHash()), "");
        pUnorderedAccesssView->m_pResource = nullptr;

        DestroyUnorderedAccessViewPlatform(pUnorderedAccesssView);

        break;
      }
      case GALObjectType::SwapChain:
      {
        plGALSwapChainHandle hSwapChain(plGAL::pl16_16Id(deadObject.m_uiHandle));
        plGALSwapChain* pSwapChain = nullptr;

        m_SwapChains.Remove(hSwapChain, &pSwapChain);

        if (pSwapChain != nullptr)
        {
          pSwapChain->DeInitPlatform(this).IgnoreResult();
          PLASMA_DELETE(&m_Allocator, pSwapChain);
        }

        break;
      }
      case GALObjectType::Query:
      {
        plGALQueryHandle hQuery(plGAL::pl20_12Id(deadObject.m_uiHandle));
        plGALQuery* pQuery = nullptr;

        m_Queries.Remove(hQuery, &pQuery);

        DestroyQueryPlatform(pQuery);

        break;
      }
      case GALObjectType::VertexDeclaration:
      {
        plGALVertexDeclarationHandle hVertexDeclaration(plGAL::pl18_14Id(deadObject.m_uiHandle));
        plGALVertexDeclaration* pVertexDeclaration = nullptr;

        PLASMA_VERIFY(m_VertexDeclarations.Remove(hVertexDeclaration, &pVertexDeclaration), "Unexpected invalid handle");
        m_VertexDeclarationTable.Remove(pVertexDeclaration->GetDescription().CalculateHash());

        DestroyVertexDeclarationPlatform(pVertexDeclaration);

        break;
      }
      default:
        PLASMA_ASSERT_NOT_IMPLEMENTED;
    }
  }

  m_DeadObjects.Clear();
}

const plGALSwapChain* plGALDevice::GetSwapChainInternal(plGALSwapChainHandle hSwapChain, const plRTTI* pRequestedType) const
{
  const plGALSwapChain* pSwapChain = GetSwapChain(hSwapChain);
  if (pSwapChain)
  {
    if (!pSwapChain->GetDescription().m_pSwapChainType->IsDerivedFrom(pRequestedType))
      return nullptr;
  }
  return pSwapChain;
}

PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_Device);
