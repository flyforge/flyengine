#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/BloomPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BloomConstants.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plBloomPass, 1, plRTTIDefaultAllocator<plBloomPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Input", m_PinInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new plDefaultValueAttribute(0.2f), new plClampValueAttribute(0.01f, 1.0f)),
    PLASMA_MEMBER_PROPERTY("Threshold", m_fThreshold)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new plDefaultValueAttribute(0.3f)),
    PLASMA_MEMBER_PROPERTY("InnerTintColor", m_InnerTintColor),
    PLASMA_MEMBER_PROPERTY("MidTintColor", m_MidTintColor),
    PLASMA_MEMBER_PROPERTY("OuterTintColor", m_OuterTintColor),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plBloomPass::plBloomPass()
  : plRenderPipelinePass("BloomPass", true)
{
  {
    // Load shader.
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/Bloom.plShader");
    PLASMA_ASSERT_DEV(m_hShader.IsValid(), "Could not load bloom shader!");
  }

  {
    m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plBloomConstants>();
  }
}

plBloomPass::~plBloomPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool plBloomPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    if (!inputs[m_PinInput.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' input must allow shader resource view.", GetName());
      return false;
    }

    // Output is half-res
    plGALTextureCreationDescription desc = *inputs[m_PinInput.m_uiInputIndex];
    desc.m_uiWidth = desc.m_uiWidth / 2;
    desc.m_uiHeight = desc.m_uiHeight / 2;
    desc.m_Format = plGALResourceFormat::RG11B10Float;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    plLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void plBloomPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinInput.m_uiInputIndex];
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorInput == nullptr || pColorOutput == nullptr)
  {
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plGALPass* pGALPass = pDevice->BeginPass(GetName());
  PLASMA_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  plUInt32 uiWidth = pColorInput->m_Desc.m_uiWidth;
  plUInt32 uiHeight = pColorInput->m_Desc.m_uiHeight;
  bool bFastDownscale = plMath::IsEven(uiWidth) && plMath::IsEven(uiHeight);

  const float fMaxRes = (float)plMath::Max(uiWidth, uiHeight);
  const float fRadius = plMath::Clamp(m_fRadius, 0.01f, 1.0f);
  const float fDownscaledSize = 4.0f / fRadius;
  const float fNumBlurPasses = plMath::Log2(fMaxRes / fDownscaledSize);
  const plUInt32 uiNumBlurPasses = (plUInt32)plMath::Ceil(fNumBlurPasses);

  // Find temp targets
  plHybridArray<plVec2, 8> targetSizes;
  plHybridArray<plGALTextureHandle, 8> tempDownscaleTextures;
  plHybridArray<plGALTextureHandle, 8> tempUpscaleTextures;

  for (plUInt32 i = 0; i < uiNumBlurPasses; ++i)
  {
    uiWidth = plMath::Max(uiWidth / 2, 1u);
    uiHeight = plMath::Max(uiHeight / 2, 1u);
    targetSizes.PushBack(plVec2((float)uiWidth, (float)uiHeight));
    auto uiSliceCount = pColorOutput->m_Desc.m_uiArraySize;

    tempDownscaleTextures.PushBack(plGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, plGALResourceFormat::RG11B10Float, plGALMSAASampleCount::None, uiSliceCount));

    // biggest upscale target is the output and lowest is not needed
    if (i > 0 && i < uiNumBlurPasses - 1)
    {
      tempUpscaleTextures.PushBack(plGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, plGALResourceFormat::RG11B10Float, plGALMSAASampleCount::None, uiSliceCount));
    }
    else
    {
      tempUpscaleTextures.PushBack(plGALTextureHandle());
    }
  }

  renderViewContext.m_pRenderContext->BindConstantBuffer("plBloomConstants", m_hConstantBuffer);
  renderViewContext.m_pRenderContext->BindShader(m_hShader);

  renderViewContext.m_pRenderContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, 1);

  // Downscale passes
  {
    plTempHashedString sInitialDownscale = "BLOOM_PASS_MODE_INITIAL_DOWNSCALE";
    plTempHashedString sInitialDownscaleFast = "BLOOM_PASS_MODE_INITIAL_DOWNSCALE_FAST";
    plTempHashedString sDownscale = "BLOOM_PASS_MODE_DOWNSCALE";
    plTempHashedString sDownscaleFast = "BLOOM_PASS_MODE_DOWNSCALE_FAST";

    for (plUInt32 i = 0; i < uiNumBlurPasses; ++i)
    {
      plGALTextureHandle hInput;
      if (i == 0)
      {
        hInput = pColorInput->m_TextureHandle;
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", bFastDownscale ? sInitialDownscaleFast : sInitialDownscale);
      }
      else
      {
        hInput = tempDownscaleTextures[i - 1];
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", bFastDownscale ? sDownscaleFast : sDownscale);
      }

      plGALTextureHandle hOutput = tempDownscaleTextures[i];
      plVec2 targetSize = targetSizes[i];

      plGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(hOutput));
      renderViewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, plRectFloat(targetSize.x, targetSize.y), "Downscale", renderViewContext.m_pCamera->IsStereoscopic());

      plColor tintColor = (i == uiNumBlurPasses - 1) ? plColor(m_OuterTintColor) : plColor::White;
      UpdateConstantBuffer(plVec2(1.0f).CompDiv(targetSize), tintColor);

      renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(hInput));
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();

      bFastDownscale = plMath::IsEven((plInt32)targetSize.x) && plMath::IsEven((plInt32)targetSize.y);
    }
  }

  // Upscale passes
  {
    const float fBlurRadius = 2.0f * fNumBlurPasses / uiNumBlurPasses;
    const float fMidPass = (uiNumBlurPasses - 1.0f) / 2.0f;

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", "BLOOM_PASS_MODE_UPSCALE");

    for (plUInt32 i = uiNumBlurPasses - 1; i-- > 0;)
    {
      plGALTextureHandle hNextInput = tempDownscaleTextures[i];
      plGALTextureHandle hInput;
      if (i == uiNumBlurPasses - 2)
      {
        hInput = tempDownscaleTextures[i + 1];
      }
      else
      {
        hInput = tempUpscaleTextures[i + 1];
      }

      plGALTextureHandle hOutput;
      if (i == 0)
      {
        hOutput = pColorOutput->m_TextureHandle;
      }
      else
      {
        hOutput = tempUpscaleTextures[i];
      }

      plVec2 targetSize = targetSizes[i];

      plGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(hOutput));
      renderViewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, plRectFloat(targetSize.x, targetSize.y), "Upscale", renderViewContext.m_pCamera->IsStereoscopic());

      plColor tintColor;
      float fPass = (float)i;
      if (fPass < fMidPass)
      {
        tintColor = plMath::Lerp<plColor>(m_InnerTintColor, m_MidTintColor, fPass / fMidPass);
      }
      else
      {
        tintColor = plMath::Lerp<plColor>(m_MidTintColor, m_OuterTintColor, (fPass - fMidPass) / fMidPass);
      }

      UpdateConstantBuffer(plVec2(fBlurRadius).CompDiv(targetSize), tintColor);

      renderViewContext.m_pRenderContext->BindTexture2D("NextColorTexture", pDevice->GetDefaultResourceView(hNextInput));
      renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(hInput));
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();
    }
  }

  // Return temp targets
  for (auto hTexture : tempDownscaleTextures)
  {
    if (!hTexture.IsInvalidated())
    {
      plGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hTexture);
    }
  }

  for (auto hTexture : tempUpscaleTextures)
  {
    if (!hTexture.IsInvalidated())
    {
      plGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hTexture);
    }
  }
}

void plBloomPass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorOutput == nullptr)
  {
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_ClearColor = plColor::Black;

  auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, "Clear");
}

plResult plBloomPass::Serialize(plStreamWriter& inout_stream) const
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fRadius;
  inout_stream << m_fThreshold;
  inout_stream << m_fIntensity;
  inout_stream << m_InnerTintColor;
  inout_stream << m_MidTintColor;
  inout_stream << m_OuterTintColor;
  return PLASMA_SUCCESS;
}

plResult plBloomPass::Deserialize(plStreamReader& inout_stream)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PLASMA_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fRadius;
  inout_stream >> m_fThreshold;
  inout_stream >> m_fIntensity;
  inout_stream >> m_InnerTintColor;
  inout_stream >> m_MidTintColor;
  inout_stream >> m_OuterTintColor;
  return PLASMA_SUCCESS;
}

void plBloomPass::UpdateConstantBuffer(plVec2 pixelSize, const plColor& tintColor)
{
  plBloomConstants* constants = plRenderContext::GetConstantBufferData<plBloomConstants>(m_hConstantBuffer);
  constants->PixelSize = pixelSize;
  constants->BloomThreshold = m_fThreshold;
  constants->BloomIntensity = m_fIntensity;

  constants->TintColor = tintColor;
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_BloomPass);
