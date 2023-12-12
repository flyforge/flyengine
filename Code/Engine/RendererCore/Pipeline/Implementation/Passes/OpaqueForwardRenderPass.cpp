#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/OpaqueForwardRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plOpaqueForwardRenderPass, 1, plRTTIDefaultAllocator<plOpaqueForwardRenderPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("SSAO", m_PinSSAO),
    PLASMA_MEMBER_PROPERTY("WriteDepth", m_bWriteDepth)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plOpaqueForwardRenderPass::plOpaqueForwardRenderPass(const char* szName)
  : plForwardRenderPass(szName)
  , m_bWriteDepth(true)
{
  m_hWhiteTexture = plResourceManager::LoadResource<plTexture2DResource>("White.color");
}

plOpaqueForwardRenderPass::~plOpaqueForwardRenderPass() {}

bool plOpaqueForwardRenderPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  if (!SUPER::GetRenderTargetDescriptions(view, inputs, outputs))
  {
    return false;
  }

  if (inputs[m_PinSSAO.m_uiInputIndex])
  {
    if (inputs[m_PinSSAO.m_uiInputIndex]->m_uiWidth != inputs[m_PinColor.m_uiInputIndex]->m_uiWidth ||
        inputs[m_PinSSAO.m_uiInputIndex]->m_uiHeight != inputs[m_PinColor.m_uiInputIndex]->m_uiHeight)
    {
      plLog::Warning("Expected same resolution for SSAO and color input to pass '{0}'!", GetName());
    }

    if (m_ShadingQuality == plForwardRenderShadingQuality::Simplified)
    {
      plLog::Warning("SSAO input will be ignored for pass '{0}' since simplified shading is activated.", GetName());
    }
  }

  return true;
}

void plOpaqueForwardRenderPass::SetupResources(plGALPass* pGALPass, const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  SUPER::SetupResources(pGALPass, renderViewContext, inputs, outputs);

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // SSAO texture
  if (m_ShadingQuality == plForwardRenderShadingQuality::Normal)
  {
    if (inputs[m_PinSSAO.m_uiInputIndex])
    {
      plGALResourceViewHandle ssaoResourceViewHandle = pDevice->GetDefaultResourceView(inputs[m_PinSSAO.m_uiInputIndex]->m_TextureHandle);
      renderViewContext.m_pRenderContext->BindTexture2D("SSAOTexture", ssaoResourceViewHandle);
    }
    else
    {
      renderViewContext.m_pRenderContext->BindTexture2D("SSAOTexture", m_hWhiteTexture, plResourceAcquireMode::BlockTillLoaded);
    }
  }
}

void plOpaqueForwardRenderPass::SetupPermutationVars(const plRenderViewContext& renderViewContext)
{
  SUPER::SetupPermutationVars(renderViewContext);

  if (m_bWriteDepth)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "TRUE");
  }
  else
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "FALSE");
  }
}

void plOpaqueForwardRenderPass::RenderObjects(const plRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitOpaque);
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitMasked);
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_OpaqueForwardRenderPass);
