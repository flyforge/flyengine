#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/MsaaResolvePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsaaResolvePass, 1, plRTTIDefaultAllocator<plMsaaResolvePass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Input", m_PinInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plMsaaResolvePass::plMsaaResolvePass()
  : plRenderPipelinePass("MsaaResolvePass", true)

{
  {
    // Load shader.
    m_hDepthResolveShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/MsaaDepthResolve.plShader");
    PLASMA_ASSERT_DEV(m_hDepthResolveShader.IsValid(), "Could not load depth resolve shader!");
  }
}

plMsaaResolvePass::~plMsaaResolvePass() = default;

bool plMsaaResolvePass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_SampleCount == plGALMSAASampleCount::None)
    {
      plLog::Error("Input is not a valid msaa target");
      return false;
    }

    m_bIsDepth = plGALResourceFormat::IsDepthFormat(pInput->m_Format);
    m_MsaaSampleCount = pInput->m_SampleCount;

    plGALTextureCreationDescription desc = *pInput;
    desc.m_SampleCount = plGALMSAASampleCount::None;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    plLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void plMsaaResolvePass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  if (m_bIsDepth)
  {
    // Setup render target
    plGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));

    // Bind render target and viewport
    auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    auto& globals = renderViewContext.m_pRenderContext->WriteGlobalConstants();
    globals.NumMsaaSamples = m_MsaaSampleCount;

    renderViewContext.m_pRenderContext->BindShader(m_hDepthResolveShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
  else
  {
    auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, plGALRenderingSetup(), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    plGALTextureSubresource subresource;
    subresource.m_uiMipLevel = 0;
    subresource.m_uiArraySlice = 0;

    pCommandEncoder->ResolveTexture(pOutput->m_TextureHandle, subresource, pInput->m_TextureHandle, subresource);

    if (renderViewContext.m_pCamera->IsStereoscopic())
    {
      subresource.m_uiArraySlice = 1;
      pCommandEncoder->ResolveTexture(pOutput->m_TextureHandle, subresource, pInput->m_TextureHandle, subresource);
    }
  }
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_MsaaResolvePass);
