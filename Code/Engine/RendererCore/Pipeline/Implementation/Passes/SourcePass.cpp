#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Pipeline/Passes/SourcePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSourcePass, 2, plRTTIDefaultAllocator<plSourcePass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_ENUM_MEMBER_PROPERTY("Format", plGALResourceFormat, m_Format),
    PLASMA_ENUM_MEMBER_PROPERTY("MSAA_Mode", plGALMSAASampleCount, m_MsaaMode),
    PLASMA_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new plExposeColorAlphaAttribute()),
    PLASMA_MEMBER_PROPERTY("Clear", m_bClear),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSourcePass::plSourcePass(const char* szName)
  : plRenderPipelinePass(szName, true)
{
  m_Format = plGALResourceFormat::RGBAUByteNormalizedsRGB;
  m_MsaaMode = plGALMSAASampleCount::None;
  m_bClear = true;
  m_ClearColor = plColor::Black;
}

plSourcePass::~plSourcePass() {}

bool plSourcePass::GetRenderTargetDescriptions(
  const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  plUInt32 uiWidth = static_cast<plUInt32>(view.GetViewport().width);
  plUInt32 uiHeight = static_cast<plUInt32>(view.GetViewport().height);

  plGALTextureCreationDescription desc;
  desc.m_uiWidth = uiWidth;
  desc.m_uiHeight = uiHeight;
  desc.m_SampleCount = m_MsaaMode;
  desc.m_Format = m_Format;
  desc.m_bCreateRenderTarget = true;
  desc.m_uiArraySize = view.GetCamera()->IsStereoscopic() ? 2 : 1;

  outputs[m_PinOutput.m_uiOutputIndex] = desc;

  return true;
}

void plSourcePass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs,
  const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  if (!m_bClear)
    return;

  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
  {
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // Setup render target
  plGALRenderingSetup renderingSetup;
  renderingSetup.m_ClearColor = m_ClearColor;
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_bClearDepth = true;
  renderingSetup.m_bClearStencil = true;

  if (plGALResourceFormat::IsDepthFormat(m_Format))
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  }
  else
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  }

  auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class plSourcePassPatch_1_2 : public plGraphPatch
{
public:
  plSourcePassPatch_1_2()
    : plGraphPatch("plSourcePass", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("MSAA Mode", "MSAA_Mode");
    pNode->RenameProperty("Clear Color", "ClearColor");
  }
};

plSourcePassPatch_1_2 g_plSourcePassPatch_1_2;



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SourcePass);
