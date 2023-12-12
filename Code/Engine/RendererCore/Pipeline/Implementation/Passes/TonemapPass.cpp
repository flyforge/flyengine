#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/TonemapPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include "../../../../../../Data/Base/Shaders/Pipeline/CopyConstants.h"
#include "../../../../../../Data/Base/Shaders/Pipeline/TonemapConstants.h"

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plTonemapMode, 1)
  PLASMA_ENUM_CONSTANT(plTonemapMode::AMD),
  PLASMA_ENUM_CONSTANT(plTonemapMode::ACES),
  PLASMA_ENUM_CONSTANT(plTonemapMode::Uncharted2),
  PLASMA_ENUM_CONSTANT(plTonemapMode::Reinhard),
  PLASMA_ENUM_CONSTANT(plTonemapMode::None),
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTonemapPass, 2, plRTTIDefaultAllocator<plTonemapPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Input", m_PinInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_ENUM_MEMBER_PROPERTY("Mode", plTonemapMode, m_eTonemapMode)->AddAttributes(new plDefaultValueAttribute(plTonemapMode::AMD)),
    PLASMA_MEMBER_PROPERTY("HDRMax", m_HDRMax)->AddAttributes(new plDefaultValueAttribute(16.0f)),
    PLASMA_MEMBER_PROPERTY("Contrast", m_Contrast)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("Shoulder", m_Shoulder)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("MidIn", m_MidIn)->AddAttributes(new plDefaultValueAttribute(0.18f)),
    PLASMA_MEMBER_PROPERTY("MidOut", m_MidOut)->AddAttributes(new plDefaultValueAttribute(0.01125f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTonemapPass::plTonemapPass()
  : plRenderPipelinePass("TonemapPass", true)
  , m_eTonemapMode(plTonemapMode::AMD)
  , m_HDRMax(16.0f)
  , m_Contrast(1.0f)
  , m_Shoulder(1.0f)
  , m_MidIn(0.18f)
  , m_MidOut(0.01125f)
{
  // Loading shaders
  {
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/Tonemap.plShader");
    PLASMA_ASSERT_DEV(m_hShader.IsValid(), "Could not load Tonemap shader.");
  }

  // Loading resources
  {
    m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plTonemapConstants>();
  }
}

plTonemapPass::~plTonemapPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool plTonemapPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
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

  plGALTextureCreationDescription inputDesc = *inputs[m_PinInput.m_uiInputIndex];
  inputDesc.m_bAllowUAV = true;
  inputDesc.m_bCreateRenderTarget = true;
  outputs[m_PinOutput.m_uiOutputIndex] = std::move(inputDesc);

  return true;
}

void plTonemapPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* pInput = inputs[m_PinInput.m_uiInputIndex];
  const auto* pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plResourceManager::ForceLoadResourceNow(m_hShader);

  const bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  plGALPass* pTonemapPass = pDevice->BeginPass(GetName());
  {
    {
      auto pCommandEncoder = plRenderContext::BeginComputeScope(pTonemapPass, renderViewContext);
  
      renderViewContext.m_pRenderContext->BindShader(m_hShader);

      renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));

      plGALUnorderedAccessViewHandle hOutput;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = pOutput->m_TextureHandle;
        desc.m_uiMipLevelToUse = 0;
        hOutput = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);

      renderViewContext.m_pRenderContext->BindConstantBuffer("plTonemapConstants", m_hConstantBuffer);

      const plUInt32 uiWidth = pOutput->m_Desc.m_uiWidth;
      const plUInt32 uiHeight = pOutput->m_Desc.m_uiHeight;

      const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;
      const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;

      UpdateConstantBuffer();

      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 6).IgnoreResult();
    }
  }
  pDevice->EndPass(pTonemapPass);

  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading);
}

void plTonemapPass::UpdateConstantBuffer()
{
  auto* constants = plRenderContext::GetConstantBufferData<plTonemapConstants>(m_hConstantBuffer);
  constants->ToneMappingMode = m_eTonemapMode.GetValue();
  constants->HDRMax = m_HDRMax;
  constants->Contrast = m_Contrast;
  constants->Shoulder = m_Shoulder;
  constants->MidIn = m_MidIn;
  constants->MidOut = m_MidOut;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plTonemapPassPatch_1_2 : public plGraphPatch
{
public:
  plTonemapPassPatch_1_2() : plGraphPatch("plTonemapPass", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("Mode", plTonemapMode::AMD);
    pNode->RenameProperty("Color", "Input");
    pNode->RemoveProperty("Bloom");
  }
};

plTonemapPassPatch_1_2 g_plTonemapPassPatch_1_2;

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TonemapPass);
