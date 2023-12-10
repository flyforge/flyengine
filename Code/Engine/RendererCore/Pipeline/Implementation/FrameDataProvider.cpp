#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plFrameDataProviderBase, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plFrameDataProviderBase::plFrameDataProviderBase()

  = default;

void* plFrameDataProviderBase::GetData(const plRenderViewContext& renderViewContext)
{
  if (m_pData == nullptr || m_uiLastUpdateFrame != plRenderWorld::GetFrameCounter())
  {
    m_pData = UpdateData(renderViewContext, m_pOwnerPipeline->GetRenderData());

    m_uiLastUpdateFrame = plRenderWorld::GetFrameCounter();
  }

  return m_pData;
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_FrameDataProvider);
