#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/UpscalePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#define A_CPU
#include "../../../../Data/Base/Shaders/ThirdParty/FFX/ffx_a.h"
#include "../../../../Data/Base/Shaders/ThirdParty/FFX/ffx_fsr1.h"

#include "../../../../Data/Base/Shaders/Pipeline/UpscaleConstants.h"

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plUpscaleMode, 1)
  PLASMA_ENUM_CONSTANT(plUpscaleMode::FSR),
  PLASMA_ENUM_CONSTANT(plUpscaleMode::BiLinear)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plFSRUpscalePreset, 1)
  PLASMA_ENUM_CONSTANT(plFSRUpscalePreset::UltraQuality),
  PLASMA_ENUM_CONSTANT(plFSRUpscalePreset::Quality),
  PLASMA_ENUM_CONSTANT(plFSRUpscalePreset::Balanced),
  PLASMA_ENUM_CONSTANT(plFSRUpscalePreset::Performance),
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plUpscalePass, 1, plRTTIDefaultAllocator<plUpscalePass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Input", m_PinInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_ENUM_MEMBER_PROPERTY("Mode", plUpscaleMode, m_eUpscaleMode),
    PLASMA_ENUM_MEMBER_PROPERTY("FSR_Preset", plFSRUpscalePreset, m_eFSRPreset),
    PLASMA_MEMBER_PROPERTY("FSR_Sharpen", m_bFSRSharpen),
    PLASMA_MEMBER_PROPERTY("FSR_Sharpness", m_fFSRSharpness)->AddAttributes(new plDefaultValueAttribute(0.2f), new plClampValueAttribute(0.0f, 2.0f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


plUpscalePass::plUpscalePass()
  : plRenderPipelinePass("UpscalePass", false)
  , m_eUpscaleMode(plUpscaleMode::FSR)
  , m_eFSRPreset(plFSRUpscalePreset::UltraQuality)
  , m_bFSRSharpen(true)
  , m_fFSRSharpness(0.2f)
{
  // Load shader
  {
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/Upscale.plShader");
    PLASMA_ASSERT_DEV(m_hShader.IsValid(), "Could not load Luminance Pass shader!");
  }

  // Load resources
  {
    m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plUpscaleConstants>();
  }
}

plUpscalePass::~plUpscalePass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool plUpscalePass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    if (!inputs[m_PinInput.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' Color input must allow shader resource view.", GetName());
      return false;
    }
  }
  else
  {
    plLog::Error("No Color input connected to '{0}'!", GetName());
    return false;
  }

  // Output has the window resolution
  {
    plGALTextureCreationDescription desc = *inputs[m_PinInput.m_uiInputIndex];
    desc.m_uiWidth = plMath::Max(static_cast<plUInt32>(view.GetTargetViewport().width), desc.m_uiWidth);
    desc.m_uiHeight = plMath::Max(static_cast<plUInt32>(view.GetTargetViewport().height), desc.m_uiHeight);
    desc.m_bAllowUAV = true;
    desc.m_bCreateRenderTarget = true;
    outputs[m_PinOutput.m_uiOutputIndex] = std::move(desc);
  }

  return true;
}

void plUpscalePass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  // AMD FidelityFX Super Resolution 1.0 (FSR1).
  // Provides high-quality solution designed to produce high resolution frames from lower resolution inputs.
  // GitHub:        https://github.com/GPUOpen-Effects/FidelityFX-FSR
  // Documentation: https://github.com/GPUOpen-Effects/FidelityFX-FSR/blob/master/docs/FidelityFX-FSR-Overview-Integration.pdf

  const auto* const pInput = inputs[m_PinInput.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pInput == nullptr || pOutput == nullptr)
    return;

  // Early exit - Nothing to upscale if render resolution = target resolution
  if (renderViewContext.m_pViewData->m_ViewPortRect == renderViewContext.m_pViewData->m_TargetViewportRect)
    return;

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plResourceManager::ForceLoadResourceNow(m_hShader);

  const bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  const plUInt32 uiWidth = pOutput->m_Desc.m_uiWidth;
  const plUInt32 uiHeight = pOutput->m_Desc.m_uiHeight;

  // As per documentation - Page 23
  static constexpr int threadGroupWorkRegionDim = 16;
  const plUInt32 uiDispatchX = (uiWidth + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
  const plUInt32 uiDispatchY = (uiHeight + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

  plGALPass* pPass = pDevice->BeginPass(GetName());
  {
    AU1 const0[4];
    AU1 const1[4];
    AU1 const2[4];
    AU1 const3[4];

    FsrEasuCon(
      const0, const1, const2, const3, // FSR constants
      static_cast<AF1>(renderViewContext.m_pViewData->m_ViewPortRect.width),
      static_cast<AF1>(renderViewContext.m_pViewData->m_ViewPortRect.height), // Render resolution
      static_cast<AF1>(renderViewContext.m_pViewData->m_ViewPortRect.width),
      static_cast<AF1>(renderViewContext.m_pViewData->m_ViewPortRect.height), // Input Texture resolution, same as render resolution in our case
      static_cast<AF1>(renderViewContext.m_pViewData->m_TargetViewportRect.width),
      static_cast<AF1>(renderViewContext.m_pViewData->m_TargetViewportRect.height) // Target (window) resolution
    );

    plGALUnorderedAccessViewHandle hOutput;
    {
      plGALUnorderedAccessViewCreationDescription desc;
      desc.m_hTexture = pOutput->m_TextureHandle;
      desc.m_uiMipLevelToUse = 0;
      hOutput = pDevice->CreateUnorderedAccessView(desc);
    }

    plGALSamplerStateHandle hSamplerState;
    {
      plGALSamplerStateCreationDescription desc;
      desc.m_MinFilter = plGALTextureFilterMode::Linear;
      desc.m_MagFilter = plGALTextureFilterMode::Linear;
      desc.m_MipFilter = plGALTextureFilterMode::Point;
      desc.m_AddressU = plImageAddressMode::Clamp;
      desc.m_AddressV = plImageAddressMode::Clamp;
      desc.m_AddressW = plImageAddressMode::Clamp;
      desc.m_uiMaxAnisotropy = 1;
      desc.m_fMipLodBias = -plMath::Log2(static_cast<float>(pOutput->m_Desc.m_uiWidth) / static_cast<float>(pInput->m_Desc.m_uiWidth));
      hSamplerState = pDevice->CreateSamplerState(desc);
    }

    if (m_eUpscaleMode == plUpscaleMode::FSR)
    {
      plGALTextureHandle hIntermediaryTexture;
      plGALUnorderedAccessViewHandle hIntermediaryOutput;

      if (m_bFSRSharpen)
      {
        {
          plGALTextureCreationDescription desc;
          desc.m_uiHeight = uiHeight;
          desc.m_uiWidth = uiWidth;
          desc.m_Format = pOutput->m_Desc.m_Format;
          desc.m_bAllowShaderResourceView = true;
          desc.m_bAllowUAV = true;
          desc.m_Type = plGALTextureType::Texture2D;
          desc.m_ResourceAccess.m_bImmutable = false;
          hIntermediaryTexture = pDevice->CreateTexture(desc);
        }

        {
          plGALUnorderedAccessViewCreationDescription desc;
          desc.m_hTexture = hIntermediaryTexture;
          desc.m_uiMipLevelToUse = 0;
          hIntermediaryOutput = pDevice->CreateUnorderedAccessView(desc);
        }
      }

      // Upscale Pass
      {
        auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "FSR Upscale");

        renderViewContext.m_pRenderContext->BindShader(m_hShader);

        renderViewContext.m_pRenderContext->BindUAV("Output", m_bFSRSharpen ? hIntermediaryOutput : hOutput);
        renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));
        renderViewContext.m_pRenderContext->BindSamplerState("UpscaleSampler", hSamplerState);
        renderViewContext.m_pRenderContext->BindConstantBuffer("plUpscaleConstants", m_hConstantBuffer);

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("UPSCALE_MODE", "UPSCALE_MODE_FSR_UPSCALE");

        auto* constants = plRenderContext::GetConstantBufferData<plUpscaleConstants>(m_hConstantBuffer);
        plMemoryUtils::Copy(constants->Const0.GetData(), &const0[0], 4);
        plMemoryUtils::Copy(constants->Const1.GetData(), &const1[0], 4);
        plMemoryUtils::Copy(constants->Const2.GetData(), &const2[0], 4);
        plMemoryUtils::Copy(constants->Const3.GetData(), &const3[0], 4);

        renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
      }

      if (m_bFSRSharpen)
      // Sharpen Pass
      {
        FsrRcasCon(const0, m_fFSRSharpness);

        auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "FSR Sharpen");

        renderViewContext.m_pRenderContext->BindShader(m_hShader);

        renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
        renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(hIntermediaryTexture));
        renderViewContext.m_pRenderContext->BindSamplerState("UpscaleSampler", hSamplerState);
        renderViewContext.m_pRenderContext->BindConstantBuffer("plUpscaleConstants", m_hConstantBuffer);

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("UPSCALE_MODE", "UPSCALE_MODE_FSR_SHARPEN");

        auto* constants = plRenderContext::GetConstantBufferData<plUpscaleConstants>(m_hConstantBuffer);
        plMemoryUtils::Copy(constants->Const0.GetData(), &const0[0], 4);

        renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();

        pDevice->DestroyTexture(hIntermediaryTexture);
      }
    }
    else
    {
      auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "BiLinear Upscale");

      renderViewContext.m_pRenderContext->BindShader(m_hShader);

      renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
      renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindSamplerState("UpscaleSampler", hSamplerState);
      renderViewContext.m_pRenderContext->BindConstantBuffer("plUpscaleConstants", m_hConstantBuffer);

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("UPSCALE_MODE", "UPSCALE_MODE_BILINEAR");

      auto* constants = plRenderContext::GetConstantBufferData<plUpscaleConstants>(m_hConstantBuffer);
      plMemoryUtils::Copy(constants->Const0.GetData(), &const0[0], 4);
      plMemoryUtils::Copy(constants->Const1.GetData(), &const1[0], 4);
      plMemoryUtils::Copy(constants->Const2.GetData(), &const2[0], 4);
      plMemoryUtils::Copy(constants->Const3.GetData(), &const3[0], 4);

      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
    }

    // Cleanup
    pDevice->DestroySamplerState(hSamplerState);
  }
  pDevice->EndPass(pPass);

  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading);
}

void plUpscalePass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInput = inputs[m_PinInput.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  auto pCommandEncoder = plRenderContext::BeginPassAndComputeScope(renderViewContext, GetName());

  pCommandEncoder->CopyTexture(pOutput->m_TextureHandle, pInput->m_TextureHandle);
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_UpscalePass);
