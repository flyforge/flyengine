#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/FramePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererFoundation/Resources/Texture.h>

#include "../../../../../../Data/Base/Shaders/Pipeline/FrameConstants.h"

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plFramePass, 1, plRTTIDefaultAllocator<plFramePass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Input", m_PinInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_MEMBER_PROPERTY("Ratio", m_Ratio)->AddAttributes(new plDefaultValueAttribute(plVec2(16, 9))),
    PLASMA_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new plDefaultValueAttribute(plColor::Black))
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plFramePass::plFramePass()
  : plRenderPipelinePass("FramePass", true)
  , m_Ratio(plVec2(16, 9))
{
  {
    // Load shader.
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/Frame.plShader");
    PLASMA_ASSERT_DEV(m_hShader.IsValid(), "Failed to load shader 'Frame.plShader'.");
  }

  {
    m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plFrameConstants>();
  }
}

plFramePass::~plFramePass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool plFramePass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  const plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  const plGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

  // Color
  if (const auto* pColorInput = inputs[m_PinInput.m_uiInputIndex]; pColorInput != nullptr)
  {
    if (!pColorInput->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' input must allow shader resource view.", GetName());
      return false;
    }

    if (const plGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]))
    {
      const plGALTextureCreationDescription& desc = pTexture->GetDescription();

      outputs[m_PinOutput.m_uiOutputIndex].SetAsRenderTarget(pColorInput->m_uiWidth, pColorInput->m_uiHeight, desc.m_Format);
      outputs[m_PinOutput.m_uiOutputIndex].m_uiArraySize = pColorInput->m_uiArraySize;
    }

    // outputs[m_PinBloomOutput.m_uiOutputIndex] = *inputs[m_PinInput.m_uiInputIndex];
  }
  else
  {
    plLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void plFramePass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInput = inputs[m_PinInput.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  const plUInt32 uiWidth = pOutput->m_Desc.m_uiWidth;
  const plUInt32 uiHeight = pOutput->m_Desc.m_uiHeight;

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plGALPass* pPass = pDevice->BeginPass(GetName());
  {
    const plGALTextureHandle& hInput = pInput->m_TextureHandle;
    const plGALTextureHandle& hOutput = pOutput->m_TextureHandle;

    // Setup render target
    plGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(hOutput));

    // Bind render target and viewport
    renderViewContext.m_pRenderContext->BeginRendering(pPass, renderingSetup, plRectFloat(uiWidth, uiHeight), "", renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindConstantBuffer("plFrameConstants", m_hConstantBuffer);

    renderViewContext.m_pRenderContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, 1);

    UpdateConstantBuffer();

    renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(hInput));
    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    renderViewContext.m_pRenderContext->EndRendering();
  }
  pDevice->EndPass(pPass);
}

void plFramePass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
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

void plFramePass::UpdateConstantBuffer() const
{
  auto* constants = plRenderContext::GetConstantBufferData<plFrameConstants>(m_hConstantBuffer);
  constants->Ratio = m_Ratio.x / m_Ratio.y;
  constants->Color = m_Color.GetAsVec4().GetAsVec3();
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_FramePass);
