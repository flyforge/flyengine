#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/ColorGradingPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include "../../../../../../Data/Base/Shaders/Pipeline/ColorGradingConstants.h"

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plColorGradingPass, 1, plRTTIDefaultAllocator<plColorGradingPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Color", m_PinInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_MEMBER_PROPERTY("MoodColor", m_MoodColor)->AddAttributes(new plDefaultValueAttribute(plColor::Orange)),
    PLASMA_MEMBER_PROPERTY("MoodStrength", m_fMoodStrength)->AddAttributes(new plClampValueAttribute(0.0f, {})),
    PLASMA_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new plClampValueAttribute(0.0f, 2.0f), new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_ACCESSOR_PROPERTY("LUT1", GetLUT1TextureFile, SetLUT1TextureFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
    PLASMA_MEMBER_PROPERTY("LUT1Strength", m_fLut1Strength)->AddAttributes(new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_ACCESSOR_PROPERTY("LUT2", GetLUT2TextureFile, SetLUT2TextureFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
    PLASMA_MEMBER_PROPERTY("LUT2Strength", m_fLut2Strength)->AddAttributes(new plClampValueAttribute(0.0f, 1.0f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plColorGradingPass::plColorGradingPass()
  : plRenderPipelinePass("ColorGradingPass", true)
  , m_MoodColor(plColor::Orange)
  , m_fMoodStrength(0.0f)
  , m_fSaturation(1.0f)
  , m_fContrast(1.0f)
  , m_fLut1Strength(0.0f)
  , m_fLut2Strength(0.0f)
{
  // Loading shaders
  {
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/ColorGrading.plShader");
    PLASMA_ASSERT_DEV(m_hShader.IsValid(), "Could not load ColorGrading shader!");
  }

  // Loading resources
  {
    m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plColorGradingConstants>();
  }
}

plColorGradingPass::~plColorGradingPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool plColorGradingPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
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
    plLog::Error("No input connected to '{0}'.", GetName());
    return false;
  }

  plGALTextureCreationDescription desc = *inputs[m_PinInput.m_uiInputIndex];
  desc.m_bAllowUAV = true;
  outputs[m_PinOutput.m_uiOutputIndex] = std::move(desc);

  return true;
}

void plColorGradingPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
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

    plUInt32 numLUTs = 0;
    plTexture3DResourceHandle luts[2] = {};
    float lutStrengths[2] = {};

    // Determine how many LUTs are active
    {
      if (m_hLUT1.IsValid())
      {
        luts[numLUTs] = m_hLUT1;
        lutStrengths[numLUTs] = m_fLut1Strength;
        numLUTs++;
      }

      if (m_hLUT2.IsValid())
      {
        luts[numLUTs] = m_hLUT2;
        lutStrengths[numLUTs] = m_fLut2Strength;
        numLUTs++;
      }
    }

    // Update shader constants
    {
      auto* constants = plRenderContext::GetConstantBufferData<plColorGradingConstants>(m_hConstantBuffer);
      constants->AutoExposureParams.SetZero();
      constants->MoodColor = m_MoodColor;
      constants->MoodStrength = m_fMoodStrength;
      constants->Saturation = m_fSaturation;
      constants->Lut1Strength = lutStrengths[0];
      constants->Lut2Strength = lutStrengths[1];

      // Pre-calculate factors of a s-shaped polynomial-function
      const float m = (0.5f - 0.5f * m_fContrast) / (0.5f + 0.5f * m_fContrast);
      const float a = 2.0f * m - 2.0f;
      const float b = -3.0f * m + 3.0f;

      constants->ContrastParams = plVec4(a, b, m, 0.0f);
    }

    renderViewContext.m_pRenderContext->BindShader(m_hShader);

    renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));
    renderViewContext.m_pRenderContext->BindTexture3D("Lut1Texture", luts[0]);
    renderViewContext.m_pRenderContext->BindTexture3D("Lut2Texture", luts[1]);

    plGALUnorderedAccessViewHandle hOutput;
    {
      plGALUnorderedAccessViewCreationDescription desc;
      desc.m_hTexture = pOutput->m_TextureHandle;
      desc.m_uiMipLevelToUse = 0;
      hOutput = pDevice->CreateUnorderedAccessView(desc);
    }

    renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
    renderViewContext.m_pRenderContext->BindConstantBuffer("plColorGradingConstants", m_hConstantBuffer);

    plTempHashedString sLUTModeValues[3] = {"LUT_MODE_NONE", "LUT_MODE_ONE", "LUT_MODE_TWO"};
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LUT_MODE", sLUTModeValues[numLUTs]);

    const plUInt32 uiWidth = pOutput->m_Desc.m_uiWidth;
    const plUInt32 uiHeight = pOutput->m_Desc.m_uiHeight;

    const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
    const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

    renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 6).IgnoreResult();
  }
  pDevice->EndPass(pPass);

  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading);
}

void plColorGradingPass::SetLUT1TextureFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hLUT1 = plResourceManager::LoadResource<plTexture3DResource>(szFile);
  }
}

const char* plColorGradingPass::GetLUT1TextureFile() const
{
  if (!m_hLUT1.IsValid())
    return "";

  return m_hLUT1.GetResourceID();
}

void plColorGradingPass::SetLUT2TextureFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hLUT2 = plResourceManager::LoadResource<plTexture3DResource>(szFile);
  }
}

const char* plColorGradingPass::GetLUT2TextureFile() const
{
  if (!m_hLUT2.IsValid())
    return "";

  return m_hLUT2.GetResourceID();
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ColorGradingPass);
