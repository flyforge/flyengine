#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/BloomPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include "../../../../../../Data/Base/Shaders/Pipeline/BloomConstants.h"
#include "../../../../../../Data/Base/Shaders/Pipeline/AmdSPDConstants.h"

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plBloomPass, 2, plRTTIDefaultAllocator<plBloomPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Input", m_PinInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new plDefaultValueAttribute(0.3f)),
    PLASMA_MEMBER_PROPERTY("BloomThreshold", m_fBloomThreshold)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("MipCount", m_uiMipCount)->AddAttributes(new plClampValueAttribute(1, 12), new plDefaultValueAttribute(6)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plBloomPass::plBloomPass()
  : plRenderPipelinePass("BloomPass", true)
  , m_fIntensity(0.3f)
  , m_fBloomThreshold(1.0f)
  , m_uiMipCount(6)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // Load bloom shader.
  {
    m_hBloomShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/Bloom.plShader");
    PLASMA_ASSERT_DEV(m_hBloomShader.IsValid(), "Could not load bloom shader!");
  }

    // Load spd shader.
  {
    m_hDownscaleShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/AmdSPD.plShader");
    PLASMA_ASSERT_DEV(m_hDownscaleShader.IsValid(), "Could not load spd shader!");
  }

  // Load resources.
  {
    m_hBloomConstantBuffer = plRenderContext::CreateConstantBufferStorage<plBloomConstants>();
    m_hSPDConstantBuffer = plRenderContext::CreateConstantBufferStorage<plAmdSPDConstants>();

    plGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(plUInt32);
    desc.m_uiTotalSize = desc.m_uiStructSize;
    desc.m_BufferType = plGALBufferType::Generic;
    desc.m_bAllowUAV = true;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_DownsampleAtomicCounter = PLASMA_NEW_ARRAY(plAlignedAllocatorWrapper::GetAllocator(), plUInt32, 1);
    m_DownsampleAtomicCounter[0] = 0;

    m_hDownsampleAtomicCounterBuffer = pDevice->CreateBuffer(desc, m_DownsampleAtomicCounter.ToByteArray());
  }
}

plBloomPass::~plBloomPass()
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  pDevice->DestroyBuffer(m_hDownsampleAtomicCounterBuffer);

  PLASMA_DELETE_ARRAY(plAlignedAllocatorWrapper::GetAllocator(), m_DownsampleAtomicCounter);
  m_hDownsampleAtomicCounterBuffer.Invalidate();

  plRenderContext::DeleteConstantBufferStorage(m_hBloomConstantBuffer);
  m_hBloomConstantBuffer.Invalidate();

  plRenderContext::DeleteConstantBufferStorage(m_hSPDConstantBuffer);
  m_hSPDConstantBuffer.Invalidate();
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
  }
  else
  {
    plLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  // Bloom Output
  {
    plGALTextureCreationDescription desc = *inputs[m_PinInput.m_uiInputIndex];
    desc.m_bAllowUAV = true;
    desc.m_bAllowShaderResourceView = true;

    outputs[m_PinOutput.m_uiOutputIndex] = std::move(desc);
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  {
    plGALTextureCreationDescription desc = *inputs[m_PinInput.m_uiInputIndex];
    desc.m_uiMipLevelCount = m_uiMipCount;
    desc.m_bAllowUAV = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_Format = plGALResourceFormat::RG11B10Float;
    m_hBloomTexture = pDevice->CreateTexture(desc);
  }

  return true;
}

void plBloomPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInput = inputs[m_PinInput.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  plTempHashedString sLuminancePass = "BLOOM_PASS_MODE_LUMINANCE";
  plTempHashedString sUpscaleBlendPass = "BLOOM_PASS_MODE_UPSCALE_BLEND_MIP";
  plTempHashedString sColorBlendPass = "BLOOM_PASS_MODE_BLEND_FRAME";

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plResourceManager::ForceLoadResourceNow(m_hBloomShader);

  const bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  plGALPass* pPass = pDevice->BeginPass(GetName());
  PLASMA_SCOPE_EXIT(
    pDevice->EndPass(pPass);
    renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading););

  const plUInt32 uiWidth = pOutput->m_Desc.m_uiWidth;
  const plUInt32 uiHeight = pOutput->m_Desc.m_uiHeight;

  // Luminance pass
  {
    auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "Luminance");

    renderViewContext.m_pRenderContext->BindShader(m_hBloomShader);

    plGALUnorderedAccessViewHandle hLuminanceOutput;
    {
      plGALUnorderedAccessViewCreationDescription desc;
      desc.m_hTexture = m_hBloomTexture;
      desc.m_uiMipLevelToUse = 0;
      hLuminanceOutput = pDevice->CreateUnorderedAccessView(desc);
    }

    renderViewContext.m_pRenderContext->BindUAV("Output", hLuminanceOutput);
    renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));
    renderViewContext.m_pRenderContext->BindConstantBuffer("plBloomConstants", m_hBloomConstantBuffer);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", sLuminancePass);

    const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
    const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

    UpdateBloomConstantBuffer(plVec2(1.0f / static_cast<float>(uiWidth), 1.0f / static_cast<float>(uiHeight)));

    renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
  }

  // Downsample pass
  {
    // AMD FidelityFX Single Pass Downsampler.
    // Provides an RDNA™-optimized solution for generating up to 12 MIP levels of a texture.
    // GitHub:        https://github.com/GPUOpen-Effects/FidelityFX-SPD
    // Documentation: https://github.com/GPUOpen-Effects/FidelityFX-SPD/blob/master/docs/FidelityFX_SPD.pdf

    const plUInt32 uiOutputMipCount = m_uiMipCount - 1;
    const plUInt32 uiSmallestWidth = uiWidth >> uiOutputMipCount;
    const plUInt32 uiSmallestHeight = uiWidth >> uiOutputMipCount;

    // Ensure that the input texture meets the requirements.
    PLASMA_ASSERT_DEV(uiOutputMipCount + 1 <= 12, "AMD FidelityFX Single Pass Downsampler can't generate more than 12 mipmap levels."); // As per documentation (page 22)

    auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "Downsample");

    renderViewContext.m_pRenderContext->BindShader(m_hDownscaleShader);

    plGALResourceViewHandle hBloomInput;
    {
      plGALResourceViewCreationDescription desc;
      desc.m_hTexture = m_hBloomTexture;
      desc.m_uiMostDetailedMipLevel = 0;
      desc.m_uiMipLevelsToUse = 1;
      hBloomInput = pDevice->CreateResourceView(desc);
    }

    renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", hBloomInput);

    renderViewContext.m_pRenderContext->BindConstantBuffer("plAmdSPDConstants", m_hSPDConstantBuffer);

    plGALUnorderedAccessViewHandle hAtomicCounter;
    {
      plGALUnorderedAccessViewCreationDescription desc;
      desc.m_OverrideViewFormat = plGALResourceFormat::RUInt;
      desc.m_hBuffer = m_hDownsampleAtomicCounterBuffer;
      desc.m_uiNumElements = 1;
      desc.m_uiFirstElement = 0;
      hAtomicCounter = pDevice->CreateUnorderedAccessView(desc);
    }

    renderViewContext.m_pRenderContext->BindUAV("AtomicCounter", hAtomicCounter);

    for (plUInt32 i = 0; i < uiOutputMipCount; ++i)
    {
      plStringBuilder sSlotName;
      sSlotName.Format("DownsampleOutput[{}]", i);

      plGALUnorderedAccessViewHandle hDownsampleOutput;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hBloomTexture;
        desc.m_uiMipLevelToUse = i + 1;
        hDownsampleOutput = pDevice->CreateUnorderedAccessView(desc);
      }

      renderViewContext.m_pRenderContext->BindUAV(sSlotName.GetView(), hDownsampleOutput);
    }

    // As per documentation (page 22)
    const plUInt32 uiDispatchX = (uiWidth + 63) >> 6;
    const plUInt32 uiDispatchY = (uiHeight + 63) >> 6;

    UpdateSPDConstantBuffer(uiDispatchX * uiDispatchY);

    renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
  }

  // Upsample and Blend pass
  {
    for (plUInt32 i = m_uiMipCount - 1; i > 0; --i)
    {
      plStringBuilder sb;
      auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, plFmt("UpscaleBlend Mip {}", i).GetTextCStr(sb));

      renderViewContext.m_pRenderContext->BindShader(m_hBloomShader);

      plUInt32 uiMipSmall = i;
      plUInt32 uiMipLarge = i - 1;

      plUInt32 uiMipLargeWidth = uiWidth >> uiMipLarge;
      plUInt32 uiMipLargeHeight = uiHeight >> uiMipLarge;

      plGALResourceViewHandle hUpscaleBlendInput;
      {
        plGALResourceViewCreationDescription desc;
        desc.m_hTexture = m_hBloomTexture;
        desc.m_uiMostDetailedMipLevel = uiMipSmall;
        desc.m_uiMipLevelsToUse = 1;
        hUpscaleBlendInput = pDevice->CreateResourceView(desc);
      }

      plGALUnorderedAccessViewHandle hUpscaleBlendOutput;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hBloomTexture;
        desc.m_uiMipLevelToUse = uiMipLarge;
        hUpscaleBlendOutput = pDevice->CreateUnorderedAccessView(desc);
      }

      renderViewContext.m_pRenderContext->BindUAV("Output", hUpscaleBlendOutput);
      renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", hUpscaleBlendInput);
      renderViewContext.m_pRenderContext->BindConstantBuffer("plBloomConstants", m_hBloomConstantBuffer);

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", sUpscaleBlendPass);

      const auto uiDispatchX = static_cast<plUInt32>(plMath::Ceil(static_cast<float>(uiMipLargeWidth) / POSTPROCESS_BLOCKSIZE ));
      const auto uiDispatchY = static_cast<plUInt32>(plMath::Ceil(static_cast<float>(uiMipLargeHeight) / POSTPROCESS_BLOCKSIZE ));

      UpdateBloomConstantBuffer(plVec2(1.0f / static_cast<float>(uiMipLargeWidth), 1.0f / static_cast<float>(uiMipLargeHeight)));

      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
    }
  }

  // Color Blend
  {
    auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "ColorBlend");

    renderViewContext.m_pRenderContext->BindShader(m_hBloomShader);

    plGALResourceViewHandle hBloomInput;
    {
      plGALResourceViewCreationDescription desc;
      desc.m_hTexture = m_hBloomTexture;
      desc.m_uiMostDetailedMipLevel = 0;
      desc.m_uiMipLevelsToUse = 1;
      hBloomInput = pDevice->CreateResourceView(desc);
    }

    plGALUnorderedAccessViewHandle hColorBlendOutput;
    {
      plGALUnorderedAccessViewCreationDescription desc;
      desc.m_hTexture = pOutput->m_TextureHandle;
      desc.m_uiMipLevelToUse = 0;
      hColorBlendOutput = pDevice->CreateUnorderedAccessView(desc);
    }

    renderViewContext.m_pRenderContext->BindUAV("Output", hColorBlendOutput);
    renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));
    renderViewContext.m_pRenderContext->BindTexture2D("MipTexture", hBloomInput);
    renderViewContext.m_pRenderContext->BindConstantBuffer("plBloomConstants", m_hBloomConstantBuffer);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", sColorBlendPass);

    const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
    const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

    UpdateBloomConstantBuffer(plVec2(1.0f / static_cast<float>(uiWidth), 1.0f / static_cast<float>(uiHeight)));

    renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
  }
}

void plBloomPass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
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

void plBloomPass::UpdateBloomConstantBuffer(plVec2 pixelSize) const
{
  auto* constants = plRenderContext::GetConstantBufferData<plBloomConstants>(m_hBloomConstantBuffer);
  constants->PixelSize = pixelSize;
  constants->BloomIntensity = m_fIntensity;
  constants->BloomThreshold = m_fBloomThreshold;
}

void plBloomPass::UpdateSPDConstantBuffer(plUInt32 uiWorkGroupCount)
{
  auto constants = plRenderContext::GetConstantBufferData<plAmdSPDConstants>(m_hSPDConstantBuffer);
  constants->MipCount = m_uiMipCount;
  constants->WorkGroupCount = uiWorkGroupCount;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plBloomPassPatch_1_2 : public plGraphPatch
{
public:
  plBloomPassPatch_1_2()
    : plGraphPatch("plBloomPass", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("BloomThreshold", 1);
    pNode->AddProperty("MipCount", 6);
  }
};

plBloomPassPatch_1_2 g_plBloomPassPatch_1_2;

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_BloomPass);
