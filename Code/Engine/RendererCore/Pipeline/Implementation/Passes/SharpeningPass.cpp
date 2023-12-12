#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/Passes/SharpeningPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include "../../../../../../Data/Base/Shaders/Pipeline/SharpeningConstants.h"

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSharpeningPass, 1, plRTTIDefaultAllocator<plSharpeningPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Input", m_PinInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new plDefaultValueAttribute(0.5f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSharpeningPass::plSharpeningPass()
  : plRenderPipelinePass("SharpeningPass")
  , m_fStrength(0.5f)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // Load Shader
  {
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/Sharpening.plShader");
    PLASMA_ASSERT_DEV(m_hShader.IsValid(), "Could not load Sharpening shader!");
  }

  // Load resources
  {
    m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plSharpeningConstants>();
  }
}

plSharpeningPass::~plSharpeningPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool plSharpeningPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Input
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    if (!inputs[m_PinInput.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' input must allow shader resource view.", GetName());
      return false;
    }
  }
  else
  {
    plLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  {
    plGALTextureCreationDescription desc = *inputs[m_PinInput.m_uiInputIndex];
    desc.m_bAllowUAV = true;
    desc.m_bCreateRenderTarget = true;
    outputs[m_PinOutput.m_uiOutputIndex] = std::move(desc);
  }

  return true;
}

void plSharpeningPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* pColorInput = inputs[m_PinInput.m_uiInputIndex];
  const auto* pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pColorInput == nullptr || pColorOutput == nullptr)
  {
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plResourceManager::ForceLoadResourceNow(m_hShader);

  const bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  plGALPass* pPass = pDevice->BeginPass(GetName());
  {
    auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext);
    renderViewContext.m_pRenderContext->BindShader(m_hShader);

    renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(pColorInput->m_TextureHandle));

    plGALUnorderedAccessViewHandle hOutput;
    {
      plGALUnorderedAccessViewCreationDescription desc;
      desc.m_hTexture = pColorOutput->m_TextureHandle;
      desc.m_uiMipLevelToUse = 0;
      hOutput = pDevice->CreateUnorderedAccessView(desc);
    }
    renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);

    renderViewContext.m_pRenderContext->BindConstantBuffer("plSharpeningConstants", m_hConstantBuffer);

    const plUInt32 uiWidth = pColorOutput->m_Desc.m_uiWidth;
    const plUInt32 uiHeight = pColorOutput->m_Desc.m_uiHeight;

    const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
    const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

    UpdateConstantBuffer();

    renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
  }
  pDevice->EndPass(pPass);

  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading);
}

void plSharpeningPass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto pInput = inputs[m_PinInput.m_uiInputIndex];
  const auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  auto pCommandEncoder = plRenderContext::BeginPassAndComputeScope(renderViewContext, GetName());

  pCommandEncoder->CopyTexture(pOutput->m_TextureHandle, pInput->m_TextureHandle);
}

void plSharpeningPass::UpdateConstantBuffer() const
{
  auto* constants = plRenderContext::GetConstantBufferData<plSharpeningConstants>(m_hConstantBuffer);
  constants->Strength = m_fStrength;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SharpeningPass);
