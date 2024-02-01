#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plFrameDataProviderBase, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

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



PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_FrameDataProvider);
