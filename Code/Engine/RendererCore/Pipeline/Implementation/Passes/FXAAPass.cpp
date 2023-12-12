#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/FXAAPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include "../../../../../../Data/Base/Shaders/Pipeline/FXAAConstants.h"

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plEdgeThresholdQuality, 1)
  PLASMA_ENUM_CONSTANT(plEdgeThresholdQuality::Little),
  PLASMA_ENUM_CONSTANT(plEdgeThresholdQuality::LowQuality),
  PLASMA_ENUM_CONSTANT(plEdgeThresholdQuality::DefaultQuality),
  PLASMA_ENUM_CONSTANT(plEdgeThresholdQuality::HighQuality),
  PLASMA_ENUM_CONSTANT(plEdgeThresholdQuality::Overkill),
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plEdgeThresholdMinQuality, 1)
  PLASMA_ENUM_CONSTANT(plEdgeThresholdMinQuality::UpperLimit),
  PLASMA_ENUM_CONSTANT(plEdgeThresholdMinQuality::HighQuality),
  PLASMA_ENUM_CONSTANT(plEdgeThresholdMinQuality::VisibleLimit),
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plFXAAPass, 1, plRTTIDefaultAllocator<plFXAAPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Input", m_PinInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_MEMBER_PROPERTY("SubPixelAliasingRemovalAmount", m_fSPARAmount)->AddAttributes(new plDefaultValueAttribute(0.75f), new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_ENUM_MEMBER_PROPERTY("EdgeThreshold", plEdgeThresholdQuality, m_eEdgeThreshold)->AddAttributes(new plDefaultValueAttribute(plEdgeThresholdQuality::Default)),
    PLASMA_ENUM_MEMBER_PROPERTY("EdgeThresholdMin", plEdgeThresholdMinQuality, m_eEdgeThresholdMin)->AddAttributes(new plDefaultValueAttribute(plEdgeThresholdMinQuality::Default)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


plFXAAPass::plFXAAPass()
  : plRenderPipelinePass("FXAAPass", true)
  , m_fSPARAmount(0.75f)
  , m_eEdgeThreshold(plEdgeThresholdQuality::Default)
  , m_eEdgeThresholdMin(plEdgeThresholdMinQuality::Default)
{
  // Load shader.
  {
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/FXAA.plShader");
    PLASMA_ASSERT_DEV(m_hShader.IsValid(), "Could not load FXAA Pass shader!");
  }

  // Init constant buffer
  {
    m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plFXAAConstants>();
  }
}

plFXAAPass::~plFXAAPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool plFXAAPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    if (!inputs[m_PinInput.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' Color input must allow shader resource view.", GetName());
      return false;
    }

    plGALTextureCreationDescription desc = *inputs[m_PinInput.m_uiInputIndex];
    desc.m_bAllowUAV = true;
    desc.m_bCreateRenderTarget = true;
    outputs[m_PinOutput.m_uiOutputIndex] = std::move(desc);
  }
  else
  {
    plLog::Error("No Color input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void plFXAAPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInput = inputs[m_PinInput.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
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

    plGALUnorderedAccessViewHandle hOutput;
    {
      plGALUnorderedAccessViewCreationDescription desc;
      desc.m_hTexture = pOutput->m_TextureHandle;
      desc.m_uiMipLevelToUse = 0;
      hOutput = pDevice->CreateUnorderedAccessView(desc);
    }

    renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
    renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));
    renderViewContext.m_pRenderContext->BindConstantBuffer("plFXAAConstants", m_hConstantBuffer);

    const plUInt32 uiWidth = pOutput->m_Desc.m_uiWidth;
    const plUInt32 uiHeight = pOutput->m_Desc.m_uiHeight;

    const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
    const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

    UpdateConstantBuffer();

    renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
  }
  pDevice->EndPass(pPass);

  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading);
}

void plFXAAPass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInput = inputs[m_PinInput.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  const plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  const plGALTexture* pDest = pDevice->GetTexture(pOutput->m_TextureHandle);

  if (const plGALTexture* pSource = pDevice->GetTexture(pInput->m_TextureHandle); pDest->GetDescription().m_Format != pSource->GetDescription().m_Format)
  {
    // TODO: use a shader when the format doesn't match exactly

    plLog::Error("Copying textures of different formats is not implemented");
  }
  else
  {
    auto pCommandEncoder = plRenderContext::BeginPassAndComputeScope(renderViewContext, GetName());

    pCommandEncoder->CopyTexture(pOutput->m_TextureHandle, pInput->m_TextureHandle);
  }
}

void plFXAAPass::UpdateConstantBuffer() const
{
  auto* constants = plRenderContext::GetConstantBufferData<plFXAAConstants>(m_hConstantBuffer);
  constants->SubPixelAliasingRemovalAmount = m_fSPARAmount;

  switch (m_eEdgeThreshold)
  {
    case plEdgeThresholdQuality::Little: constants->EdgeThreshold = 0.333f; break;
    case plEdgeThresholdQuality::LowQuality: constants->EdgeThreshold = 0.250f; break;
    case plEdgeThresholdQuality::DefaultQuality: constants->EdgeThreshold = 0.166f; break;
    case plEdgeThresholdQuality::HighQuality: constants->EdgeThreshold = 0.125f; break;
    case plEdgeThresholdQuality::Overkill: constants->EdgeThreshold = 0.063f; break;
  }

  switch (m_eEdgeThresholdMin)
  {
    case plEdgeThresholdMinQuality::UpperLimit: constants->EdgeThresholdMin = 0.0833f; break;
    case plEdgeThresholdMinQuality::HighQuality: constants->EdgeThresholdMin = 0.0625f; break;
    case plEdgeThresholdMinQuality::VisibleLimit: constants->EdgeThresholdMin = 0.0312f; break;
  }
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_FXAAPass);
