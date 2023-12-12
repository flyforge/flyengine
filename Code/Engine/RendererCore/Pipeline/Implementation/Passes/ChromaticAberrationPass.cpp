#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/ChromaticAberrationPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/Texture.h>

#include "../../../../../../Data/Base/Shaders/Pipeline/ChromaticAberrationConstants.h"

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plChromaticAberrationPass, 1, plRTTIDefaultAllocator<plChromaticAberrationPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Input", m_PinInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_ACCESSOR_PROPERTY("OffsetTexture", GetOffsetTextureFile, SetOffsetTextureFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    PLASMA_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, {})),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plChromaticAberrationPass::plChromaticAberrationPass()
  : plRenderPipelinePass("ChromaticAberrationPass", true)
  , m_fStrength(1.0f)
{
  // Load shader.
  {
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/ChromaticAberration.plShader");
    PLASMA_ASSERT_DEV(m_hShader.IsValid(), "Failed to load shader 'ChromaticAberration.plShader'.");
  }

  // Load resources.
  {
    m_hOffsetTexture = plResourceManager::LoadResource<plTexture2DResource>("Black.color");
    m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plChromaticAberrationConstants>();
  }
}

plChromaticAberrationPass::~plChromaticAberrationPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool plChromaticAberrationPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    if (!inputs[m_PinInput.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' input must allow shader resource view.", GetName());
      return false;
    }

    {
      plGALTextureCreationDescription desc = *inputs[m_PinInput.m_uiInputIndex];
      desc.m_bAllowUAV = true;
      desc.m_bCreateRenderTarget = true;
      outputs[m_PinOutput.m_uiOutputIndex] = std::move(desc);
    }
  }
  else
  {
    plLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void plChromaticAberrationPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* pColorInput = inputs[m_PinInput.m_uiInputIndex];
  const auto* pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorInput == nullptr || pColorOutput == nullptr)
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

    const plGALResourceViewHandle hInput = pDevice->GetDefaultResourceView(pColorInput->m_TextureHandle);
    plGALUnorderedAccessViewHandle hOutput;
    {
      plGALUnorderedAccessViewCreationDescription desc;
      desc.m_hTexture = pColorOutput->m_TextureHandle;
      desc.m_uiMipLevelToUse = 0;
      hOutput = pDevice->CreateUnorderedAccessView(desc);
    }

    renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
    renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", hInput);
    renderViewContext.m_pRenderContext->BindTexture2D("OffsetTexture", m_hOffsetTexture, plResourceAcquireMode::BlockTillLoaded);
    renderViewContext.m_pRenderContext->BindConstantBuffer("plChromaticAberrationConstants", m_hConstantBuffer);

    const plUInt32 uiWidth = pColorOutput->m_Desc.m_uiWidth;
    const plUInt32 uiHeight = pColorOutput->m_Desc.m_uiHeight;

    const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
    const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

    UpdateConstantBuffer();

    renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
  }
  pDevice->EndPass(pPass);

  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading);
}

void plChromaticAberrationPass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto pInput = inputs[m_PinInput.m_uiInputIndex];
  const auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];

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

void plChromaticAberrationPass::UpdateConstantBuffer() const
{
  auto* constants = plRenderContext::GetConstantBufferData<plChromaticAberrationConstants>(m_hConstantBuffer);
  constants->Strength = m_fStrength;
}

void plChromaticAberrationPass::SetOffsetTextureFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hOffsetTexture = plResourceManager::LoadResource<plTexture2DResource>(szFile);
  }
}

const char* plChromaticAberrationPass::GetOffsetTextureFile() const
{
  if (!m_hOffsetTexture.IsValid())
    return "";

  return m_hOffsetTexture.GetResourceID();
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ChromaticAberrationPass);
