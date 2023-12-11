#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/TonemapPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/TonemapConstants.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTonemapPass, 1, plRTTIDefaultAllocator<plTonemapPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Color", m_PinColorInput),
    PLASMA_MEMBER_PROPERTY("Bloom", m_PinBloomInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_ACCESSOR_PROPERTY("VignettingTexture", GetVignettingTextureFile, SetVignettingTextureFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    PLASMA_MEMBER_PROPERTY("MoodColor", m_MoodColor)->AddAttributes(new plDefaultValueAttribute(plColor::Orange)),
    PLASMA_MEMBER_PROPERTY("MoodStrength", m_fMoodStrength)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new plClampValueAttribute(0.0f, 2.0f), new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_MEMBER_PROPERTY("LUT1Strength", m_fLut1Strength)->AddAttributes(new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_MEMBER_PROPERTY("LUT2Strength", m_fLut2Strength)->AddAttributes(new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_ACCESSOR_PROPERTY("LUT1", GetLUT1TextureFile, SetLUT1TextureFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
    PLASMA_ACCESSOR_PROPERTY("LUT2", GetLUT2TextureFile, SetLUT2TextureFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTonemapPass::plTonemapPass()
  : plRenderPipelinePass("TonemapPass", true)
{
  m_hVignettingTexture = plResourceManager::LoadResource<plTexture2DResource>("White.color");
  m_hNoiseTexture = plResourceManager::LoadResource<plTexture2DResource>("Textures/BlueNoise.dds");

  m_MoodColor = plColor::Orange;
  m_fMoodStrength = 0.0f;
  m_fSaturation = 1.0f;
  m_fContrast = 1.0f;
  m_fLut1Strength = 1.0f;
  m_fLut2Strength = 0.0f;

  m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/Tonemap.plShader");
  PLASMA_ASSERT_DEV(m_hShader.IsValid(), "Could not load tonemap shader!");

  m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plTonemapConstants>();
}

plTonemapPass::~plTonemapPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool plTonemapPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  const plGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

  // Color
  auto pColorInput = inputs[m_PinColorInput.m_uiInputIndex];
  if (pColorInput != nullptr)
  {
    if (const plGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]))
    {
      const plGALTextureCreationDescription& desc = pTexture->GetDescription();
      // if (desc.m_uiWidth != pColorInput->m_uiWidth || desc.m_uiHeight != pColorInput->m_uiHeight)
      //{
      //  plLog::Error("Render target sizes don't match");
      //  return false;
      //}

      outputs[m_PinOutput.m_uiOutputIndex].SetAsRenderTarget(pColorInput->m_uiWidth, pColorInput->m_uiHeight, desc.m_Format);
      outputs[m_PinOutput.m_uiOutputIndex].m_uiArraySize = pColorInput->m_uiArraySize;
    }
    else
    {
      plLog::Error("View '{0}' does not have a valid color target", view.GetName());
      return false;
    }
  }
  else
  {
    plLog::Error("No input connected to tone map pass!");
    return false;
  }

  return true;
}

void plTonemapPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinColorInput.m_uiInputIndex];
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorInput == nullptr || pColorOutput == nullptr)
  {
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // Setup render target
  plGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));

  // Bind render target and viewport
  auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  // Determine how many LUTs are active
  plUInt32 numLUTs = 0;
  plTexture3DResourceHandle luts[2] = {};
  float lutStrengths[2] = {};

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

  {
    plTonemapConstants* constants = plRenderContext::GetConstantBufferData<plTonemapConstants>(m_hConstantBuffer);
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

  plGALResourceViewHandle hBloomTextureView;
  auto pBloomInput = inputs[m_PinBloomInput.m_uiInputIndex];
  if (pBloomInput != nullptr)
  {
    hBloomTextureView = pDevice->GetDefaultResourceView(pBloomInput->m_TextureHandle);
  }

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindConstantBuffer("plTonemapConstants", m_hConstantBuffer);
  renderViewContext.m_pRenderContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("VignettingTexture", m_hVignettingTexture, plResourceAcquireMode::BlockTillLoaded);
  renderViewContext.m_pRenderContext->BindTexture2D("NoiseTexture", m_hNoiseTexture, plResourceAcquireMode::BlockTillLoaded);
  renderViewContext.m_pRenderContext->BindTexture2D("SceneColorTexture", pDevice->GetDefaultResourceView(pColorInput->m_TextureHandle));
  renderViewContext.m_pRenderContext->BindTexture2D("BloomTexture", hBloomTextureView);
  renderViewContext.m_pRenderContext->BindTexture3D("Lut1Texture", luts[0]);
  renderViewContext.m_pRenderContext->BindTexture3D("Lut2Texture", luts[1]);

  plTempHashedString sLUTModeValues[3] = {"LUT_MODE_NONE", "LUT_MODE_ONE", "LUT_MODE_TWO"};
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LUT_MODE", sLUTModeValues[numLUTs]);

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}

plResult plTonemapPass::Serialize(plStreamWriter& inout_stream) const
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  plStringBuilder sTemp = GetVignettingTextureFile();
  inout_stream << sTemp;
  inout_stream << m_MoodColor;
  inout_stream << m_fMoodStrength;
  inout_stream << m_fSaturation;
  inout_stream << m_fContrast;
  inout_stream << m_fLut1Strength;
  inout_stream << m_fLut2Strength;
  sTemp = GetLUT1TextureFile();
  inout_stream << sTemp;
  sTemp = GetLUT2TextureFile();
  inout_stream << sTemp;
  return PLASMA_SUCCESS;
}

plResult plTonemapPass::Deserialize(plStreamReader& inout_stream)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PLASMA_IGNORE_UNUSED(uiVersion);
  plStringBuilder sTemp;
  inout_stream >> sTemp;
  SetVignettingTextureFile(sTemp);
  inout_stream >> m_MoodColor;
  inout_stream >> m_fMoodStrength;
  inout_stream >> m_fSaturation;
  inout_stream >> m_fContrast;
  inout_stream >> m_fLut1Strength;
  inout_stream >> m_fLut2Strength;
  inout_stream >> sTemp;
  SetLUT1TextureFile(sTemp);
  inout_stream >> sTemp;
  SetLUT2TextureFile(sTemp);
  return PLASMA_SUCCESS;
}

void plTonemapPass::SetVignettingTextureFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hVignettingTexture = plResourceManager::LoadResource<plTexture2DResource>(szFile);
  }
}

const char* plTonemapPass::GetVignettingTextureFile() const
{
  if (!m_hVignettingTexture.IsValid())
    return "";

  return m_hVignettingTexture.GetResourceID();
}


void plTonemapPass::SetLUT1TextureFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hLUT1 = plResourceManager::LoadResource<plTexture3DResource>(szFile);
  }
}

const char* plTonemapPass::GetLUT1TextureFile() const
{
  if (!m_hLUT1.IsValid())
    return "";

  return m_hLUT1.GetResourceID();
}

void plTonemapPass::SetLUT2TextureFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hLUT2 = plResourceManager::LoadResource<plTexture3DResource>(szFile);
  }
}

const char* plTonemapPass::GetLUT2TextureFile() const
{
  if (!m_hLUT2.IsValid())
    return "";

  return m_hLUT2.GetResourceID();
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TonemapPass);