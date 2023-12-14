#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

plRenderContext* plRenderContext::s_pDefaultInstance = nullptr;
plHybridArray<plRenderContext*, 4> plRenderContext::s_Instances;

plMap<plRenderContext::ShaderVertexDecl, plGALVertexDeclarationHandle> plRenderContext::s_GALVertexDeclarations;

plMutex plRenderContext::s_ConstantBufferStorageMutex;
plIdTable<plConstantBufferStorageId, plConstantBufferStorageBase*> plRenderContext::s_ConstantBufferStorageTable;
plMap<plUInt32, plDynamicArray<plConstantBufferStorageBase*>> plRenderContext::s_FreeConstantBufferStorage;

plGALSamplerStateHandle plRenderContext::s_hDefaultSamplerStates[4];

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RendererContext)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    plShaderUtils::g_RequestBuiltinShaderCallback = plMakeDelegate(plRenderContext::LoadBuiltinShader);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    plShaderUtils::g_RequestBuiltinShaderCallback = {};
    plRenderContext::OnEngineShutdown();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

plRenderContext::Statistics::Statistics()
{
  Reset();
}

void plRenderContext::Statistics::Reset()
{
  m_uiFailedDrawcalls = 0;
}

//////////////////////////////////////////////////////////////////////////

plRenderContext* plRenderContext::GetDefaultInstance()
{
  if (s_pDefaultInstance == nullptr)
    s_pDefaultInstance = CreateInstance();

  return s_pDefaultInstance;
}

plRenderContext* plRenderContext::CreateInstance()
{
  return PLASMA_DEFAULT_NEW(plRenderContext);
}

void plRenderContext::DestroyInstance(plRenderContext* pRenderer)
{
  PLASMA_DEFAULT_DELETE(pRenderer);
}

plRenderContext::plRenderContext()
{
  if (s_pDefaultInstance == nullptr)
  {
    s_pDefaultInstance = this;
  }

  s_Instances.PushBack(this);

  m_StateFlags = plRenderContextFlags::AllStatesInvalid;
  m_Topology = plGALPrimitiveTopology::ENUM_COUNT; // Set to something invalid
  m_uiMeshBufferPrimitiveCount = 0;
  m_DefaultTextureFilter = plTextureFilterSetting::FixedAnisotropic4x;
  m_bAllowAsyncShaderLoading = false;

  m_hGlobalConstantBufferStorage = CreateConstantBufferStorage<plGlobalConstants>();

  ResetContextState();
}

plRenderContext::~plRenderContext()
{
  DeleteConstantBufferStorage(m_hGlobalConstantBufferStorage);

  if (s_pDefaultInstance == this)
    s_pDefaultInstance = nullptr;

  s_Instances.RemoveAndSwap(this);
}

plRenderContext::Statistics plRenderContext::GetAndResetStatistics()
{
  plRenderContext::Statistics ret = m_Statistics;
  ret.Reset();

  return ret;
}

plGALRenderCommandEncoder* plRenderContext::BeginRendering(plGALPass* pGALPass, const plGALRenderingSetup& renderingSetup, const plRectFloat& viewport, const char* szName, bool bStereoSupport)
{
  plGALMSAASampleCount::Enum msaaSampleCount = plGALMSAASampleCount::None;

  plGALRenderTargetViewHandle hRTV;
  if (renderingSetup.m_RenderTargetSetup.GetRenderTargetCount() > 0)
  {
    hRTV = renderingSetup.m_RenderTargetSetup.GetRenderTarget(0);
  }
  if (hRTV.IsInvalidated())
  {
    hRTV = renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget();
  }

  if (const plGALRenderTargetView* pRTV = plGALDevice::GetDefaultDevice()->GetRenderTargetView(hRTV))
  {
    msaaSampleCount = pRTV->GetTexture()->GetDescription().m_SampleCount;
  }

  if (msaaSampleCount != plGALMSAASampleCount::None)
  {
    SetShaderPermutationVariable("MSAA", "TRUE");
  }
  else
  {
    SetShaderPermutationVariable("MSAA", "FALSE");
  }

  #if PLASMA_ENABLED(PLASMA_GAMEOBJECT_VELOCITY)
    SetShaderPermutationVariable("GAMEOBJECT_VELOCITY", "TRUE");
  #else
    SetShaderPermutationVariable("GAMEOBJECT_VELOCITY", "FALSE");
  #endif

  auto& gc = WriteGlobalConstants();
  gc.ViewportSize = plVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);
  gc.NumMsaaSamples = msaaSampleCount;

  auto pGALCommandEncoder = pGALPass->BeginRendering(renderingSetup, szName);

  pGALCommandEncoder->SetViewport(viewport);

  m_pGALPass = pGALPass;
  m_pGALCommandEncoder = pGALCommandEncoder;
  m_bCompute = false;
  m_bStereoRendering = bStereoSupport;

  return pGALCommandEncoder;
}

void plRenderContext::EndRendering()
{
  m_pGALPass->EndRendering(GetRenderCommandEncoder());

  m_pGALPass = nullptr;
  m_pGALCommandEncoder = nullptr;
  m_bStereoRendering = false;

  // TODO: The render context needs to reset its state after every encoding block if we want to record to separate command buffers.
  // Although this is currently not possible since a lot of high level code binds stuff only once per frame on the render context.
  // Resetting the state after every encoding block breaks those assumptions.
  //ResetContextState();
}

plGALComputeCommandEncoder* plRenderContext::BeginCompute(plGALPass* pGALPass, const char* szName /*= ""*/)
{
  auto pGALCommandEncoder = pGALPass->BeginCompute(szName);

  m_pGALPass = pGALPass;
  m_pGALCommandEncoder = pGALCommandEncoder;
  m_bCompute = true;

  return pGALCommandEncoder;
}

void plRenderContext::EndCompute()
{
  m_pGALPass->EndCompute(GetComputeCommandEncoder());

  m_pGALPass = nullptr;
  m_pGALCommandEncoder = nullptr;

  // TODO: See EndRendering
  //ResetContextState();
}

void plRenderContext::SetShaderPermutationVariable(const char* szName, const plTempHashedString& sTempValue)
{
  plTempHashedString sHashedName(szName);

  plHashedString sName;
  plHashedString sValue;
  if (plShaderManager::IsPermutationValueAllowed(szName, sHashedName, sTempValue, sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}

void plRenderContext::SetShaderPermutationVariable(const plHashedString& sName, const plHashedString& sValue)
{
  if (plShaderManager::IsPermutationValueAllowed(sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}


void plRenderContext::BindMaterial(const plMaterialResourceHandle& hMaterial)
{
  // Don't set m_hMaterial directly since we first need to check whether the material has been modified in the mean time.
  m_hNewMaterial = hMaterial;
  m_StateFlags.Add(plRenderContextFlags::MaterialBindingChanged);
}

void plRenderContext::BindTexture2D(const plTempHashedString& sSlotName, const plTexture2DResourceHandle& hTexture,
  plResourceAcquireMode acquireMode /*= plResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    plResourceLock<plTexture2DResource> pTexture(hTexture, acquireMode);
    BindTexture2D(sSlotName, plGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
    BindSamplerState(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTexture2D(sSlotName, plGALResourceViewHandle());
  }
}

void plRenderContext::BindTexture3D(const plTempHashedString& sSlotName, const plTexture3DResourceHandle& hTexture,
  plResourceAcquireMode acquireMode /*= plResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    plResourceLock<plTexture3DResource> pTexture(hTexture, acquireMode);
    BindTexture3D(sSlotName, plGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
    BindSamplerState(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTexture3D(sSlotName, plGALResourceViewHandle());
  }
}

void plRenderContext::BindTextureCube(const plTempHashedString& sSlotName, const plTextureCubeResourceHandle& hTexture,
  plResourceAcquireMode acquireMode /*= plResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    plResourceLock<plTextureCubeResource> pTexture(hTexture, acquireMode);
    BindTextureCube(sSlotName, plGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
    BindSamplerState(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTextureCube(sSlotName, plGALResourceViewHandle());
  }
}

void plRenderContext::BindTexture2D(const plTempHashedString& sSlotName, plGALResourceViewHandle hResourceView)
{
  plGALResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundTextures2D.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundTextures2D.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(plRenderContextFlags::TextureBindingChanged);
}

void plRenderContext::BindTexture3D(const plTempHashedString& sSlotName, plGALResourceViewHandle hResourceView)
{
  plGALResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundTextures3D.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundTextures3D.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(plRenderContextFlags::TextureBindingChanged);
}

void plRenderContext::BindTextureCube(const plTempHashedString& sSlotName, plGALResourceViewHandle hResourceView)
{
  plGALResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundTexturesCube.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundTexturesCube.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(plRenderContextFlags::TextureBindingChanged);
}

void plRenderContext::BindUAV(const plTempHashedString& sSlotName, plGALUnorderedAccessViewHandle hUnorderedAccessView)
{
  plGALUnorderedAccessViewHandle* pOldResourceView = nullptr;
  if (m_BoundUAVs.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hUnorderedAccessView)
      return;

    *pOldResourceView = hUnorderedAccessView;
  }
  else
  {
    m_BoundUAVs.Insert(sSlotName.GetHash(), hUnorderedAccessView);
  }

  m_StateFlags.Add(plRenderContextFlags::UAVBindingChanged);
}


void plRenderContext::BindSamplerState(const plTempHashedString& sSlotName, plGALSamplerStateHandle hSamplerSate)
{
  PLASMA_ASSERT_DEBUG(sSlotName != "LinearSampler", "'LinearSampler' is a resevered sampler name and must not be set manually.");
  PLASMA_ASSERT_DEBUG(sSlotName != "LinearClampSampler", "'LinearClampSampler' is a resevered sampler name and must not be set manually.");
  PLASMA_ASSERT_DEBUG(sSlotName != "PointSampler", "'PointSampler' is a resevered sampler name and must not be set manually.");
  PLASMA_ASSERT_DEBUG(sSlotName != "PointClampSampler", "'PointClampSampler' is a resevered sampler name and must not be set manually.");

  plGALSamplerStateHandle* pOldSamplerState = nullptr;
  if (m_BoundSamplers.TryGetValue(sSlotName.GetHash(), pOldSamplerState))
  {
    if (*pOldSamplerState == hSamplerSate)
      return;

    *pOldSamplerState = hSamplerSate;
  }
  else
  {
    m_BoundSamplers.Insert(sSlotName.GetHash(), hSamplerSate);
  }

  m_StateFlags.Add(plRenderContextFlags::SamplerBindingChanged);
}

void plRenderContext::BindBuffer(const plTempHashedString& sSlotName, plGALResourceViewHandle hResourceView)
{
  plGALResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundBuffer.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundBuffer.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(plRenderContextFlags::BufferBindingChanged);
}

void plRenderContext::BindConstantBuffer(const plTempHashedString& sSlotName, plGALBufferHandle hConstantBuffer)
{
  BoundConstantBuffer* pBoundConstantBuffer = nullptr;
  if (m_BoundConstantBuffers.TryGetValue(sSlotName.GetHash(), pBoundConstantBuffer))
  {
    if (pBoundConstantBuffer->m_hConstantBuffer == hConstantBuffer)
      return;

    pBoundConstantBuffer->m_hConstantBuffer = hConstantBuffer;
    pBoundConstantBuffer->m_hConstantBufferStorage.Invalidate();
  }
  else
  {
    m_BoundConstantBuffers.Insert(sSlotName.GetHash(), BoundConstantBuffer(hConstantBuffer));
  }

  m_StateFlags.Add(plRenderContextFlags::ConstantBufferBindingChanged);
}

void plRenderContext::BindConstantBuffer(const plTempHashedString& sSlotName, plConstantBufferStorageHandle hConstantBufferStorage)
{
  BoundConstantBuffer* pBoundConstantBuffer = nullptr;
  if (m_BoundConstantBuffers.TryGetValue(sSlotName.GetHash(), pBoundConstantBuffer))
  {
    if (pBoundConstantBuffer->m_hConstantBufferStorage == hConstantBufferStorage)
      return;

    pBoundConstantBuffer->m_hConstantBuffer.Invalidate();
    pBoundConstantBuffer->m_hConstantBufferStorage = hConstantBufferStorage;
  }
  else
  {
    m_BoundConstantBuffers.Insert(sSlotName.GetHash(), BoundConstantBuffer(hConstantBufferStorage));
  }

  m_StateFlags.Add(plRenderContextFlags::ConstantBufferBindingChanged);
}

void plRenderContext::BindShader(const plShaderResourceHandle& hShader, plBitflags<plShaderBindFlags> flags)
{
  m_hMaterial.Invalidate();
  m_StateFlags.Remove(plRenderContextFlags::MaterialBindingChanged);

  BindShaderInternal(hShader, flags);
}

void plRenderContext::BindMeshBuffer(const plMeshBufferResourceHandle& hMeshBuffer)
{
  plResourceLock<plMeshBufferResource> pMeshBuffer(hMeshBuffer, plResourceAcquireMode::AllowLoadingFallback);
  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetVertexDeclaration()), pMeshBuffer->GetTopology(),
    pMeshBuffer->GetPrimitiveCount());
}

void plRenderContext::BindMeshBuffer(plGALBufferHandle hVertexBuffer, plGALBufferHandle hIndexBuffer,
  const plVertexDeclarationInfo* pVertexDeclarationInfo, plGALPrimitiveTopology::Enum topology, plUInt32 uiPrimitiveCount, plGALBufferHandle hVertexBuffer2, plGALBufferHandle hVertexBuffer3, plGALBufferHandle hVertexBuffer4)
{
  if (m_hVertexBuffers[0] == hVertexBuffer && m_hVertexBuffers[1] == hVertexBuffer2 && m_hVertexBuffers[2] == hVertexBuffer3 && m_hVertexBuffers[3] == hVertexBuffer4 && m_hIndexBuffer == hIndexBuffer && m_pVertexDeclarationInfo == pVertexDeclarationInfo && m_Topology == topology && m_uiMeshBufferPrimitiveCount == uiPrimitiveCount)
  {
    return;
  }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  if (pVertexDeclarationInfo)
  {
    for (plUInt32 i1 = 0; i1 < pVertexDeclarationInfo->m_VertexStreams.GetCount(); ++i1)
    {
      for (plUInt32 i2 = 0; i2 < pVertexDeclarationInfo->m_VertexStreams.GetCount(); ++i2)
      {
        if (i1 != i2)
        {
          PLASMA_ASSERT_DEBUG(pVertexDeclarationInfo->m_VertexStreams[i1].m_Semantic != pVertexDeclarationInfo->m_VertexStreams[i2].m_Semantic,
            "Same semantic cannot be used twice in the same vertex declaration");
        }
      }
    }
  }
#endif

  if (m_Topology != topology)
  {
    m_Topology = topology;

    plTempHashedString sTopologies[plGALPrimitiveTopology::ENUM_COUNT] = {
      plTempHashedString("TOPOLOGY_POINTS"), plTempHashedString("TOPOLOGY_LINES"), plTempHashedString("TOPOLOGY_TRIANGLES")};

    SetShaderPermutationVariable("TOPOLOGY", sTopologies[m_Topology]);
  }

  m_hVertexBuffers[0] = hVertexBuffer;
  m_hVertexBuffers[1] = hVertexBuffer2;
  m_hVertexBuffers[2] = hVertexBuffer3;
  m_hVertexBuffers[3] = hVertexBuffer4;
  m_hIndexBuffer = hIndexBuffer;
  m_pVertexDeclarationInfo = pVertexDeclarationInfo;
  m_uiMeshBufferPrimitiveCount = uiPrimitiveCount;

  m_StateFlags.Add(plRenderContextFlags::MeshBufferBindingChanged);
}

void plRenderContext::BindMeshBuffer(const plDynamicMeshBufferResourceHandle& hDynamicMeshBuffer)
{
  plResourceLock<plDynamicMeshBufferResource> pMeshBuffer(hDynamicMeshBuffer, plResourceAcquireMode::AllowLoadingFallback);
  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetVertexDeclaration()), pMeshBuffer->GetDescriptor().m_Topology, pMeshBuffer->GetDescriptor().m_uiMaxPrimitives, pMeshBuffer->GetColorBuffer());
}

plResult plRenderContext::DrawMeshBuffer(plUInt32 uiPrimitiveCount, plUInt32 uiFirstPrimitive, plUInt32 uiInstanceCount)
{
  if (ApplyContextStates().Failed() || uiPrimitiveCount == 0 || uiInstanceCount == 0)
  {
    m_Statistics.m_uiFailedDrawcalls++;
    return PLASMA_FAILURE;
  }

  PLASMA_ASSERT_DEV(uiFirstPrimitive < m_uiMeshBufferPrimitiveCount, "Invalid primitive range: first primitive ({0}) can't be larger than number of primitives ({1})", uiFirstPrimitive, uiPrimitiveCount);

  uiPrimitiveCount = plMath::Min(uiPrimitiveCount, m_uiMeshBufferPrimitiveCount - uiFirstPrimitive);
  PLASMA_ASSERT_DEV(uiPrimitiveCount > 0, "Invalid primitive range: number of primitives can't be zero.");

  auto pCommandEncoder = GetRenderCommandEncoder();

  const plUInt32 uiVertsPerPrimitive = plGALPrimitiveTopology::VerticesPerPrimitive(pCommandEncoder->GetPrimitiveTopology());

  uiPrimitiveCount *= uiVertsPerPrimitive;
  uiFirstPrimitive *= uiVertsPerPrimitive;
  if (m_bStereoRendering)
  {
    uiInstanceCount *= 2;
  }

  if (uiInstanceCount > 1)
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      pCommandEncoder->DrawIndexedInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
    else
    {
      pCommandEncoder->DrawInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
  }
  else
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      pCommandEncoder->DrawIndexed(uiPrimitiveCount, uiFirstPrimitive);
    }
    else
    {
      pCommandEncoder->Draw(uiPrimitiveCount, uiFirstPrimitive);
    }
  }

  return PLASMA_SUCCESS;
}

plResult plRenderContext::Dispatch(plUInt32 uiThreadGroupCountX, plUInt32 uiThreadGroupCountY, plUInt32 uiThreadGroupCountZ)
{
  if (ApplyContextStates().Failed())
  {
    m_Statistics.m_uiFailedDrawcalls++;
    return PLASMA_FAILURE;
  }

  GetComputeCommandEncoder()->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);

  return PLASMA_SUCCESS;
}

plResult plRenderContext::ApplyContextStates(bool bForce)
{
  // First apply material state since this can modify all other states.
  // Note ApplyMaterialState only returns a valid material pointer if the constant buffer of this material needs to be updated.
  // This needs to be done once we have determined the correct shader permutation.
  plMaterialResource* pMaterial = nullptr;
  PLASMA_SCOPE_EXIT(if (pMaterial != nullptr) { plResourceManager::EndAcquireResource(pMaterial); });

  if (bForce || m_StateFlags.IsSet(plRenderContextFlags::MaterialBindingChanged))
  {
    pMaterial = ApplyMaterialState();

    m_StateFlags.Remove(plRenderContextFlags::MaterialBindingChanged);
  }

  plShaderPermutationResource* pShaderPermutation = nullptr;
  PLASMA_SCOPE_EXIT(if (pShaderPermutation != nullptr) { plResourceManager::EndAcquireResource(pShaderPermutation); });

  bool bRebuildVertexDeclaration = m_StateFlags.IsAnySet(plRenderContextFlags::ShaderStateChanged | plRenderContextFlags::MeshBufferBindingChanged);

  if (bForce || m_StateFlags.IsSet(plRenderContextFlags::ShaderStateChanged))
  {
    pShaderPermutation = ApplyShaderState();

    if (pShaderPermutation == nullptr)
    {
      return PLASMA_FAILURE;
    }

    m_StateFlags.Remove(plRenderContextFlags::ShaderStateChanged);
  }
  else
  {
    pShaderPermutation = plResourceManager::BeginAcquireResource(
      m_hActiveShaderPermutation, m_bAllowAsyncShaderLoading ? plResourceAcquireMode::AllowLoadingFallback : plResourceAcquireMode::BlockTillLoaded);
  }

  if (m_hActiveShaderPermutation.IsValid())
  {
    if ((bForce || m_StateFlags.IsAnySet(plRenderContextFlags::TextureBindingChanged | plRenderContextFlags::UAVBindingChanged |
                                         plRenderContextFlags::SamplerBindingChanged | plRenderContextFlags::BufferBindingChanged |
                                         plRenderContextFlags::ConstantBufferBindingChanged)))
    {
      if (pShaderPermutation == nullptr)
      {
        pShaderPermutation = plResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, plResourceAcquireMode::BlockTillLoaded);
      }
    }

    plLogBlock applyBindingsBlock("Applying Shader Bindings", pShaderPermutation != nullptr ? pShaderPermutation->GetResourceDescription().GetData() : "");

    if (pShaderPermutation == nullptr)
    {
      return PLASMA_FAILURE;
    }
    plGALShaderHandle hShader = pShaderPermutation->GetGALShader();
    const plGALShader* pShader = plGALDevice::GetDefaultDevice()->GetShader(hShader);

    if (bForce || m_StateFlags.IsSet(plRenderContextFlags::UAVBindingChanged))
    {
      ApplyUAVBindings(pShader);
      m_StateFlags.Remove(plRenderContextFlags::UAVBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(plRenderContextFlags::TextureBindingChanged))
    {
      ApplyTextureBindings(pShader);
      m_StateFlags.Remove(plRenderContextFlags::TextureBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(plRenderContextFlags::SamplerBindingChanged))
    {
      ApplySamplerBindings(pShader);
      m_StateFlags.Remove(plRenderContextFlags::SamplerBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(plRenderContextFlags::BufferBindingChanged))
    {
      ApplyBufferBindings(pShader);
      m_StateFlags.Remove(plRenderContextFlags::BufferBindingChanged);
    }

    if (pMaterial != nullptr)
    {
      pMaterial->UpdateConstantBuffer(pShaderPermutation);
      BindConstantBuffer("plMaterialConstants", pMaterial->m_hConstantBufferStorage);
    }

    UploadConstants();

    if (bForce || m_StateFlags.IsSet(plRenderContextFlags::ConstantBufferBindingChanged))
    {
      ApplyConstantBufferBindings(pShader);
      m_StateFlags.Remove(plRenderContextFlags::ConstantBufferBindingChanged);
    }
  }

  if ((bForce || bRebuildVertexDeclaration) && !m_bCompute)
  {
    if (m_hActiveGALShader.IsInvalidated())
      return PLASMA_FAILURE;

    auto pCommandEncoder = GetRenderCommandEncoder();

    if (bForce || m_StateFlags.IsSet(plRenderContextFlags::MeshBufferBindingChanged))
    {
      pCommandEncoder->SetPrimitiveTopology(m_Topology);

      for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(m_hVertexBuffers); ++i)
      {
        pCommandEncoder->SetVertexBuffer(i, m_hVertexBuffers[i]);
      }

      if (!m_hIndexBuffer.IsInvalidated())
        pCommandEncoder->SetIndexBuffer(m_hIndexBuffer);
    }

    plGALVertexDeclarationHandle hVertexDeclaration;
    if (m_pVertexDeclarationInfo != nullptr && BuildVertexDeclaration(m_hActiveGALShader, *m_pVertexDeclarationInfo, hVertexDeclaration).Failed())
      return PLASMA_FAILURE;

    // If there is a vertex buffer we need a valid vertex declaration as well.
    if ((!m_hVertexBuffers[0].IsInvalidated() || !m_hVertexBuffers[1].IsInvalidated() || !m_hVertexBuffers[2].IsInvalidated() || !m_hVertexBuffers[3].IsInvalidated()) && hVertexDeclaration.IsInvalidated())
      return PLASMA_FAILURE;

    pCommandEncoder->SetVertexDeclaration(hVertexDeclaration);

    m_StateFlags.Remove(plRenderContextFlags::MeshBufferBindingChanged);
  }

  return PLASMA_SUCCESS;
}

void plRenderContext::ResetContextState()
{
  m_StateFlags = plRenderContextFlags::AllStatesInvalid;

  m_hActiveShader.Invalidate();
  m_hActiveGALShader.Invalidate();

  m_PermutationVariables.Clear();
  m_hNewMaterial.Invalidate();
  m_hMaterial.Invalidate();

  m_hActiveShaderPermutation.Invalidate();

  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
  }

  m_hIndexBuffer.Invalidate();
  m_pVertexDeclarationInfo = nullptr;
  m_Topology = plGALPrimitiveTopology::ENUM_COUNT; // Set to something invalid
  m_uiMeshBufferPrimitiveCount = 0;

  m_BoundTextures2D.Clear();
  m_BoundTextures3D.Clear();
  m_BoundTexturesCube.Clear();
  m_BoundBuffer.Clear();

  m_BoundSamplers.Clear();
  m_BoundSamplers.Insert(plHashingUtils::StringHash("LinearSampler"), GetDefaultSamplerState(plDefaultSamplerFlags::LinearFiltering));
  m_BoundSamplers.Insert(plHashingUtils::StringHash("LinearClampSampler"), GetDefaultSamplerState(plDefaultSamplerFlags::LinearFiltering | plDefaultSamplerFlags::Clamp));
  m_BoundSamplers.Insert(plHashingUtils::StringHash("PointSampler"), GetDefaultSamplerState(plDefaultSamplerFlags::PointFiltering));
  m_BoundSamplers.Insert(plHashingUtils::StringHash("PointClampSampler"), GetDefaultSamplerState(plDefaultSamplerFlags::PointFiltering | plDefaultSamplerFlags::Clamp));

  m_BoundUAVs.Clear();
  m_BoundConstantBuffers.Clear();
}

plGlobalConstants& plRenderContext::WriteGlobalConstants()
{
  plConstantBufferStorage<plGlobalConstants>* pStorage = nullptr;
  PLASMA_VERIFY(TryGetConstantBufferStorage(m_hGlobalConstantBufferStorage, pStorage), "Invalid Global Constant Storage");
  return pStorage->GetDataForWriting();
}

const plGlobalConstants& plRenderContext::ReadGlobalConstants() const
{
  plConstantBufferStorage<plGlobalConstants>* pStorage = nullptr;
  PLASMA_VERIFY(TryGetConstantBufferStorage(m_hGlobalConstantBufferStorage, pStorage), "Invalid Global Constant Storage");
  return pStorage->GetDataForReading();
}

// static
plConstantBufferStorageHandle plRenderContext::CreateConstantBufferStorage(plUInt32 uiSizeInBytes, plConstantBufferStorageBase*& out_pStorage)
{
  PLASMA_ASSERT_DEV(plMemoryUtils::IsSizeAligned(uiSizeInBytes, 16u), "Storage struct for constant buffer is not aligned to 16 bytes");

  PLASMA_LOCK(s_ConstantBufferStorageMutex);

  plConstantBufferStorageBase* pStorage = nullptr;

  auto it = s_FreeConstantBufferStorage.Find(uiSizeInBytes);
  if (it.IsValid())
  {
    plDynamicArray<plConstantBufferStorageBase*>& storageForSize = it.Value();
    if (!storageForSize.IsEmpty())
    {
      pStorage = storageForSize[0];
      storageForSize.RemoveAtAndSwap(0);
    }
  }

  if (pStorage == nullptr)
  {
    pStorage = PLASMA_DEFAULT_NEW(plConstantBufferStorageBase, uiSizeInBytes);
  }

  out_pStorage = pStorage;
  return plConstantBufferStorageHandle(s_ConstantBufferStorageTable.Insert(pStorage));
}

// static
void plRenderContext::DeleteConstantBufferStorage(plConstantBufferStorageHandle hStorage)
{
  PLASMA_LOCK(s_ConstantBufferStorageMutex);

  plConstantBufferStorageBase* pStorage = nullptr;
  if (!s_ConstantBufferStorageTable.Remove(hStorage.m_InternalId, &pStorage))
  {
    // already deleted
    return;
  }

  plUInt32 uiSizeInBytes = pStorage->m_Data.GetCount();

  auto it = s_FreeConstantBufferStorage.Find(uiSizeInBytes);
  if (!it.IsValid())
  {
    it = s_FreeConstantBufferStorage.Insert(uiSizeInBytes, plDynamicArray<plConstantBufferStorageBase*>());
  }

  it.Value().PushBack(pStorage);
}

// static
bool plRenderContext::TryGetConstantBufferStorage(plConstantBufferStorageHandle hStorage, plConstantBufferStorageBase*& out_pStorage)
{
  PLASMA_LOCK(s_ConstantBufferStorageMutex);
  return s_ConstantBufferStorageTable.TryGetValue(hStorage.m_InternalId, out_pStorage);
}

// static
plGALSamplerStateHandle plRenderContext::GetDefaultSamplerState(plBitflags<plDefaultSamplerFlags> flags)
{
  plUInt32 uiSamplerStateIndex = flags.GetValue();
  PLASMA_ASSERT_DEV(uiSamplerStateIndex < PLASMA_ARRAY_SIZE(s_hDefaultSamplerStates), "");

  if (s_hDefaultSamplerStates[uiSamplerStateIndex].IsInvalidated())
  {
    plGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = flags.IsSet(plDefaultSamplerFlags::LinearFiltering) ? plGALTextureFilterMode::Linear : plGALTextureFilterMode::Point;
    desc.m_MagFilter = flags.IsSet(plDefaultSamplerFlags::LinearFiltering) ? plGALTextureFilterMode::Linear : plGALTextureFilterMode::Point;
    desc.m_MipFilter = flags.IsSet(plDefaultSamplerFlags::LinearFiltering) ? plGALTextureFilterMode::Linear : plGALTextureFilterMode::Point;

    desc.m_AddressU = flags.IsSet(plDefaultSamplerFlags::Clamp) ? plImageAddressMode::Clamp : plImageAddressMode::Repeat;
    desc.m_AddressV = flags.IsSet(plDefaultSamplerFlags::Clamp) ? plImageAddressMode::Clamp : plImageAddressMode::Repeat;
    desc.m_AddressW = flags.IsSet(plDefaultSamplerFlags::Clamp) ? plImageAddressMode::Clamp : plImageAddressMode::Repeat;

    s_hDefaultSamplerStates[uiSamplerStateIndex] = plGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }

  return s_hDefaultSamplerStates[uiSamplerStateIndex];
}

// private functions
//////////////////////////////////////////////////////////////////////////

// static
void plRenderContext::LoadBuiltinShader(plShaderUtils::plBuiltinShaderType type, plShaderUtils::plBuiltinShader& out_shader)
{
  plShaderResourceHandle hActiveShader;
  bool bStereo = false;
  switch (type)
  {
    case plShaderUtils::plBuiltinShaderType::CopyImageArray:
      bStereo = true;
      [[fallthrough]];
    case plShaderUtils::plBuiltinShaderType::CopyImage:
      hActiveShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/Copy_FS.plShader");
      break;
    case plShaderUtils::plBuiltinShaderType::DownscaleImageArray:
      bStereo = true;
      [[fallthrough]];
    case plShaderUtils::plBuiltinShaderType::DownscaleImage:
      hActiveShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/Downscale.plShader");
      break;
  }

  PLASMA_ASSERT_DEV(hActiveShader.IsValid(), "Could not load builtin shader!");

  plHashTable<plHashedString, plHashedString> permutationVariables;
  static plHashedString sVSRTAI = plMakeHashedString("VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX");
  static plHashedString sTrue = plMakeHashedString("TRUE");
  static plHashedString sFalse = plMakeHashedString("FALSE");
  static plHashedString sCameraMode = plMakeHashedString("CAMERA_MODE");
  static plHashedString sPerspective = plMakeHashedString("CAMERA_MODE_PERSPECTIVE");
  static plHashedString sStereo = plMakeHashedString("CAMERA_MODE_STEREO");

  permutationVariables.Insert(sCameraMode, bStereo ? sStereo : sPerspective);
  if (plGALDevice::GetDefaultDevice()->GetCapabilities().m_bVertexShaderRenderTargetArrayIndex)
    permutationVariables.Insert(sVSRTAI, sTrue);
  else
    permutationVariables.Insert(sVSRTAI, sFalse);


  plShaderPermutationResourceHandle hActiveShaderPermutation = plShaderManager::PreloadSinglePermutation(hActiveShader, permutationVariables, false);

  PLASMA_ASSERT_DEV(hActiveShaderPermutation.IsValid(), "Could not load builtin shader permutation!");

  plResourceLock<plShaderPermutationResource> pShaderPermutation(hActiveShaderPermutation, plResourceAcquireMode::BlockTillLoaded);

  PLASMA_ASSERT_DEV(pShaderPermutation->IsShaderValid(), "Builtin shader permutation shader is invalid!");

  out_shader.m_hActiveGALShader = pShaderPermutation->GetGALShader();
  PLASMA_ASSERT_DEV(!out_shader.m_hActiveGALShader.IsInvalidated(), "Invalid GAL Shader handle.");

  out_shader.m_hBlendState = pShaderPermutation->GetBlendState();
  out_shader.m_hDepthStencilState = pShaderPermutation->GetDepthStencilState();
  out_shader.m_hRasterizerState = pShaderPermutation->GetRasterizerState();
}

// static
void plRenderContext::OnEngineShutdown()
{
  plShaderStageBinary::OnEngineShutdown();

  for (auto rc : s_Instances)
    PLASMA_DEFAULT_DELETE(rc);

  s_Instances.Clear();

  // Cleanup sampler states
  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(s_hDefaultSamplerStates); ++i)
  {
    if (!s_hDefaultSamplerStates[i].IsInvalidated())
    {
      plGALDevice::GetDefaultDevice()->DestroySamplerState(s_hDefaultSamplerStates[i]);
      s_hDefaultSamplerStates[i].Invalidate();
    }
  }

  // Cleanup vertex declarations
  {
    for (auto it = s_GALVertexDeclarations.GetIterator(); it.IsValid(); ++it)
    {
      plGALDevice::GetDefaultDevice()->DestroyVertexDeclaration(it.Value());
    }

    s_GALVertexDeclarations.Clear();
  }

  // Cleanup constant buffer storage
  {
    for (auto it = s_ConstantBufferStorageTable.GetIterator(); it.IsValid(); ++it)
    {
      plConstantBufferStorageBase* pStorage = it.Value();
      PLASMA_DEFAULT_DELETE(pStorage);
    }

    s_ConstantBufferStorageTable.Clear();

    for (auto it = s_FreeConstantBufferStorage.GetIterator(); it.IsValid(); ++it)
    {
      plDynamicArray<plConstantBufferStorageBase*>& storageForSize = it.Value();
      for (auto& pStorage : storageForSize)
      {
        PLASMA_DEFAULT_DELETE(pStorage);
      }
    }

    s_FreeConstantBufferStorage.Clear();
  }
}

// static
plResult plRenderContext::BuildVertexDeclaration(plGALShaderHandle hShader, const plVertexDeclarationInfo& decl, plGALVertexDeclarationHandle& out_Declaration)
{
  ShaderVertexDecl svd;
  svd.m_hShader = hShader;
  svd.m_uiVertexDeclarationHash = decl.m_uiHash;

  bool bExisted = false;
  auto it = s_GALVertexDeclarations.FindOrAdd(svd, &bExisted);

  if (!bExisted)
  {
    const plGALShader* pShader = plGALDevice::GetDefaultDevice()->GetShader(hShader);

    auto pBytecode = pShader->GetDescription().m_ByteCodes[plGALShaderStage::VertexShader];

    plGALVertexDeclarationCreationDescription vd;
    vd.m_hShader = hShader;

    for (plUInt32 slot = 0; slot < decl.m_VertexStreams.GetCount(); ++slot)
    {
      auto& stream = decl.m_VertexStreams[slot];

      // stream.m_Format
      plGALVertexAttribute gal;
      gal.m_bInstanceData = false;
      gal.m_eFormat = stream.m_Format;
      gal.m_eSemantic = stream.m_Semantic;
      gal.m_uiOffset = stream.m_uiOffset;
      gal.m_uiVertexBufferSlot = stream.m_uiVertexBufferSlot;
      vd.m_VertexAttributes.PushBack(gal);
    }

    out_Declaration = plGALDevice::GetDefaultDevice()->CreateVertexDeclaration(vd);

    if (out_Declaration.IsInvalidated())
    {
      /* This can happen when the resource system gives you a fallback resource, which then selects a shader that
      does not fit the mesh layout.
      E.g. when a material is not yet loaded and the fallback material is used, that fallback material may
      use another shader, that requires more data streams, than what the mesh provides.
      This problem will go away, once the proper material is loaded.

      This can be fixed by ensuring that the fallback material uses a shader that only requires data that is
      always there, e.g. only position and maybe a texcoord, and of course all meshes must provide at least those
      data streams.

      Otherwise, this is harmless, the renderer will ignore invalid drawcalls and once all the correct stuff is
      available, it will work.
      */

      plLog::Warning("Failed to create vertex declaration");
      return PLASMA_FAILURE;
    }

    it.Value() = out_Declaration;
  }

  out_Declaration = it.Value();
  return PLASMA_SUCCESS;
}

void plRenderContext::UploadConstants()
{
  BindConstantBuffer("plGlobalConstants", m_hGlobalConstantBufferStorage);

  for (auto it = m_BoundConstantBuffers.GetIterator(); it.IsValid(); ++it)
  {
    plConstantBufferStorageHandle hConstantBufferStorage = it.Value().m_hConstantBufferStorage;
    plConstantBufferStorageBase* pConstantBufferStorage = nullptr;
    if (TryGetConstantBufferStorage(hConstantBufferStorage, pConstantBufferStorage))
    {
      pConstantBufferStorage->UploadData(m_pGALCommandEncoder);
    }
  }
}

void plRenderContext::SetShaderPermutationVariableInternal(const plHashedString& sName, const plHashedString& sValue)
{
  plHashedString* pOldValue = nullptr;
  m_PermutationVariables.TryGetValue(sName, pOldValue);

  if (pOldValue == nullptr || *pOldValue != sValue)
  {
    m_PermutationVariables.Insert(sName, sValue);
    m_StateFlags.Add(plRenderContextFlags::ShaderStateChanged);
  }
}

void plRenderContext::BindShaderInternal(const plShaderResourceHandle& hShader, plBitflags<plShaderBindFlags> flags)
{
  if (flags.IsAnySet(plShaderBindFlags::ForceRebind) || m_hActiveShader != hShader)
  {
    m_ShaderBindFlags = flags;
    m_hActiveShader = hShader;

    m_StateFlags.Add(plRenderContextFlags::ShaderStateChanged);
  }
}

plShaderPermutationResource* plRenderContext::ApplyShaderState()
{
  m_hActiveGALShader.Invalidate();

  m_StateFlags.Add(plRenderContextFlags::TextureBindingChanged | plRenderContextFlags::SamplerBindingChanged |
                   plRenderContextFlags::BufferBindingChanged | plRenderContextFlags::ConstantBufferBindingChanged);

  if (!m_hActiveShader.IsValid())
    return nullptr;

  m_hActiveShaderPermutation = plShaderManager::PreloadSinglePermutation(m_hActiveShader, m_PermutationVariables, m_bAllowAsyncShaderLoading);

  if (!m_hActiveShaderPermutation.IsValid())
    return nullptr;

  plShaderPermutationResource* pShaderPermutation = plResourceManager::BeginAcquireResource(
    m_hActiveShaderPermutation, m_bAllowAsyncShaderLoading ? plResourceAcquireMode::AllowLoadingFallback : plResourceAcquireMode::BlockTillLoaded);

  if (!pShaderPermutation->IsShaderValid())
  {
    plResourceManager::EndAcquireResource(pShaderPermutation);
    return nullptr;
  }

  m_hActiveGALShader = pShaderPermutation->GetGALShader();
  PLASMA_ASSERT_DEV(!m_hActiveGALShader.IsInvalidated(), "Invalid GAL Shader handle.");

  m_pGALCommandEncoder->SetShader(m_hActiveGALShader);

  // Set render state from shader
  if (!m_bCompute)
  {
    auto pCommandEncoder = GetRenderCommandEncoder();

    if (!m_ShaderBindFlags.IsSet(plShaderBindFlags::NoBlendState))
      pCommandEncoder->SetBlendState(pShaderPermutation->GetBlendState());

    if (!m_ShaderBindFlags.IsSet(plShaderBindFlags::NoRasterizerState))
      pCommandEncoder->SetRasterizerState(pShaderPermutation->GetRasterizerState());

    if (!m_ShaderBindFlags.IsSet(plShaderBindFlags::NoDepthStencilState))
      pCommandEncoder->SetDepthStencilState(pShaderPermutation->GetDepthStencilState());
  }

  return pShaderPermutation;
}

plMaterialResource* plRenderContext::ApplyMaterialState()
{
  if (!m_hNewMaterial.IsValid())
  {
    BindShaderInternal(plShaderResourceHandle(), plShaderBindFlags::Default);
    return nullptr;
  }

  // check whether material has been modified
  plMaterialResource* pMaterial = plResourceManager::BeginAcquireResource(m_hNewMaterial, plResourceAcquireMode::AllowLoadingFallback);

  if (m_hNewMaterial != m_hMaterial || pMaterial->IsModified())
  {
    auto pCachedValues = pMaterial->GetOrUpdateCachedValues();

    BindShaderInternal(pCachedValues->m_hShader, plShaderBindFlags::Default);

    if (!pMaterial->m_hConstantBufferStorage.IsInvalidated())
    {
      BindConstantBuffer("plMaterialConstants", pMaterial->m_hConstantBufferStorage);
    }

    for (auto it = pCachedValues->m_PermutationVars.GetIterator(); it.IsValid(); ++it)
    {
      SetShaderPermutationVariableInternal(it.Key(), it.Value());
    }

    for (auto it = pCachedValues->m_Texture2DBindings.GetIterator(); it.IsValid(); ++it)
    {
      BindTexture2D(it.Key(), it.Value());
    }

    for(auto it = pCachedValues->m_Texture3DBindings.GetIterator(); it.IsValid(); ++it)
    {
      BindTexture3D(it.Key(), it.Value());
    }

    for (auto it = pCachedValues->m_TextureCubeBindings.GetIterator(); it.IsValid(); ++it)
    {
      BindTextureCube(it.Key(), it.Value());
    }

    m_hMaterial = m_hNewMaterial;
  }

  // The material needs its constant buffer updated.
  // Thus we keep it acquired until we have the correct shader permutation for the constant buffer layout.
  if (pMaterial->AreConstantsModified())
  {
    m_StateFlags.Add(plRenderContextFlags::ConstantBufferBindingChanged);

    return pMaterial;
  }

  plResourceManager::EndAcquireResource(pMaterial);
  return nullptr;
}

void plRenderContext::ApplyConstantBufferBindings(const plGALShader* pShader)
{
  const auto& bindings = pShader->GetBindingMapping();
  for (const plShaderResourceBinding& binding : bindings)
  {
    if (binding.m_DescriptorType != plGALShaderDescriptorType::ConstantBuffer)
      continue;

    const plUInt64 uiResourceHash = binding.m_sName.GetHash();

    BoundConstantBuffer boundConstantBuffer;
    if (!m_BoundConstantBuffers.TryGetValue(uiResourceHash, boundConstantBuffer))
    {
      // If the shader was compiled with debug info the shader compiler will not strip unused resources and
      // thus this error would trigger although the shader doesn't actually uses the resource.
      // #TODO_SHADER if (!pBinary->GetByteCode()->m_bWasCompiledWithDebug)
      {
        plLog::Error("No resource is bound for constant buffer slot '{0}'", binding.m_sName);
      }
      m_pGALCommandEncoder->SetConstantBuffer(binding, plGALBufferHandle());
      continue;
    }

    if (!boundConstantBuffer.m_hConstantBuffer.IsInvalidated())
    {
      m_pGALCommandEncoder->SetConstantBuffer(binding, boundConstantBuffer.m_hConstantBuffer);
    }
    else
    {
      plConstantBufferStorageBase* pConstantBufferStorage = nullptr;
      if (TryGetConstantBufferStorage(boundConstantBuffer.m_hConstantBufferStorage, pConstantBufferStorage))
      {
        m_pGALCommandEncoder->SetConstantBuffer(binding, pConstantBufferStorage->GetGALBufferHandle());
      }
      else
      {
        plLog::Error("Invalid constant buffer storage is bound for slot '{0}'", binding.m_sName);
        m_pGALCommandEncoder->SetConstantBuffer(binding, plGALBufferHandle());
      }
    }
  }
}

void plRenderContext::ApplyTextureBindings(const plGALShader* pShader)
{
  const auto& bindings = pShader->GetBindingMapping();
  for (const plShaderResourceBinding& binding : bindings)
  {
    const plUInt64 uiResourceHash = binding.m_sName.GetHash();
    plGALResourceViewHandle hResourceView;

    if (binding.m_DescriptorType == plGALShaderDescriptorType::Texture)
    {
      switch (binding.m_TextureType)
      {
        case plGALShaderTextureType::Texture2D:
        case plGALShaderTextureType::Texture2DArray:
        case plGALShaderTextureType::Texture2DMS:
        case plGALShaderTextureType::Texture2DMSArray:
          m_BoundTextures2D.TryGetValue(uiResourceHash, hResourceView);
          m_pGALCommandEncoder->SetResourceView(binding, hResourceView);
          break;
        case plGALShaderTextureType::Texture3D:
          m_BoundTextures3D.TryGetValue(uiResourceHash, hResourceView);
          m_pGALCommandEncoder->SetResourceView(binding, hResourceView);
          break;
        case plGALShaderTextureType::TextureCube:
        case plGALShaderTextureType::TextureCubeArray:
          m_BoundTexturesCube.TryGetValue(uiResourceHash, hResourceView);
          m_pGALCommandEncoder->SetResourceView(binding, hResourceView);
          break;
        case plGALShaderTextureType::Texture1D:
        case plGALShaderTextureType::Texture1DArray:
        default:
          PLASMA_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }
  }
}

void plRenderContext::ApplyUAVBindings(const plGALShader* pShader)
{
  const auto& bindings = pShader->GetBindingMapping();
  for (const plShaderResourceBinding& binding : bindings)
  {
    auto type = plGALShaderResourceType::MakeFromShaderDescriptorType(binding.m_DescriptorType);
    if (type == plGALShaderResourceType::UAV)
    {
      const plUInt64 uiResourceHash = binding.m_sName.GetHash();

      plGALUnorderedAccessViewHandle hResourceView;
      m_BoundUAVs.TryGetValue(uiResourceHash, hResourceView);

      m_pGALCommandEncoder->SetUnorderedAccessView(binding, hResourceView);
    }
  }
}

void plRenderContext::ApplySamplerBindings(const plGALShader* pShader)
{
  const auto& bindings = pShader->GetBindingMapping();
  for (const plShaderResourceBinding& binding : bindings)
  {
    auto type = plGALShaderResourceType::MakeFromShaderDescriptorType(binding.m_DescriptorType);
    if (type == plGALShaderResourceType::Sampler)
    {
      const plUInt64 uiResourceHash = binding.m_sName.GetHash();

      plGALSamplerStateHandle hSamplerState;
      if (!m_BoundSamplers.TryGetValue(uiResourceHash, hSamplerState))
      {
        hSamplerState = GetDefaultSamplerState(plDefaultSamplerFlags::LinearFiltering); // Bind a default state to avoid DX11 errors.
      }

      m_pGALCommandEncoder->SetSamplerState(binding, hSamplerState);
    }
  }
}

void plRenderContext::ApplyBufferBindings(const plGALShader* pShader)
{
  const auto& bindings = pShader->GetBindingMapping();
  for (const plShaderResourceBinding& binding : bindings)
  {
    if (binding.m_DescriptorType >= plGALShaderDescriptorType::TexelBuffer && binding.m_DescriptorType >= plGALShaderDescriptorType::StructuredBuffer)
    {
      const plUInt64 uiResourceHash = binding.m_sName.GetHash();

      plGALResourceViewHandle hResourceView;
      m_BoundBuffer.TryGetValue(uiResourceHash, hResourceView);

      m_pGALCommandEncoder->SetResourceView(binding, hResourceView);
    }
  }
}

void plRenderContext::SetDefaultTextureFilter(plTextureFilterSetting::Enum filter)
{
  PLASMA_ASSERT_DEBUG(
    filter >= plTextureFilterSetting::FixedBilinear && filter <= plTextureFilterSetting::FixedAnisotropic16x, "Invalid default texture filter");
  filter = plMath::Clamp(filter, plTextureFilterSetting::FixedBilinear, plTextureFilterSetting::FixedAnisotropic16x);

  if (m_DefaultTextureFilter == filter)
    return;

  m_DefaultTextureFilter = filter;
}

plTextureFilterSetting::Enum plRenderContext::GetSpecificTextureFilter(plTextureFilterSetting::Enum configuration) const
{
  if (configuration >= plTextureFilterSetting::FixedNearest && configuration <= plTextureFilterSetting::FixedAnisotropic16x)
    return configuration;

  int iFilter = m_DefaultTextureFilter;

  switch (configuration)
  {
    case plTextureFilterSetting::LowestQuality:
      iFilter -= 2;
      break;
    case plTextureFilterSetting::LowQuality:
      iFilter -= 1;
      break;
    case plTextureFilterSetting::HighQuality:
      iFilter += 1;
      break;
    case plTextureFilterSetting::HighestQuality:
      iFilter += 2;
      break;
    default:
      break;
  }

  iFilter = plMath::Clamp<int>(iFilter, plTextureFilterSetting::FixedBilinear, plTextureFilterSetting::FixedAnisotropic16x);

  return (plTextureFilterSetting::Enum)iFilter;
}

void plRenderContext::SetAllowAsyncShaderLoading(bool bAllow)
{
  m_bAllowAsyncShaderLoading = bAllow;
}

bool plRenderContext::GetAllowAsyncShaderLoading()
{
  return m_bAllowAsyncShaderLoading;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_RenderContext_Implementation_RenderContext);
