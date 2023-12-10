#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/CopyTexturePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCopyTexturePass, 1, plRTTIDefaultAllocator<plCopyTexturePass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Input", m_PinInput),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCopyTexturePass::plCopyTexturePass()
  : plRenderPipelinePass("CopyTexturePass")
{
}

plCopyTexturePass::~plCopyTexturePass() = default;

bool plCopyTexturePass::GetRenderTargetDescriptions(
  const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  const plGALTextureCreationDescription* pInput = inputs[m_PinInput.m_uiInputIndex];

  if (pInput != nullptr)
  {
    plGALTextureCreationDescription desc = *pInput;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    plLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void plCopyTexturePass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  const plGALTexture* pDest = pDevice->GetTexture(pOutput->m_TextureHandle);
  const plGALTexture* pSource = pDevice->GetTexture(pInput->m_TextureHandle);

  if (pDest->GetDescription().m_Format != pSource->GetDescription().m_Format)
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



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_CopyTexturePass);
