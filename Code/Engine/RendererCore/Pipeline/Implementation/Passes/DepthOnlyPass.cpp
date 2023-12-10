#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/DepthOnlyPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDepthOnlyPass, 1, plRTTIDefaultAllocator<plDepthOnlyPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plDepthOnlyPass::plDepthOnlyPass(const char* szName)
  : plRenderPipelinePass(szName, true)
{
}

plDepthOnlyPass::~plDepthOnlyPass() = default;

bool plDepthOnlyPass::GetRenderTargetDescriptions(
  const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // DepthStencil
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    plLog::Error("No depth stencil input connected to pass '{0}'!", GetName());
    return false;
  }

  return true;
}

void plDepthOnlyPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs,
  const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // Setup render target
  plGALRenderingSetup renderingSetup;
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_NORMAL");

  // Render
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitOpaque);
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitMasked);
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_DepthOnlyPass);
