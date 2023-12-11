#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRenderPipelinePass, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_ACCESSOR_PROPERTY("Name", GetName, SetName),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Grape))
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRenderPipelinePass::plRenderPipelinePass(const char* szName, bool bIsStereoAware)
  : m_bIsStereoAware(bIsStereoAware)

{
  m_sName.Assign(szName);
}

plRenderPipelinePass::~plRenderPipelinePass() = default;

void plRenderPipelinePass::SetName(const char* szName)
{
  if (!plStringUtils::IsNullOrEmpty(szName))
  {
    m_sName.Assign(szName);
  }
}

const char* plRenderPipelinePass::GetName() const
{
  return m_sName.GetData();
}

void plRenderPipelinePass::InitRenderPipelinePass(const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) {}

void plRenderPipelinePass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) {}

void plRenderPipelinePass::ReadBackProperties(plView* pView) {}

plResult plRenderPipelinePass::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream << m_bActive;
  inout_stream << m_sName;
  return PLASMA_SUCCESS;
}

plResult plRenderPipelinePass::Deserialize(plStreamReader& inout_stream)
{
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PLASMA_ASSERT_DEBUG(uiVersion == 1, "Unknown version encountered");

  inout_stream >> m_bActive;
  inout_stream >> m_sName;
  return PLASMA_SUCCESS;
}

void plRenderPipelinePass::RenderDataWithCategory(const plRenderViewContext& renderViewContext, plRenderData::Category category, plRenderDataBatch::Filter filter)
{
  PLASMA_PROFILE_AND_MARKER(renderViewContext.m_pRenderContext->GetCommandEncoder(), plRenderData::GetCategoryName(category));

  auto batchList = m_pPipeline->GetRenderDataBatchesWithCategory(category, filter);
  const plUInt32 uiBatchCount = batchList.GetBatchCount();
  for (plUInt32 i = 0; i < uiBatchCount; ++i)
  {
    const plRenderDataBatch& batch = batchList.GetBatch(i);

    if (const plRenderData* pRenderData = batch.GetFirstData<plRenderData>())
    {
      const plRTTI* pType = pRenderData->GetDynamicRTTI();

      if (const plRenderer* pRenderer = plRenderData::GetCategoryRenderer(category, pType))
      {
        pRenderer->RenderBatch(renderViewContext, this, batch);
      }
    }
  }
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelinePass);