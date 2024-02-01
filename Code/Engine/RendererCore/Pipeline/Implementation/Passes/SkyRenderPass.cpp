#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/SkyRenderPass.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSkyRenderPass, 1, plRTTIDefaultAllocator<plSkyRenderPass>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plSkyRenderPass::plSkyRenderPass(const char* szName)
  : plForwardRenderPass(szName)
{
}

plSkyRenderPass::~plSkyRenderPass() = default;

void plSkyRenderPass::RenderObjects(const plRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::Sky);
}



PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SkyRenderPass);
