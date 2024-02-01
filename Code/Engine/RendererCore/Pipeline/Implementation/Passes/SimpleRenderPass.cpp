#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimpleRenderPass, 1, plRTTIDefaultAllocator<plSimpleRenderPass>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Color", m_PinColor),
    PL_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    PL_MEMBER_PROPERTY("Message", m_sMessage),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSimpleRenderPass::plSimpleRenderPass(const char* szName)
  : plRenderPipelinePass(szName, true)
{
}

plSimpleRenderPass::~plSimpleRenderPass() = default;

bool plSimpleRenderPass::GetRenderTargetDescriptions(
  const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  const plGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }
  else
  {
    // If no input is available, we use the render target setup instead.
    const plGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]);
    if (pTexture)
    {
      outputs[m_PinColor.m_uiOutputIndex] = pTexture->GetDescription();
      outputs[m_PinColor.m_uiOutputIndex].m_bCreateRenderTarget = true;
      outputs[m_PinColor.m_uiOutputIndex].m_bAllowShaderResourceView = true;
      outputs[m_PinColor.m_uiOutputIndex].m_ResourceAccess.m_bReadBack = false;
      outputs[m_PinColor.m_uiOutputIndex].m_ResourceAccess.m_bImmutable = true;
      outputs[m_PinColor.m_uiOutputIndex].m_pExisitingNativeObject = nullptr;
    }
  }

  // DepthStencil
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    // If no input is available, we use the render target setup instead.
    const plGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hDSTarget);
    if (pTexture)
    {
      outputs[m_PinDepthStencil.m_uiOutputIndex] = pTexture->GetDescription();
    }
  }

  return true;
}

void plSimpleRenderPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs,
  const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // Setup render target
  plGALRenderingSetup renderingSetup;
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(inputs[m_PinColor.m_uiInputIndex]->m_TextureHandle));
  }

  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  // Setup Permutation Vars
  plTempHashedString sRenderPass("RENDER_PASS_FORWARD");
  if (renderViewContext.m_pViewData->m_ViewRenderMode != plViewRenderMode::None)
  {
    sRenderPass = plViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  // Execute render functions
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::SimpleOpaque);
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::SimpleTransparent);

  if (!m_sMessage.IsEmpty())
  {
    plDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, m_sMessage.GetData(), plVec2I32(20, 20), plColor::OrangeRed);
  }

  plDebugRenderer::Render(renderViewContext);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::SimpleForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::SimpleForeground);

  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::GUI);
}

plResult plSimpleRenderPass::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_sMessage;
  return PL_SUCCESS;
}

plResult plSimpleRenderPass::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_sMessage;
  return PL_SUCCESS;
}

void plSimpleRenderPass::SetMessage(const char* szMessage)
{
  m_sMessage = szMessage;
}



PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SimpleRenderPass);
