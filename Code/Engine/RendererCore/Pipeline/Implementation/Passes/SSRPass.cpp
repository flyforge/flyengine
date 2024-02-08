#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/SSRPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include "../../../../../../Data/Base/Shaders/Pipeline/CopyConstants.h"
#include "../../../../../../Data/Base/Shaders/Common/PostprocessCommon.h"
#include "../../../../../../Data/Base/Shaders/Pipeline/SSRConstants.h"

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSSRPass, 1, plRTTIDefaultAllocator<plSSRPass>)
{
  PL_BEGIN_PROPERTIES
  {
      PL_MEMBER_PROPERTY("LitOutput", m_PinInputColor),
      PL_MEMBER_PROPERTY("Material", m_PinInputMaterial),
      PL_MEMBER_PROPERTY("Velocity", m_PinInputVelocity),
      PL_MEMBER_PROPERTY("DepthStencil", m_PinInputDepth),
      PL_MEMBER_PROPERTY("Output", m_PinOutput),

      PL_MEMBER_PROPERTY("MaxDist", m_fMaxDist)->AddAttributes(new plDefaultValueAttribute(15.0f)),
      PL_MEMBER_PROPERTY("Resolution", m_fResolution)->AddAttributes(new plDefaultValueAttribute(0.1f), new plClampValueAttribute(0, 1)),
      PL_MEMBER_PROPERTY("Steps", m_iSteps)->AddAttributes(new plDefaultValueAttribute(10)),
      PL_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new plDefaultValueAttribute(0.1f))
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSSRPass::plSSRPass()
  : plRenderPipelinePass("SSRPass", true)
  , m_fMaxDist(15.0f)
  , m_fResolution(0.1f)
  , m_iSteps(10)
  , m_fThickness(0.1f)
{

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  {
    m_hShaderTileMinMaxRoughnessHorizontal = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/SSR/SSRTileMinMaxRoughessHorizontal.plShader");
    PL_ASSERT_DEV(m_hShaderTileMinMaxRoughnessHorizontal.IsValid(), "SSR: Could not load min max roughness horizontal shader!");
  }
  {
    m_hShaderTileMinMaxRoughnessVerticle = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline//SSR/SSRTileMinMaxRoughessVertical.plShader");
    PL_ASSERT_DEV(m_hShaderTileMinMaxRoughnessVerticle.IsValid(), "SSR: Could not load min max roughness vertical shader!");
  }
  {
    m_hShaderDepthHierarchy = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline//SSR/SSRDepthHierarchy.plShader");
    PL_ASSERT_DEV(m_hShaderDepthHierarchy.IsValid(), "SSR: Could not load depth hierarchy shader!");
  }
  {
    m_hShaderSSRTrace = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline//SSR/SSRTracePass.plShader");
    PL_ASSERT_DEV(m_hShaderSSRTrace.IsValid(), "SSR: Could not load trace shader shader!");
  }
  {
    m_hSSRConstantBuffer = plRenderContext::CreateConstantBufferStorage<plSSRConstants>();
    m_hPostProcessConstantBuffer = plRenderContext::CreateConstantBufferStorage<plPostProcessingConstants>();
  }
  {
    m_hBlueNoiseTexture = plResourceManager::LoadResource<plTexture2DResource>("Textures/BlueNoise.dds");
  }
}

plSSRPass::~plSSRPass()
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  pDevice->DestroyBuffer(m_hTilesTracingEarlyexitBuffer);
  PL_DELETE_ARRAY(plAlignedAllocatorWrapper::GetAllocator(), m_TilesTracingEarlyexit);
  pDevice->DestroyBuffer(m_hTilesTracingCheapBuffer);
  PL_DELETE_ARRAY(plAlignedAllocatorWrapper::GetAllocator(), m_TilesTracingCheap);
  pDevice->DestroyBuffer(m_hTilesTracingExpensiveBuffer);
  PL_DELETE_ARRAY(plAlignedAllocatorWrapper::GetAllocator(), m_TilesTracingExpensive);

  plRenderContext::DeleteConstantBufferStorage(m_hSSRConstantBuffer);
  m_hSSRConstantBuffer.Invalidate();

  plRenderContext::DeleteConstantBufferStorage(m_hPostProcessConstantBuffer);
  m_hPostProcessConstantBuffer.Invalidate();
}

plResult plSSRPass::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fMaxDist;
  inout_stream << m_fResolution;
  inout_stream << m_iSteps;
  inout_stream << m_fThickness;
  return PL_SUCCESS;
}

plResult plSSRPass::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fMaxDist;
  inout_stream >> m_fResolution;
  inout_stream >> m_iSteps;
  inout_stream >> m_fThickness;
  return PL_SUCCESS;
}

bool plSSRPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInputColor.m_uiInputIndex])
  {
    if (!inputs[m_PinInputColor.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' Color input must allow shader resource view.", GetName());
      return false;
    }

    plGALTextureCreationDescription desc = *inputs[m_PinInputColor.m_uiInputIndex];
    desc.m_bAllowUAV = true;
    desc.m_bCreateRenderTarget = true;

    outputs[m_PinOutput.m_uiOutputIndex] = std::move(desc);
  }
  else
  {
    plLog::Error("No Color input connected to '{0}'!", GetName());
    return false;
  }

  // Depth
  if (inputs[m_PinInputDepth.m_uiInputIndex])
  {
    if (!inputs[m_PinInputDepth.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' Depth input must allow shader resource view.", GetName());
      return false;
    }
  }
  else
  {
    plLog::Error("No Depth input connected to '{0}'!", GetName());
    return false;
  }

  // Material
  if (inputs[m_PinInputMaterial.m_uiInputIndex])
  {
    if (!inputs[m_PinInputMaterial.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' material input must allow shader resource view.", GetName());
      return false;
    }
  }
  else
  {
    plLog::Error("No material input connected to pass '{0}'.", GetName());
    return false;
  }

  // Velocity
  if (inputs[m_PinInputVelocity.m_uiInputIndex])
  {
    if (!inputs[m_PinInputVelocity.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' velocity input must allow shader resource view.", GetName());
      return false;
    }
  }
  else
  {
    plLog::Error("No velocity input connected to pass '{0}'.", GetName());
    return false;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plGALTextureCreationDescription output = outputs[m_PinOutput.m_uiOutputIndex];

  m_MinMaxRoughnessWidth = (output.m_uiWidth + SSR_TILESIZE -1) / SSR_TILESIZE;
  m_MinMaxRoughnessHeight = (output.m_uiHeight + SSR_TILESIZE -1) / SSR_TILESIZE;
  {
    plGALTextureCreationDescription desc = *inputs[m_PinInputColor.m_uiInputIndex];
    desc.m_Type = plGALTextureType::Texture2D;
    desc.m_uiWidth = m_MinMaxRoughnessWidth;
    desc.m_uiHeight = m_MinMaxRoughnessHeight;
    desc.m_Format = plGALResourceFormat::RGFloat;
    desc.m_bAllowShaderResourceView = true;
    desc.m_bAllowUAV = true;
    m_hTileMinMaxRoughness = pDevice->CreateTexture(desc);


    desc.m_uiHeight = output.m_uiHeight;
    m_hTextureTileMinMaxRoughnessHorizontal = pDevice->CreateTexture(desc);
  }

  {
    //because these are all created together can assume 1 valid = all valid
    if(!m_hTilesTracingEarlyexitBuffer.IsInvalidated())
    {
      pDevice->DestroyBuffer(m_hTilesTracingEarlyexitBuffer);
      PL_DELETE_ARRAY(plAlignedAllocatorWrapper::GetAllocator(), m_TilesTracingEarlyexit);
      pDevice->DestroyBuffer(m_hTilesTracingCheapBuffer);
      PL_DELETE_ARRAY(plAlignedAllocatorWrapper::GetAllocator(), m_TilesTracingCheap);
      pDevice->DestroyBuffer(m_hTilesTracingExpensiveBuffer);
      PL_DELETE_ARRAY(plAlignedAllocatorWrapper::GetAllocator(), m_TilesTracingExpensive);
    }

    plGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(plUInt32);
    desc.m_uiTotalSize = m_MinMaxRoughnessWidth * m_MinMaxRoughnessHeight * sizeof(plUInt32);
    desc.m_BufferType = plGALBufferType::Generic;
    desc.m_bAllowUAV = true;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_TilesTracingEarlyexit = PL_NEW_ARRAY(plAlignedAllocatorWrapper::GetAllocator(), plUInt32, 1);
    m_TilesTracingEarlyexit[0] = 0;
    m_TilesTracingCheap = PL_NEW_ARRAY(plAlignedAllocatorWrapper::GetAllocator(), plUInt32, 1);
    m_TilesTracingCheap[0] = 0;
    m_TilesTracingExpensive = PL_NEW_ARRAY(plAlignedAllocatorWrapper::GetAllocator(), plUInt32, 1);
    m_TilesTracingExpensive[0] = 0;

    m_hTilesTracingEarlyexitBuffer = pDevice->CreateBuffer(desc, m_TilesTracingEarlyexit.ToByteArray());
    m_hTilesTracingCheapBuffer = pDevice->CreateBuffer(desc, m_TilesTracingCheap.ToByteArray());
    m_hTilesTracingExpensiveBuffer = pDevice->CreateBuffer(desc, m_TilesTracingExpensive.ToByteArray());
  }

  {
    plGALTextureCreationDescription desc = *inputs[m_PinInputColor.m_uiInputIndex];
    desc.m_Type = plGALTextureType::Texture2D;
    desc.m_uiWidth = (uint32_t)std::pow(2.0f, 1.0f + std::floor(std::log2((float)output.m_uiWidth / 2)));
    desc.m_uiHeight = (uint32_t)std::pow(2.0f, 1.0f + std::floor(std::log2((float)output.m_uiHeight / 2)));
    desc.m_uiMipLevelCount = 1 + (uint32_t)std::floor(std::log2f(std::max((float)desc.m_uiWidth, (float)desc.m_uiHeight)));
    desc.m_Format = plGALResourceFormat::RGFloat;
    desc.m_bAllowShaderResourceView = true;
    desc.m_bAllowUAV = true;
    m_hDepthHierarchy = pDevice->CreateTexture(desc);
    m_hDepthHierarchyTmp = pDevice->CreateTexture(desc);
  }

  {
    plGALTextureCreationDescription desc = *inputs[m_PinInputColor.m_uiInputIndex];
    desc.m_Type = plGALTextureType::Texture2D;
    desc.m_Format = plGALResourceFormat::RGBAHalf;
    desc.m_uiWidth = desc.m_uiWidth / 2;
    desc.m_uiHeight = desc.m_uiHeight / 2;
    desc.m_bAllowShaderResourceView = true;
    desc.m_bAllowUAV = true;
    m_hIndirectSpecular = pDevice->CreateTexture(desc);
    m_hDirectionPDF = pDevice->CreateTexture(desc);
    desc.m_Format = plGALResourceFormat::RHalf;
    m_hRayLength = pDevice->CreateTexture(desc);
  }
  return true;
}

void plSSRPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInputColor = inputs[m_PinInputColor.m_uiInputIndex];
  const auto* const pInputVelocity = inputs[m_PinInputVelocity.m_uiInputIndex];
  const auto* const pInputDepth = inputs[m_PinInputDepth.m_uiInputIndex];
  const auto* const pInputMaterial = inputs[m_PinInputMaterial.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInputColor == nullptr || pInputVelocity == nullptr || pInputDepth == nullptr || pOutput == nullptr || pInputMaterial == nullptr)
  {
    return;
  }


  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plResourceManager::ForceLoadResourceNow(m_hShaderTileMinMaxRoughnessHorizontal);
  plResourceManager::ForceLoadResourceNow(m_hShaderTileMinMaxRoughnessVerticle);
  plResourceManager::ForceLoadResourceNow(m_hShaderDepthHierarchy);

  const bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  // SSR Clear textures
  {
    plGALRenderingSetup renderingSetup;
    renderingSetup.m_ClearColor = plColor::Black;
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
    renderingSetup.m_bClearDepth = true;
    renderingSetup.m_bClearStencil = true;

    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(m_hTextureTileMinMaxRoughnessHorizontal));
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(1, pDevice->GetDefaultRenderTargetView(m_hTileMinMaxRoughness));
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(2, pDevice->GetDefaultRenderTargetView(m_hDepthHierarchy));
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(3, pDevice->GetDefaultRenderTargetView(m_hIndirectSpecular));
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(4, pDevice->GetDefaultRenderTargetView(m_hDirectionPDF));

    auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, "SSR Clear UAV");
  }

  plGALPass* pPass = pDevice->BeginPass(GetName());
  {
    UpdateSSRConstantBuffer();

    // TODO: Need a better solution. Currently need to do this due to issuer in buffer updates
    {
      plGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(plUInt32);
      desc.m_uiTotalSize = sizeof(plUInt32) * 9;
      desc.m_BufferType = plGALBufferType::Generic;
      desc.m_bAllowUAV = true;
      //desc.m_bUseAsStructuredBuffer = true;
      desc.m_bUseForIndirectArguments = true;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_TileStatistics = PL_NEW_ARRAY(plAlignedAllocatorWrapper::GetAllocator(), plPostprocessTileStatistics, 9);

      m_TileStatistics[0].Dispatch_Earlyexit_ThreadGroupCountX = 0;// shader atomic
      m_TileStatistics[0].Dispatch_Earlyexit_ThreadGroupCountY = 1;
      m_TileStatistics[0].Dispatch_Earlyexit_ThreadGroupCountZ = 1;
      m_TileStatistics[0].Dispatch_Cheap_ThreadGroupCountX = 0; // shader atomic
      m_TileStatistics[0].Dispatch_Cheap_ThreadGroupCountY = 1;
      m_TileStatistics[0].Dispatch_Cheap_ThreadGroupCountZ = 1;
      m_TileStatistics[0].Dispatch_Expensive_ThreadGroupCountX = 0; // shader atomic
      m_TileStatistics[0].Dispatch_Expensive_ThreadGroupCountY = 1;
      m_TileStatistics[0].Dispatch_Expensive_ThreadGroupCountZ = 1;

      m_hTileStatisticsBuffer = pDevice->CreateBuffer(desc, m_TileStatistics.ToByteArray());
    }

    // SSR Compute tile classification (horizontal)
    {
      auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "SSR Tile Classification - Horizontal");
      renderViewContext.m_pRenderContext->BindShader(m_hShaderTileMinMaxRoughnessHorizontal);

      plGALUnorderedAccessViewHandle hOutput;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hTextureTileMinMaxRoughnessHorizontal;
        desc.m_uiMipLevelToUse = 0;
        hOutput = pDevice->CreateUnorderedAccessView(desc);
      }

      renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
      renderViewContext.m_pRenderContext->BindTexture2D("MaterialParamsBuffer", pDevice->GetDefaultResourceView(pInputMaterial->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));

      const plUInt32 uiWidth = (pOutput->m_Desc.m_uiWidth + SSR_TILESIZE -1) / SSR_TILESIZE;
      const plUInt32 uiHeight = pOutput->m_Desc.m_uiHeight;

      const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;
      const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;

      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
    }
    // SSR Compute tile classification (verticle)
    {
      auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "SSR Tile Classification - Verticle");
      renderViewContext.m_pRenderContext->BindShader(m_hShaderTileMinMaxRoughnessVerticle);

      plGALUnorderedAccessViewHandle hTileStatistics;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_OverrideViewFormat = plGALResourceFormat::RUInt;
        desc.m_hBuffer = m_hTileStatisticsBuffer;
        desc.m_uiNumElements = 9;
        desc.m_uiFirstElement = 0;
        hTileStatistics = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("TileTracingStatistics", hTileStatistics);

      plGALUnorderedAccessViewHandle hTilesTracingEarlyexit;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_OverrideViewFormat = plGALResourceFormat::RUInt;
        desc.m_hBuffer = m_hTilesTracingEarlyexitBuffer;
        desc.m_uiNumElements = m_MinMaxRoughnessWidth * m_MinMaxRoughnessHeight;
        desc.m_uiFirstElement = 0;
        hTilesTracingEarlyexit = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("TilesTracingEarlyexit", hTilesTracingEarlyexit);

      plGALUnorderedAccessViewHandle hTilesTracingCheap;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_OverrideViewFormat = plGALResourceFormat::RUInt;
        desc.m_hBuffer = m_hTilesTracingCheapBuffer;
        desc.m_uiNumElements = m_MinMaxRoughnessWidth * m_MinMaxRoughnessHeight;
        desc.m_uiFirstElement = 0;
        hTilesTracingCheap = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("TilesTracingCheap", hTilesTracingCheap);

      plGALUnorderedAccessViewHandle hTilesTracingExpensive;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_OverrideViewFormat = plGALResourceFormat::RUInt;
        desc.m_hBuffer = m_hTilesTracingExpensiveBuffer;
        desc.m_uiNumElements = m_MinMaxRoughnessWidth * m_MinMaxRoughnessHeight;
        desc.m_uiFirstElement = 0;
        hTilesTracingExpensive = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("TilesTracingExpensive", hTilesTracingExpensive);

      plGALUnorderedAccessViewHandle hOutput;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hTileMinMaxRoughness;
        desc.m_uiMipLevelToUse = 0;
        hOutput = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);

      renderViewContext.m_pRenderContext->BindTexture2D("TileMinMaxRoughnessHorizontal", pDevice->GetDefaultResourceView(m_hTextureTileMinMaxRoughnessHorizontal));
      renderViewContext.m_pRenderContext->BindConstantBuffer("plSSRConstants", m_hSSRConstantBuffer);

      const plUInt32 uiDispatchX = (m_MinMaxRoughnessWidth + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;
      const plUInt32 uiDispatchY = (m_MinMaxRoughnessHeight + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;

      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
    }

    plUInt32 hierarchyWidth = (plUInt32)std::pow(2.0f, 1.0f + std::floor(std::log2((float)pOutput->m_Desc.m_uiWidth / 2)));
    plUInt32 hierarchyHeight = (plUInt32)std::pow(2.0f, 1.0f + std::floor(std::log2((float)pOutput->m_Desc.m_uiHeight / 2)));
    // Depth hierarchy
    {
      auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "SSR Depth hierarchy");
      renderViewContext.m_pRenderContext->BindShader(m_hShaderDepthHierarchy);


      {
        plGALUnorderedAccessViewHandle hOutput;
        {
          plGALUnorderedAccessViewCreationDescription desc;
          desc.m_hTexture = m_hDepthHierarchy;
          desc.m_uiMipLevelToUse = 0;
          hOutput = pDevice->CreateUnorderedAccessView(desc);
        }

        renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
        renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));

        auto* postProcessConst = plRenderContext::GetConstantBufferData<plPostProcessingConstants>(m_hPostProcessConstantBuffer);
        postProcessConst->params0.x = hierarchyWidth;
        postProcessConst->params0.y = hierarchyHeight;
        postProcessConst->params0.z = 1.0;

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SSR_DEPTH_MODE", "SSR_DEPTH_MODE_FIRST_PASS");

        const plUInt32 uiDispatchX = plMath::Max(1u, (plUInt32)hierarchyWidth / POSTPROCESS_BLOCKSIZE);
        const plUInt32 uiDispatchY = plMath::Max(1u, (plUInt32)hierarchyHeight / POSTPROCESS_BLOCKSIZE);

        renderViewContext.m_pRenderContext->BindConstantBuffer("plPostProcessingConstants", m_hPostProcessConstantBuffer);

        renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();

        pCommandEncoder->CopyTexture(m_hDepthHierarchyTmp, m_hDepthHierarchy);
      }
      {
        plUInt32 mipCount = 1 + (uint32_t)std::floor(std::log2f(std::max((float)hierarchyWidth, (float)hierarchyHeight)));
        for (plUInt32 i = 1; i < mipCount; i++)
        {
          plGALUnorderedAccessViewHandle hOutput;
          {
            plGALUnorderedAccessViewCreationDescription desc;
            desc.m_hTexture = m_hDepthHierarchy;
            desc.m_uiMipLevelToUse = i;
            hOutput = pDevice->CreateUnorderedAccessView(desc);
          }

          hierarchyWidth /= 2;
          hierarchyHeight /= 2;

          hierarchyWidth = plMath::Max(1u, hierarchyWidth);
          hierarchyHeight = plMath::Max(1u, hierarchyHeight);

          renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
          renderViewContext.m_pRenderContext->BindTexture2D("Input", pDevice->GetDefaultResourceView(m_hDepthHierarchyTmp));

          auto* postProcessConst = plRenderContext::GetConstantBufferData<plPostProcessingConstants>(m_hPostProcessConstantBuffer);
          postProcessConst->params0.x = hierarchyWidth;
          postProcessConst->params0.y = hierarchyHeight;
          postProcessConst->params0.z = 1.0;

          renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SSR_DEPTH_MODE", "SSR_DEPTH_MODE_SECOND_PASS");

          const plUInt32 uiDispatchX = plMath::Max(1u, (plUInt32)hierarchyWidth / POSTPROCESS_BLOCKSIZE);
          const plUInt32 uiDispatchY = plMath::Max(1u, (plUInt32)hierarchyHeight / POSTPROCESS_BLOCKSIZE);

          renderViewContext.m_pRenderContext->BindConstantBuffer("plPostProcessingConstants", m_hPostProcessConstantBuffer);

          renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();

          pCommandEncoder->CopyTexture(m_hDepthHierarchyTmp, m_hDepthHierarchy);
        }
      }
    }
    auto* postProcessConst = plRenderContext::GetConstantBufferData<plPostProcessingConstants>(m_hPostProcessConstantBuffer);
    postProcessConst->resolution.x = pOutput->m_Desc.m_uiWidth / 2;
    postProcessConst->resolution.y = pOutput->m_Desc.m_uiHeight / 2;
    postProcessConst->resolution.z = 1.0 / postProcessConst->resolution.x;
    postProcessConst->resolution.w = 1.0 / postProcessConst->resolution.y;

    // Factor to scale ratio between hierarchy and trace pass
    postProcessConst->params1.x = (float)postProcessConst->resolution.x / (float)hierarchyWidth;
    postProcessConst->params1.y = (float)postProcessConst->resolution.y / (float)hierarchyHeight;
    postProcessConst->params1.z = 1.0f / postProcessConst->params1.x;
    postProcessConst->params1.w = 1.0f / postProcessConst->params1.y;

    // SSR Trace Pass
    {
      auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "SSR Raytrace pass");
      renderViewContext.m_pRenderContext->BindShader(m_hShaderSSRTrace);

      plGALUnorderedAccessViewHandle hOutputIndirectSpec;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = pOutput->m_TextureHandle;
        desc.m_uiMipLevelToUse = 0;
        hOutputIndirectSpec = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("OutputIndirectSpecular", hOutputIndirectSpec);

      plGALUnorderedAccessViewHandle hOutputDirectionPDF;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hDirectionPDF;
        desc.m_uiMipLevelToUse = 0;
        hOutputDirectionPDF = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("OutputRayDirectionPDF", hOutputDirectionPDF);

      plGALUnorderedAccessViewHandle hOutputRayLength;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hRayLength;
        desc.m_uiMipLevelToUse = 0;
        hOutputRayLength = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("OutputRayLengths", hOutputRayLength);

      renderViewContext.m_pRenderContext->BindTexture2D("Input", pDevice->GetDefaultResourceView(pInputColor->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("BlueNoise",m_hBlueNoiseTexture, plResourceAcquireMode::BlockTillLoaded);
      renderViewContext.m_pRenderContext->BindTexture2D("MaterialInput", pDevice->GetDefaultResourceView(pInputMaterial->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("VelocityInput", pDevice->GetDefaultResourceView(pInputVelocity->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("DepthInput", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindConstantBuffer("plPostProcessingConstants", m_hPostProcessConstantBuffer);

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SSR_TRACING_MODE", "SSR_TRACING_MODE_EARLYEXIT");
      renderViewContext.m_pRenderContext->BindBuffer("tiles", pDevice->GetDefaultResourceView(m_hTilesTracingEarlyexitBuffer));
      renderViewContext.m_pRenderContext->DispatchIndirect(m_hTileStatisticsBuffer,  0).IgnoreResult();

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SSR_TRACING_MODE", "SSR_TRACING_MODE_CHEAP");
      renderViewContext.m_pRenderContext->BindBuffer("tiles", pDevice->GetDefaultResourceView(m_hTilesTracingCheapBuffer));
      renderViewContext.m_pRenderContext->DispatchIndirect(m_hTileStatisticsBuffer,  sizeof(plUInt32) * 3).IgnoreResult();
      
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SSR_TRACING_MODE", "SSR_TRACING_MODE_EXPENSIVE");
      renderViewContext.m_pRenderContext->BindBuffer("tiles", pDevice->GetDefaultResourceView(m_hTilesTracingExpensiveBuffer));
      renderViewContext.m_pRenderContext->DispatchIndirect(m_hTileStatisticsBuffer,  sizeof(plUInt32) * 6).IgnoreResult();
    }

    // SSR Pass
    {
//      auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "SSR");
//
//      renderViewContext.m_pRenderContext->BindShader(m_hSSRShader);
//
//      plGALUnorderedAccessViewHandle hOutput;
//      {
//        plGALUnorderedAccessViewCreationDescription desc;
//        desc.m_hTexture = pOutput->m_TextureHandle;
//        desc.m_uiMipLevelToUse = 0;
//        hOutput = pDevice->CreateUnorderedAccessView(desc);
//      }
//
//      renderViewContext.m_pRenderContext->BindUAV("tex_uav", hOutput);
//      renderViewContext.m_pRenderContext->BindTexture2D("tex", pDevice->GetDefaultResourceView(pInputColor->m_TextureHandle));
//      renderViewContext.m_pRenderContext->BindTexture2D("tex_normal", pDevice->GetDefaultResourceView(pInputNormal->m_TextureHandle));
//      renderViewContext.m_pRenderContext->BindTexture2D("tex_velocity", pDevice->GetDefaultResourceView(pInputVelocity->m_TextureHandle));
//      renderViewContext.m_pRenderContext->BindTexture2D("tex_material", pDevice->GetDefaultResourceView(inputs[m_PinInputMaterial.m_uiInputIndex]->m_TextureHandle));
//      renderViewContext.m_pRenderContext->BindTexture2D("tex_depth", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));
//      renderViewContext.m_pRenderContext->BindConstantBuffer("plSSRConstants", m_hSSRConstantBuffer);
//
//      const plUInt32 uiWidth = pOutput->m_Desc.m_uiWidth;
//      const plUInt32 uiHeight = pOutput->m_Desc.m_uiHeight;
//
//      const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;
//      const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;
//
//      UpdateSSRConstantBuffer();
//
//      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
    }

    pDevice->EndPass(pPass);

    renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading);
 
    pDevice->DestroyBuffer(m_hTileStatisticsBuffer);
    PL_DELETE_ARRAY(plAlignedAllocatorWrapper::GetAllocator(), m_TileStatistics);
  }
}

void plSSRPass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInput = inputs[m_PinInputColor.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  const plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  const plGALTexture* pDest = pDevice->GetTexture(pOutput->m_TextureHandle);

  if (const plGALTexture* pSource = pDevice->GetTexture(pInput->m_TextureHandle); pDest->GetDescription().m_Format != pSource->GetDescription().m_Format)
  {
    // TODO: use a shader when the format doesn't match exactly

    plLog::Error("Copying textures of different formats is not implemented");
  }
  else
  {
    auto pCommandEncoder = plRenderContext::BeginPassAndComputeScope(renderViewContext, GetName());

    pCommandEncoder->CopyTexture(pOutput->m_TextureHandle, pInput->m_TextureHandle);
  }
}

void plSSRPass::UpdateSSRConstantBuffer() const
{
  auto* constants = plRenderContext::GetConstantBufferData<plSSRConstants>(m_hSSRConstantBuffer);
  constants->RoughnessCutoff = 0.8;
  constants->Frame = plRenderWorld::GetFrameCounter();
}

PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SSRPass);
