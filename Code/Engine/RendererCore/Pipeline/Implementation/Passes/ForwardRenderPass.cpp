#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/ClusteredDataProvider.h>
#include <RendererCore/Lights/SimplifiedDataProvider.h>
#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plForwardRenderPass, 3, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Color", m_PinColor),
    PL_MEMBER_PROPERTY("Velocity", m_PinVelocity),
    PL_MEMBER_PROPERTY("Material", m_PinMaterial),
    PL_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    PL_ENUM_MEMBER_PROPERTY("ShadingQuality", plForwardRenderShadingQuality, m_ShadingQuality)->AddAttributes(new plDefaultValueAttribute((int)plForwardRenderShadingQuality::Normal)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_ENUM(plForwardRenderShadingQuality, 1)
  PL_ENUM_CONSTANTS(plForwardRenderShadingQuality::Normal, plForwardRenderShadingQuality::Simplified)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

plForwardRenderPass::plForwardRenderPass(const char* szName)
  : plRenderPipelinePass(szName, true)
  , m_ShadingQuality(plForwardRenderShadingQuality::Normal)
{
}

plForwardRenderPass::~plForwardRenderPass() = default;

bool plForwardRenderPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }
  else
  {
    plLog::Error("No color input connected to pass '{0}'!", GetName());
    return false;
  }

  // Velocity
  if (inputs[m_PinVelocity.m_uiInputIndex])
  {
    outputs[m_PinVelocity.m_uiOutputIndex] = *inputs[m_PinVelocity.m_uiInputIndex];
  }

  // Material
  if (inputs[m_PinMaterial.m_uiInputIndex])
  {
    outputs[m_PinMaterial.m_uiOutputIndex] = *inputs[m_PinMaterial.m_uiInputIndex];
  }

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

void plForwardRenderPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plGALPass* pGALPass = pDevice->BeginPass(GetName());

  SetupResources(pGALPass, renderViewContext, inputs, outputs);
  SetupPermutationVars(renderViewContext);
  SetupLighting(renderViewContext);

  RenderObjects(renderViewContext);

  renderViewContext.m_pRenderContext->EndRendering();
  pDevice->EndPass(pGALPass);
}

plResult plForwardRenderPass::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_ShadingQuality;
  return PL_SUCCESS;
}

plResult plForwardRenderPass::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_ShadingQuality;
  return PL_SUCCESS;
}

void plForwardRenderPass::SetupResources(plGALPass* pGALPass, const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // Setup render target
  plGALRenderingSetup renderingSetup;
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(inputs[m_PinColor.m_uiInputIndex]->m_TextureHandle));
  }

  if (inputs[m_PinVelocity.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(1, pDevice->GetDefaultRenderTargetView(inputs[m_PinVelocity.m_uiInputIndex]->m_TextureHandle));
  }

  if (inputs[m_PinMaterial.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(2, pDevice->GetDefaultRenderTargetView(inputs[m_PinMaterial.m_uiInputIndex]->m_TextureHandle));
  }


  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  renderViewContext.m_pRenderContext->BeginRendering(pGALPass, std::move(renderingSetup), renderViewContext.m_pViewData->m_ViewPortRect, "", renderViewContext.m_pCamera->IsStereoscopic());
}

void plForwardRenderPass::SetupPermutationVars(const plRenderViewContext& renderViewContext)
{
  plTempHashedString sRenderPass("RENDER_PASS_FORWARD");
  if (renderViewContext.m_pViewData->m_ViewRenderMode != plViewRenderMode::None)
  {
    sRenderPass = plViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  plStringBuilder sDebugText;
  plViewRenderMode::GetDebugText(renderViewContext.m_pViewData->m_ViewRenderMode, sDebugText);
  if (!sDebugText.IsEmpty())
  {
    plDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, sDebugText, plVec2I32(10, 10), plColor::White);
  }

  // Set permutation for shading quality
  if (m_ShadingQuality == plForwardRenderShadingQuality::Normal)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_NORMAL");
  }
  else if (m_ShadingQuality == plForwardRenderShadingQuality::Simplified)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_SIMPLIFIED");
  }
  else
  {
    PL_REPORT_FAILURE("Unknown shading quality setting.");
  }
}

void plForwardRenderPass::SetupLighting(const plRenderViewContext& renderViewContext)
{
  // Setup clustered data
  if (m_ShadingQuality == plForwardRenderShadingQuality::Normal)
  {
    auto pClusteredData = GetPipeline()->GetFrameDataProvider<plClusteredDataProvider>()->GetData(renderViewContext);
    pClusteredData->BindResources(renderViewContext.m_pRenderContext);
  }
  // Or other light properties.
  else
  {
    auto pSimplifiedData = GetPipeline()->GetFrameDataProvider<plSimplifiedDataProvider>()->GetData(renderViewContext);
    pSimplifiedData->BindResources(renderViewContext.m_pRenderContext);
    // todo
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plForwardRenderPassPatch_1_2 : public plGraphPatch
{
public:
  plForwardRenderPassPatch_1_2()
    : plGraphPatch("plForwardRenderPass", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("Velocity", {});
  }
};

plForwardRenderPassPatch_1_2 g_plForwardRenderPassPatch_1_2;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class plForwardRenderPassPatch_2_3 : public plGraphPatch
{
public:
  plForwardRenderPassPatch_2_3()
    : plGraphPatch("plForwardRenderPass", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("Material", {});
  }
};

plForwardRenderPassPatch_2_3 g_plForwardRenderPassPatch_2_3;


PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ForwardRenderPass);
