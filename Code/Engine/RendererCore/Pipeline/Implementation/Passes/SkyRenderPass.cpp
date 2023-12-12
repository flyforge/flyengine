#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/SkyRenderPass.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSkyRenderPass, 1, plRTTIDefaultAllocator<plSkyRenderPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("WriteDepth", m_bWriteDepth)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plSkyRenderPass::plSkyRenderPass(const char* szName)
  : plForwardRenderPass(szName)
{
}

plSkyRenderPass::~plSkyRenderPass() {}

void plSkyRenderPass::SetupPermutationVars(const plRenderViewContext& renderViewContext)
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

#if PLASMA_ENABLED(PLASMA_GAMEOBJECT_VELOCITY)
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("GAMEOBJECT_VELOCITY", "TRUE");
#else
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("GAMEOBJECT_VELOCITY", "FALSE");
#endif
}

void plSkyRenderPass::RenderObjects(const plRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::Sky);
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::PostSky);
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SkyRenderPass);
