#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/MsaaUpscalePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsaaUpscalePass, 2, plRTTIDefaultAllocator<plMsaaUpscalePass>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Input", m_PinInput),
    PL_MEMBER_PROPERTY("Output", m_PinOutput),
    PL_ENUM_MEMBER_PROPERTY("MSAA_Mode", plGALMSAASampleCount, m_MsaaMode)
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plMsaaUpscalePass::plMsaaUpscalePass()
  : plRenderPipelinePass("MsaaUpscalePass")

{
  {
    // Load shader.
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/MsaaUpscale.plShader");
    PL_ASSERT_DEV(m_hShader.IsValid(), "Could not load msaa upscale shader!");
  }
}

plMsaaUpscalePass::~plMsaaUpscalePass() = default;

bool plMsaaUpscalePass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_SampleCount != plGALMSAASampleCount::None)
    {
      plLog::Error("Input must not be a msaa target");
      return false;
    }

    plGALTextureCreationDescription desc = *pInput;
    desc.m_SampleCount = m_MsaaMode;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    plLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void plMsaaUpscalePass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // Setup render target
  plGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));

  // Bind render target and viewport
  auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}

plResult plMsaaUpscalePass::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_MsaaMode;
  return PL_SUCCESS;
}

plResult plMsaaUpscalePass::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_MsaaMode;
  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plMsaaUpscalePassPatch_1_2 : public plGraphPatch
{
public:
  plMsaaUpscalePassPatch_1_2()
    : plGraphPatch("plMsaaUpscalePass", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override { pNode->RenameProperty("MSAA Mode", "MSAA_Mode"); }
};

plMsaaUpscalePassPatch_1_2 g_plMsaaUpscalePassPatch_1_2;



PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_MsaaUpscalePass);
