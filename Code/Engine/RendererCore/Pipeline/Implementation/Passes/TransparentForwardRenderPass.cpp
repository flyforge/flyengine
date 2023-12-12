#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/TransparentForwardRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTransparentForwardRenderPass, 1, plRTTIDefaultAllocator<plTransparentForwardRenderPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ResolvedDepth", m_PinResolvedDepth),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTransparentForwardRenderPass::plTransparentForwardRenderPass(const char* szName)
  : plForwardRenderPass(szName)
{
}

plTransparentForwardRenderPass::~plTransparentForwardRenderPass()
{
  if (!m_hSceneColorSamplerState.IsInvalidated())
  {
    plGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSceneColorSamplerState);
    m_hSceneColorSamplerState.Invalidate();
  }
}

void plTransparentForwardRenderPass::Execute(const plRenderViewContext& renderViewContext,
  const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinColor.m_uiInputIndex];
  if (pColorInput == nullptr)
  {
    return;
  }

  CreateSamplerState();

  plUInt32 uiWidth = pColorInput->m_Desc.m_uiWidth;
  plUInt32 uiHeight = pColorInput->m_Desc.m_uiHeight;

  plGALTextureCreationDescription desc;
  desc.SetAsRenderTarget(uiWidth, uiHeight, pColorInput->m_Desc.m_Format);
  desc.m_uiArraySize = pColorInput->m_Desc.m_uiArraySize;
  desc.m_uiMipLevelCount = 1;

  plGALTextureHandle hSceneColor = plGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plGALPass* pGALPass = pDevice->BeginPass(GetName());

  SetupResources(pGALPass, renderViewContext, inputs, outputs);
  SetupPermutationVars(renderViewContext);
  SetupLighting(renderViewContext);

  UpdateSceneColorTexture(renderViewContext, hSceneColor, pColorInput->m_TextureHandle);

  plGALResourceViewHandle colorResourceViewHandle = pDevice->GetDefaultResourceView(hSceneColor);
  renderViewContext.m_pRenderContext->BindTexture2D("SceneColor", colorResourceViewHandle);
  renderViewContext.m_pRenderContext->BindSamplerState("SceneColorSampler", m_hSceneColorSamplerState);

  RenderObjects(renderViewContext);

  renderViewContext.m_pRenderContext->EndRendering();
  pDevice->EndPass(pGALPass);

  plGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hSceneColor);
}

void plTransparentForwardRenderPass::SetupResources(plGALPass* pGALPass, const plRenderViewContext& renderViewContext,
  const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  SUPER::SetupResources(pGALPass, renderViewContext, inputs, outputs);

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  if (inputs[m_PinResolvedDepth.m_uiInputIndex])
  {
    plGALResourceViewHandle depthResourceViewHandle = pDevice->GetDefaultResourceView(inputs[m_PinResolvedDepth.m_uiInputIndex]->m_TextureHandle);
    renderViewContext.m_pRenderContext->BindTexture2D("SceneDepth", depthResourceViewHandle);
  }
}

void plTransparentForwardRenderPass::RenderObjects(const plRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitTransparent);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitForeground);
}

void plTransparentForwardRenderPass::UpdateSceneColorTexture(
  const plRenderViewContext& renderViewContext, plGALTextureHandle hSceneColorTexture, plGALTextureHandle hCurrentColorTexture)
{
  plGALTextureSubresource subresource;
  subresource.m_uiMipLevel = 0;
  subresource.m_uiArraySlice = 0;

  renderViewContext.m_pRenderContext->GetCommandEncoder()->ResolveTexture(hSceneColorTexture, subresource, hCurrentColorTexture, subresource);
}

void plTransparentForwardRenderPass::CreateSamplerState()
{
  if (m_hSceneColorSamplerState.IsInvalidated())
  {
    plGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = plGALTextureFilterMode::Linear;
    desc.m_MagFilter = plGALTextureFilterMode::Linear;
    desc.m_MipFilter = plGALTextureFilterMode::Linear;
    desc.m_AddressU = plImageAddressMode::Clamp;
    desc.m_AddressV = plImageAddressMode::Mirror;
    desc.m_AddressW = plImageAddressMode::Mirror;

    m_hSceneColorSamplerState = plGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TransparentForwardRenderPass);
