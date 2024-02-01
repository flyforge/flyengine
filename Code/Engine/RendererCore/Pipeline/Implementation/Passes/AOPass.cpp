#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/AOPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/DownscaleDepthConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/SSAOConstants.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAOPass, 1, plRTTIDefaultAllocator<plAOPass>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("DepthInput", m_PinDepthInput),
    PL_MEMBER_PROPERTY("Output", m_PinOutput),
    PL_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.01f, 10.0f)),
    PL_MEMBER_PROPERTY("MaxScreenSpaceRadius", m_fMaxScreenSpaceRadius)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.01f, 2.0f)),
    PL_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new plDefaultValueAttribute(2.0f)),
    PL_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new plDefaultValueAttribute(0.7f)),
    PL_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new plDefaultValueAttribute(80.0f), new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("FadeOutEnd", GetFadeOutEnd, SetFadeOutEnd)->AddAttributes(new plDefaultValueAttribute(100.0f), new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("PositionBias", m_fPositionBias)->AddAttributes(new plDefaultValueAttribute(5.0f), new plClampValueAttribute(0.0f, 1000.0f)),
    PL_MEMBER_PROPERTY("MipLevelScale", m_fMipLevelScale)->AddAttributes(new plDefaultValueAttribute(10.0f), new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("DepthBlurThreshold", m_fDepthBlurThreshold)->AddAttributes(new plDefaultValueAttribute(2.0f), new plClampValueAttribute(0.01f, plVariant())),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAOPass::plAOPass()
  : plRenderPipelinePass("AOPass", true)

{
  m_hNoiseTexture = plResourceManager::LoadResource<plTexture2DResource>("Textures/SSAONoise.dds");

  m_hDownscaleShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/DownscaleDepth.plShader");
  PL_ASSERT_DEV(m_hDownscaleShader.IsValid(), "Could not load downsample shader!");

  m_hSSAOShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/SSAO.plShader");
  PL_ASSERT_DEV(m_hSSAOShader.IsValid(), "Could not load SSAO shader!");

  m_hBlurShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/SSAOBlur.plShader");
  PL_ASSERT_DEV(m_hBlurShader.IsValid(), "Could not load SSAO shader!");

  m_hDownscaleConstantBuffer = plRenderContext::CreateConstantBufferStorage<plDownscaleDepthConstants>();
  m_hSSAOConstantBuffer = plRenderContext::CreateConstantBufferStorage<plSSAOConstants>();
}

plAOPass::~plAOPass()
{
  if (!m_hSSAOSamplerState.IsInvalidated())
  {
    plGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSSAOSamplerState);
    m_hSSAOSamplerState.Invalidate();
  }

  plRenderContext::DeleteConstantBufferStorage(m_hDownscaleConstantBuffer);
  m_hDownscaleConstantBuffer.Invalidate();

  plRenderContext::DeleteConstantBufferStorage(m_hSSAOConstantBuffer);
  m_hSSAOConstantBuffer.Invalidate();
}

bool plAOPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  if (auto pDepthInput = inputs[m_PinDepthInput.m_uiInputIndex])
  {
    if (!pDepthInput->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' input must allow shader resource view.", GetName());
      return false;
    }

    if (pDepthInput->m_SampleCount != plGALMSAASampleCount::None)
    {
      plLog::Error("'{0}' input must be resolved", GetName());
      return false;
    }

    plGALTextureCreationDescription desc = *pDepthInput;
    desc.m_Format = plGALResourceFormat::RGHalf;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    plLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void plAOPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  auto pDepthInput = inputs[m_PinDepthInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pDepthInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plGALPass* pGALPass = pDevice->BeginPass(GetName());
  PL_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  plUInt32 uiWidth = pDepthInput->m_Desc.m_uiWidth;
  plUInt32 uiHeight = pDepthInput->m_Desc.m_uiHeight;

  plUInt32 uiNumMips = 3;
  plUInt32 uiHzbWidth = plMath::RoundUp(uiWidth, 1u << uiNumMips);
  plUInt32 uiHzbHeight = plMath::RoundUp(uiHeight, 1u << uiNumMips);

  float fHzbScaleX = (float)uiWidth / uiHzbWidth;
  float fHzbScaleY = (float)uiHeight / uiHzbHeight;

  // Find temp targets
  plGALTextureHandle hzbTexture;
  plHybridArray<plVec2, 8> hzbSizes;
  plHybridArray<plGALResourceViewHandle, 8> hzbResourceViews;
  plHybridArray<plGALRenderTargetViewHandle, 8> hzbRenderTargetViews;

  plGALTextureHandle tempSSAOTexture;

  {
    {
      plGALTextureCreationDescription desc;
      desc.m_uiWidth = uiHzbWidth / 2;
      desc.m_uiHeight = uiHzbHeight / 2;
      desc.m_uiMipLevelCount = 3;
      desc.m_Type = plGALTextureType::Texture2D;
      desc.m_Format = plGALResourceFormat::RHalf;
      desc.m_bCreateRenderTarget = true;
      desc.m_bAllowShaderResourceView = true;
      desc.m_uiArraySize = pOutput->m_Desc.m_uiArraySize;

      hzbTexture = plGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);
    }

    for (plUInt32 i = 0; i < uiNumMips; ++i)
    {
      uiHzbWidth = uiHzbWidth / 2;
      uiHzbHeight = uiHzbHeight / 2;

      hzbSizes.PushBack(plVec2((float)uiHzbWidth, (float)uiHzbHeight));

      {
        plGALResourceViewCreationDescription desc;
        desc.m_hTexture = hzbTexture;
        desc.m_uiMostDetailedMipLevel = i;
        desc.m_uiMipLevelsToUse = 1;
        desc.m_uiArraySize = pOutput->m_Desc.m_uiArraySize;

        hzbResourceViews.PushBack(pDevice->CreateResourceView(desc));
      }

      {
        plGALRenderTargetViewCreationDescription desc;
        desc.m_hTexture = hzbTexture;
        desc.m_uiMipLevel = i;
        desc.m_uiSliceCount = pOutput->m_Desc.m_uiArraySize;

        hzbRenderTargetViews.PushBack(pDevice->CreateRenderTargetView(desc));
      }
    }

    tempSSAOTexture = plGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, plGALResourceFormat::RGHalf, plGALMSAASampleCount::None, pOutput->m_Desc.m_uiArraySize);
  }

  // Mip map passes
  {
    CreateSamplerState();

    for (plUInt32 i = 0; i < uiNumMips; ++i)
    {
      plGALResourceViewHandle hInputView;
      plVec2 pixelSize;

      if (i == 0)
      {
        hInputView = pDevice->GetDefaultResourceView(pDepthInput->m_TextureHandle);
        pixelSize = plVec2(1.0f / uiWidth, 1.0f / uiHeight);
      }
      else
      {
        hInputView = hzbResourceViews[i - 1];
        pixelSize = plVec2(1.0f).CompDiv(hzbSizes[i - 1]);
      }

      plGALRenderTargetViewHandle hOutputView = hzbRenderTargetViews[i];
      plVec2 targetSize = hzbSizes[i];

      plGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, hOutputView);
      renderViewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, plRectFloat(targetSize.x, targetSize.y), "SSAOMipMaps", renderViewContext.m_pCamera->IsStereoscopic());

      plDownscaleDepthConstants* constants = plRenderContext::GetConstantBufferData<plDownscaleDepthConstants>(m_hDownscaleConstantBuffer);
      constants->PixelSize = pixelSize;
      constants->LinearizeDepth = (i == 0);

      renderViewContext.m_pRenderContext->BindConstantBuffer("plDownscaleDepthConstants", m_hDownscaleConstantBuffer);
      renderViewContext.m_pRenderContext->BindShader(m_hDownscaleShader);

      renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", hInputView);
      renderViewContext.m_pRenderContext->BindSamplerState("DepthSampler", m_hSSAOSamplerState);

      renderViewContext.m_pRenderContext->BindNullMeshBuffer(plGALPrimitiveTopology::Triangles, 1);

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();
    }
  }

  // Update constants
  {
    float fadeOutScale = -1.0f / plMath::Max(0.001f, (m_fFadeOutEnd - m_fFadeOutStart));
    float fadeOutOffset = -fadeOutScale * m_fFadeOutStart + 1.0f;

    plSSAOConstants* constants = plRenderContext::GetConstantBufferData<plSSAOConstants>(m_hSSAOConstantBuffer);
    constants->TexCoordsScale = plVec2(fHzbScaleX, fHzbScaleY);
    constants->FadeOutParams = plVec2(fadeOutScale, fadeOutOffset);
    constants->WorldRadius = m_fRadius;
    constants->MaxScreenSpaceRadius = m_fMaxScreenSpaceRadius;
    constants->Contrast = m_fContrast;
    constants->Intensity = m_fIntensity;
    constants->PositionBias = m_fPositionBias / 1000.0f;
    constants->MipLevelScale = m_fMipLevelScale;
    constants->DepthBlurScale = 1.0f / m_fDepthBlurThreshold;
  }

  // SSAO pass
  {
    plGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(tempSSAOTexture));
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "SSAO", renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindConstantBuffer("plSSAOConstants", m_hSSAOConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hSSAOShader);

    renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pDevice->GetDefaultResourceView(pDepthInput->m_TextureHandle));
    renderViewContext.m_pRenderContext->BindTexture2D("LowResDepthTexture", pDevice->GetDefaultResourceView(hzbTexture));
    renderViewContext.m_pRenderContext->BindSamplerState("DepthSampler", m_hSSAOSamplerState);

    renderViewContext.m_pRenderContext->BindTexture2D("NoiseTexture", m_hNoiseTexture, plResourceAcquireMode::BlockTillLoaded);

    renderViewContext.m_pRenderContext->BindNullMeshBuffer(plGALPrimitiveTopology::Triangles, 1);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // Blur pass
  {
    plGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "Blur", renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindConstantBuffer("plSSAOConstants", m_hSSAOConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hBlurShader);

    renderViewContext.m_pRenderContext->BindTexture2D("SSAOTexture", pDevice->GetDefaultResourceView(tempSSAOTexture));

    renderViewContext.m_pRenderContext->BindNullMeshBuffer(plGALPrimitiveTopology::Triangles, 1);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // Return temp targets
  if (!hzbTexture.IsInvalidated())
  {
    plGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hzbTexture);
  }

  if (!tempSSAOTexture.IsInvalidated())
  {
    plGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(tempSSAOTexture);
  }
}

void plAOPass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
  {
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_ClearColor = plColor::White;

  auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());
}

plResult plAOPass::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fRadius;
  inout_stream << m_fMaxScreenSpaceRadius;
  inout_stream << m_fContrast;
  inout_stream << m_fIntensity;
  inout_stream << m_fFadeOutStart;
  inout_stream << m_fFadeOutEnd;
  inout_stream << m_fPositionBias;
  inout_stream << m_fMipLevelScale;
  inout_stream << m_fDepthBlurThreshold;
  return PL_SUCCESS;
}

plResult plAOPass::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fRadius;
  inout_stream >> m_fMaxScreenSpaceRadius;
  inout_stream >> m_fContrast;
  inout_stream >> m_fIntensity;
  inout_stream >> m_fFadeOutStart;
  inout_stream >> m_fFadeOutEnd;
  inout_stream >> m_fPositionBias;
  inout_stream >> m_fMipLevelScale;
  inout_stream >> m_fDepthBlurThreshold;
  return PL_SUCCESS;
}

void plAOPass::SetFadeOutStart(float fStart)
{
  m_fFadeOutStart = plMath::Clamp(fStart, 0.0f, m_fFadeOutEnd);
}

float plAOPass::GetFadeOutStart() const
{
  return m_fFadeOutStart;
}

void plAOPass::SetFadeOutEnd(float fEnd)
{
  if (m_fFadeOutEnd == fEnd)
    return;

  m_fFadeOutEnd = plMath::Max(fEnd, m_fFadeOutStart);

  if (!m_hSSAOSamplerState.IsInvalidated())
  {
    plGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSSAOSamplerState);
    m_hSSAOSamplerState.Invalidate();
  }
}

float plAOPass::GetFadeOutEnd() const
{
  return m_fFadeOutEnd;
}

void plAOPass::CreateSamplerState()
{
  if (m_hSSAOSamplerState.IsInvalidated())
  {
    plGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = plGALTextureFilterMode::Point;
    desc.m_MagFilter = plGALTextureFilterMode::Point;
    desc.m_MipFilter = plGALTextureFilterMode::Point;
    desc.m_AddressU = plImageAddressMode::ClampBorder;
    desc.m_AddressV = plImageAddressMode::ClampBorder;
    desc.m_AddressW = plImageAddressMode::ClampBorder;
    desc.m_BorderColor = plColor::White * m_fFadeOutEnd;

    m_hSSAOSamplerState = plGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }
}

PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_AOPass);
