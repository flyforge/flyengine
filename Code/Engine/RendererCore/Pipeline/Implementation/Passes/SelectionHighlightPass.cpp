#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/SelectionHighlightPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/SelectionHighlightConstants.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSelectionHighlightPass, 1, plRTTIDefaultAllocator<plSelectionHighlightPass>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Color", m_PinColor),
    PL_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),

    PL_MEMBER_PROPERTY("HighlightColor", m_HighlightColor)->AddAttributes(new plDefaultValueAttribute(plColorScheme::LightUI(plColorScheme::Yellow))),
    PL_MEMBER_PROPERTY("OverlayOpacity", m_fOverlayOpacity)->AddAttributes(new plDefaultValueAttribute(0.1f))
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSelectionHighlightPass::plSelectionHighlightPass(const char* szName)
  : plRenderPipelinePass(szName, true)
{
  // Load shader.
  m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/SelectionHighlight.plShader");
  PL_ASSERT_DEV(m_hShader.IsValid(), "Could not load selection highlight shader!");

  m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plSelectionHighlightConstants>();
}

plSelectionHighlightPass::~plSelectionHighlightPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool plSelectionHighlightPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
    return true;
  }

  return false;
}

void plSelectionHighlightPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  auto pColorOutput = outputs[m_PinColor.m_uiOutputIndex];
  if (pColorOutput == nullptr)
  {
    return;
  }

  auto pDepthInput = inputs[m_PinDepthStencil.m_uiInputIndex];
  if (pDepthInput == nullptr)
  {
    return;
  }

  plRenderDataBatchList renderDataBatchList = GetPipeline()->GetRenderDataBatchesWithCategory(plDefaultRenderDataCategories::Selection);
  if (renderDataBatchList.GetBatchCount() == 0)
  {
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plGALTextureHandle hDepthTexture;

  // render all selection objects to depth target only
  {
    plUInt32 uiWidth = pColorOutput->m_Desc.m_uiWidth;
    plUInt32 uiHeight = pColorOutput->m_Desc.m_uiHeight;
    plGALMSAASampleCount::Enum sampleCount = pColorOutput->m_Desc.m_SampleCount;
    plUInt32 uiSliceCount = pColorOutput->m_Desc.m_uiArraySize;

    hDepthTexture = plGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, plGALResourceFormat::D24S8, sampleCount, uiSliceCount);

    plGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(hDepthTexture));
    renderingSetup.m_bClearDepth = true;
    renderingSetup.m_bClearStencil = true;

    auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");

    RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::Selection);
  }

  // reconstruct selection overlay from depth target
  {
    auto constants = plRenderContext::GetConstantBufferData<plSelectionHighlightConstants>(m_hConstantBuffer);
    constants->HighlightColor = m_HighlightColor;
    constants->OverlayOpacity = m_fOverlayOpacity;

    plGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));

    auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindConstantBuffer("plSelectionHighlightConstants", m_hConstantBuffer);
    renderViewContext.m_pRenderContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("SelectionDepthTexture", pDevice->GetDefaultResourceView(hDepthTexture));
    renderViewContext.m_pRenderContext->BindTexture2D("SceneDepthTexture", pDevice->GetDefaultResourceView(pDepthInput->m_TextureHandle));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    plGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hDepthTexture);
  }
}

plResult plSelectionHighlightPass::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_HighlightColor;
  inout_stream << m_fOverlayOpacity;
  return PL_SUCCESS;
}

plResult plSelectionHighlightPass::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_HighlightColor;
  inout_stream >> m_fOverlayOpacity;
  return PL_SUCCESS;
}

PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SelectionHighlightPass);
