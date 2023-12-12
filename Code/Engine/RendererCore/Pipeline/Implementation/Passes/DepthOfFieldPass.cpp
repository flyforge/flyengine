#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/Passes/DepthOfFieldPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#include "../../../../../../Data/Base/Shaders/Pipeline/DepthOfFieldConstants.h"

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDepthOfFieldPass, 1, plRTTIDefaultAllocator<plDepthOfFieldPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Color", m_PinInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_MEMBER_PROPERTY("DepthStencil", m_PinDepth),
    PLASMA_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new plDefaultValueAttribute(5.5f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plDepthOfFieldPass::plDepthOfFieldPass()
  : plRenderPipelinePass("DepthOfFieldPass")
  , m_fRadius(5.5f)
{
  // Loading shaders
  {
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/DepthOfField.plShader");
    PLASMA_ASSERT_DEV(m_hShader.IsValid(), "Could not load DepthOfField shader!");
  }

  // Loading resources
  {
    m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plDepthOfFieldConstants>();
  }
}

plDepthOfFieldPass::~plDepthOfFieldPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool plDepthOfFieldPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    if (!inputs[m_PinInput.m_uiInputIndex]->m_bAllowShaderResourceView)
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
    plGALTextureCreationDescription desc = *inputs[m_PinInput.m_uiInputIndex];
    desc.m_bAllowUAV = true;
    outputs[m_PinOutput.m_uiOutputIndex] = std::move(desc);
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  const plUInt32 uiWidth = outputs[m_PinOutput.m_uiOutputIndex].m_uiWidth;
  const plUInt32 uiHeight = outputs[m_PinOutput.m_uiOutputIndex].m_uiHeight;

  const plUInt32 uiWidthHalf = uiWidth / 2;
  const plUInt32 uiHeightHalf = uiHeight / 2;
  {
    plGALTextureCreationDescription desc = *inputs[m_PinInput.m_uiInputIndex];
    desc.m_uiWidth = uiWidthHalf;
    desc.m_uiHeight = uiHeightHalf;
    desc.m_bAllowUAV = true;
    desc.m_bAllowShaderResourceView = true;

    m_hBokehTexture1 = pDevice->CreateTexture(desc);
    m_hBokehTexture2 = pDevice->CreateTexture(desc);
  }

  return true;
}

void plDepthOfFieldPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInput = inputs[m_PinInput.m_uiInputIndex];
  const auto* const pDepth = inputs[m_PinDepth.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pDepth == nullptr || pOutput == nullptr)
  {
    return;
  }

  if(plRenderWorld::GetOverridePipeline() && !plRenderWorld::GetDOFEnabled())
  {
    ExecuteInactive(renderViewContext, inputs, outputs);
  }

  plTempHashedString sCircleOfConfusionPass = "DOF_PASS_MODE_COC";
  plTempHashedString sBokehPass = "DOF_PASS_MODE_BOKEH";
  plTempHashedString sTentPass = "DOF_PASS_MODE_TENT";
  plTempHashedString sBlendPass = "DOF_PASS_MODE_UPSCALE_BLEND";

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plResourceManager::ForceLoadResourceNow(m_hShader);

  const bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  const plUInt32 uiWidth = pOutput->m_Desc.m_uiWidth;
  const plUInt32 uiHeight = pOutput->m_Desc.m_uiHeight;

  const plUInt32 uiWidthHalf = uiWidth / 2;
  const plUInt32 uiHeightHalf = uiHeight / 2;

  plGALPass* pPass = pDevice->BeginPass(GetName());
  {
    // Circle of Confusion Pass
    {
      auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "Circle of Confusion");
      renderViewContext.m_pRenderContext->BindShader(m_hShader);

      plGALUnorderedAccessViewHandle hOutput;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hBokehTexture1;
        hOutput = pDevice->CreateUnorderedAccessView(desc);
      }

      renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
      renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pDevice->GetDefaultResourceView(pDepth->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindConstantBuffer("plDepthOfFieldConstants", m_hConstantBuffer);

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("DOF_PASS_MODE", sCircleOfConfusionPass);

      const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
      const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

      UpdateConstantBuffer();

      plGlobalConstants& gc = renderViewContext.m_pRenderContext->WriteGlobalConstants();
      plVec4 olViewportSize = gc.ViewportSize;

      gc.ViewportSize = plVec4(uiWidthHalf, uiHeightHalf, 1.0f / uiWidthHalf, 1.0f / uiHeightHalf);
      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
      gc.ViewportSize = olViewportSize;
    }

    // Bokeh Pass
    {
      auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "Bokeh");
      renderViewContext.m_pRenderContext->BindShader(m_hShader);

      plGALUnorderedAccessViewHandle hOutput;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hBokehTexture2;
        hOutput = pDevice->CreateUnorderedAccessView(desc);
      }

      renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
      renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(m_hBokehTexture1));
      renderViewContext.m_pRenderContext->BindConstantBuffer("plDepthOfFieldConstants", m_hConstantBuffer);

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("DOF_PASS_MODE", sBokehPass);

      const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
      const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

      UpdateConstantBuffer();

      plGlobalConstants& gc = renderViewContext.m_pRenderContext->WriteGlobalConstants();
      plVec4 olViewportSize = gc.ViewportSize;

      gc.ViewportSize = plVec4(uiWidthHalf, uiHeightHalf, 1.0f / uiWidthHalf, 1.0f / uiHeightHalf);
      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
      gc.ViewportSize = olViewportSize;
    }

    // Tent Pass
    {
      auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "Tent");
      renderViewContext.m_pRenderContext->BindShader(m_hShader);

      plGALUnorderedAccessViewHandle hOutput;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hBokehTexture1;
        hOutput = pDevice->CreateUnorderedAccessView(desc);
      }

      renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
      renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(m_hBokehTexture2));
      renderViewContext.m_pRenderContext->BindConstantBuffer("plDepthOfFieldConstants", m_hConstantBuffer);

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("DOF_PASS_MODE", sTentPass);

      const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
      const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

      UpdateConstantBuffer();

      plGlobalConstants& gc = renderViewContext.m_pRenderContext->WriteGlobalConstants();
      plVec4 olViewportSize = gc.ViewportSize;

      gc.ViewportSize = plVec4(uiWidthHalf, uiHeightHalf, 1.0f / uiWidthHalf, 1.0f / uiHeightHalf);
      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
      gc.ViewportSize = olViewportSize;
    }

    // Upscale Blend Pass
    {
      auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "Upscale Blend");
      renderViewContext.m_pRenderContext->BindShader(m_hShader);

      plGALUnorderedAccessViewHandle hOutput;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = pOutput->m_TextureHandle;
        hOutput = pDevice->CreateUnorderedAccessView(desc);
      }

      renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
      renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("BokehTexture", pDevice->GetDefaultResourceView(m_hBokehTexture1));
      renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pDevice->GetDefaultResourceView(pDepth->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindConstantBuffer("plDepthOfFieldConstants", m_hConstantBuffer);

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("DOF_PASS_MODE", sBlendPass);

      const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
      const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

      UpdateConstantBuffer();

      plGlobalConstants& gc = renderViewContext.m_pRenderContext->WriteGlobalConstants();
      plVec4 olViewportSize = gc.ViewportSize;

      gc.ViewportSize = plVec4(uiWidth, uiHeight, 1.0f / uiWidth, 1.0f / uiHeight);
      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
      gc.ViewportSize = olViewportSize;
    }
  }
  pDevice->EndPass(pPass);

  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading);
}

void plDepthOfFieldPass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
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

void plDepthOfFieldPass::UpdateConstantBuffer()
{
  auto* constants = plRenderContext::GetConstantBufferData<plDepthOfFieldConstants>(m_hConstantBuffer);
  constants->Radius = plRenderWorld::GetOverridePipeline() ? plRenderWorld::GetDOFRadius() : m_fRadius;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_DepthOfFieldPass);
