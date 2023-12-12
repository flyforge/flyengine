#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/MotionBlurPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include "../../../../Data/Base/Shaders/Pipeline/MotionBlurConstants.h"

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plMotionBlurMode, 1)
  PLASMA_ENUM_CONSTANTS(plMotionBlurMode::ObjectBased, plMotionBlurMode::ScreenBased)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMotionBlurPass, 1, plRTTIDefaultAllocator<plMotionBlurPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Color", m_PinInputColor),
    PLASMA_MEMBER_PROPERTY("Velocity", m_PinInputVelocity),
    PLASMA_MEMBER_PROPERTY("DepthStencil", m_PinInputDepth),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_MEMBER_PROPERTY("Samples", m_fSamples)->AddAttributes(new plDefaultValueAttribute(32)),
    PLASMA_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new plClampValueAttribute(0.0f, 10.0f), new plDefaultValueAttribute(1.0f)),
    PLASMA_ENUM_MEMBER_PROPERTY("Mode", plMotionBlurMode, m_eMode)->AddAttributes(new plDefaultValueAttribute(plMotionBlurMode::ObjectBased)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plMotionBlurPass::plMotionBlurPass()
  : plRenderPipelinePass("MotionBlurPass")
  , m_fSamples(32)
  , m_fStrength(1.0f)
  , m_eMode(plMotionBlurMode::ObjectBased)
{
  // Load shaders
  {
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/MotionBlur.plShader");
    PLASMA_ASSERT_DEV(m_hShader.IsValid(), "Could not load MotionBlur shader!");
  }

  // Load resources
  {
    m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plMotionBlurConstants>();
  }
}

plMotionBlurPass::~plMotionBlurPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool plMotionBlurPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Velocity
  if (inputs[m_PinInputVelocity.m_uiInputIndex])
  {
    if (!inputs[m_PinInputVelocity.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' velocity input must allow shader resource view.", GetName());
      return false;
    }
  }
  else if (m_eMode == plMotionBlurMode::ObjectBased) // Required when object based
  {
    plLog::Error("No velocity input connected to '{0}'!", GetName());
    return false;
  }

  // Depth
  if (inputs[m_PinInputDepth.m_uiInputIndex])
  {
    if (!inputs[m_PinInputDepth.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' depth input must allow shader resource view.", GetName());
      return false;
    }
  }
  else if (m_eMode == plMotionBlurMode::ScreenBased) // Required when screen based
  {
    plLog::Error("No depth input connected to '{0}'!", GetName());
    return false;
  }

  // Color
  if (inputs[m_PinInputColor.m_uiInputIndex])
  {
    if (!inputs[m_PinInputColor.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' color input must allow shader resource view.", GetName());
      return false;
    }
  }
  else
  {
    plLog::Error("No color input connected to '{0}'!", GetName());
    return false;
  }

  {
    plGALTextureCreationDescription desc = *inputs[m_PinInputColor.m_uiInputIndex];
    desc.m_bAllowUAV = true;
    desc.m_bCreateRenderTarget = true;
    outputs[m_PinOutput.m_uiOutputIndex] = std::move(desc);
  }

  return true;
}

void plMotionBlurPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInputColor = inputs[m_PinInputColor.m_uiInputIndex];
  const auto* const pInputVelocity = inputs[m_PinInputVelocity.m_uiInputIndex];
  const auto* const pInputDepth = inputs[m_PinInputDepth.m_uiInputIndex];

  if (pInputColor == nullptr || (m_eMode == plMotionBlurMode::ObjectBased && pInputVelocity == nullptr) || (m_eMode == plMotionBlurMode::ScreenBased && pInputDepth == nullptr))
  {
    return;
  }

  if(plRenderWorld::GetOverridePipeline() && !plRenderWorld::GetMotionBlurEnabled())
  {
    ExecuteInactive(renderViewContext, inputs, outputs);
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plResourceManager::ForceLoadResourceNow(m_hShader);

  const bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  plGALPass* pPass = pDevice->BeginPass(GetName());
  PLASMA_SCOPE_EXIT(
    pDevice->EndPass(pPass);
    renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

  if (const auto* pColorOutput = outputs[m_PinOutput.m_uiOutputIndex]; pColorOutput != nullptr && !pColorOutput->m_TextureHandle.IsInvalidated())
  {
    auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext);
    renderViewContext.m_pRenderContext->BindShader(m_hShader);

    renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(pInputColor->m_TextureHandle));

    if (m_eMode == plMotionBlurMode::ObjectBased)
    {
      renderViewContext.m_pRenderContext->BindTexture2D("VelocityTexture", pDevice->GetDefaultResourceView(pInputVelocity->m_TextureHandle));
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("MOTION_BLUR_MODE", "MOTION_BLUR_MODE_OBJECT_BASED");
    }
    else
    {
      renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("MOTION_BLUR_MODE", "MOTION_BLUR_MODE_SCREEN_BASED");
    }

    plGALUnorderedAccessViewHandle hMotionBlurOutput;
    {
      plGALUnorderedAccessViewCreationDescription desc;
      desc.m_hTexture = pColorOutput->m_TextureHandle;
      desc.m_uiMipLevelToUse = 0;
      hMotionBlurOutput = pDevice->CreateUnorderedAccessView(desc);
    }
    renderViewContext.m_pRenderContext->BindUAV("MotionBlurOutput", hMotionBlurOutput);

    const plUInt32 uiWidth = pColorOutput->m_Desc.m_uiWidth;
    const plUInt32 uiHeight = pColorOutput->m_Desc.m_uiHeight;

    const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
    const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

    auto* constants = plRenderContext::GetConstantBufferData<plMotionBlurConstants>(m_hConstantBuffer);
    constants->MotionBlurSamples = m_fSamples;
    constants->MotionBlurStrength = m_fStrength;

    renderViewContext.m_pRenderContext->BindConstantBuffer("plMotionBlurConstants", m_hConstantBuffer);

    renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
  }
}

void plMotionBlurPass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto pInput = inputs[m_PinInputColor.m_uiInputIndex];
  const auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  auto pCommandEncoder = plRenderContext::BeginPassAndComputeScope(renderViewContext, GetName());

  pCommandEncoder->CopyTexture(pOutput->m_TextureHandle, pInput->m_TextureHandle);
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_MotionBlurPass);
