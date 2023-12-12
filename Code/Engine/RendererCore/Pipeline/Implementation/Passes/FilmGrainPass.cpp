#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/FilmGrainPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include "../../../../../../Data/Base/Shaders/Pipeline/FilmGrainConstants.h"

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plFilmGrainPass, 1, plRTTIDefaultAllocator<plFilmGrainPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Input", m_PinInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new plDefaultValueAttribute(0.002f), new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new plDefaultValueAttribute(3.0f)),
    PLASMA_MEMBER_PROPERTY("Mean", m_fMean)->AddAttributes(new plDefaultValueAttribute(0.0f)),
    PLASMA_MEMBER_PROPERTY("Variance", m_fVariance)->AddAttributes(new plDefaultValueAttribute(0.5f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plFilmGrainPass::plFilmGrainPass()
  : plRenderPipelinePass("FilmGrainPass", true)
  , m_fIntensity(0.002f)
  , m_fSpeed(3.0f)
  , m_fMean(0.0f)
  , m_fVariance(0.5f)
{
  // Load shader.
  {
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/FilmGrain.plShader");
    PLASMA_ASSERT_DEV(m_hShader.IsValid(), "Could not load FilmGrain Pass shader!");
  }

  // Load resources.
  {
    m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plFilmGrainConstants>();
  }
}

plFilmGrainPass::~plFilmGrainPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool plFilmGrainPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Input
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

void plFilmGrainPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInput = inputs[m_PinInput.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
    return;

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
    renderViewContext.m_pRenderContext->BindConstantBuffer("plFilmGrainConstants", m_hConstantBuffer);

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

void plFilmGrainPass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInput = inputs[m_PinInput.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
    return;

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

void plFilmGrainPass::UpdateConstantBuffer() const
{
  auto* constants = plRenderContext::GetConstantBufferData<plFilmGrainConstants>(m_hConstantBuffer);
  constants->Intensity = m_fIntensity;
  constants->Speed = m_fSpeed;
  constants->Mean = m_fMean;
  constants->Variance = m_fVariance;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_FilmGrainPass);
