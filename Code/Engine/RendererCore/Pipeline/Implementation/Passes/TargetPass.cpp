#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetView.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTargetPass, 1, plRTTIDefaultAllocator<plTargetPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Color0", m_PinColor0),
    PLASMA_MEMBER_PROPERTY("Color1", m_PinColor1),
    PLASMA_MEMBER_PROPERTY("Color2", m_PinColor2),
    PLASMA_MEMBER_PROPERTY("Color3", m_PinColor3),
    PLASMA_MEMBER_PROPERTY("Color4", m_PinColor4),
    PLASMA_MEMBER_PROPERTY("Color5", m_PinColor5),
    PLASMA_MEMBER_PROPERTY("Color6", m_PinColor6),
    PLASMA_MEMBER_PROPERTY("Color7", m_PinColor7),
    PLASMA_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTargetPass::plTargetPass(const char* szName)
  : plRenderPipelinePass(szName, true)
{
}

plTargetPass::~plTargetPass() = default;

const plGALTextureHandle* plTargetPass::GetTextureHandle(const plGALRenderTargets& renderTargets, const plRenderPipelineNodePin* pPin)
{
  // auto inputs = GetInputPins();
  if (pPin->m_pParent != this)
  {
    plLog::Error("plTargetPass::GetTextureHandle: The given pin is not part of this pass!");
    return nullptr;
  }

  plGALTextureHandle hTarget;
  if (pPin->m_uiInputIndex == 8)
  {
    return &renderTargets.m_hDSTarget;
  }
  else
  {
    return &renderTargets.m_hRTs[pPin->m_uiInputIndex];
  }

  return nullptr;
}

bool plTargetPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  const char* pinNames[] = {
    "Color0",
    "Color1",
    "Color2",
    "Color3",
    "Color4",
    "Color5",
    "Color6",
    "Color7",
    "DepthStencil",
  };

  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(pinNames); ++i)
  {
    if (!VerifyInput(view, inputs, pinNames[i]))
      return false;
  }

  return true;
}

void plTargetPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) {}

bool plTargetPass::VerifyInput(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, const char* szPinName)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  const plRenderPipelineNodePin* pPin = GetPinByName(szPinName);
  if (inputs[pPin->m_uiInputIndex])
  {
    const plGALTextureHandle* pHandle = GetTextureHandle(view.GetActiveRenderTargets(), pPin);
    if (pHandle)
    {
      const plGALTexture* pTexture = pDevice->GetTexture(*pHandle);
      if (pTexture)
      {
        // TODO: Need a more sophisticated check here what is considered 'matching'
        // if (inputs[pPin->m_uiInputIndex]->CalculateHash() != pTexture->GetDescription().CalculateHash())
        //  return false;
      }
    }
  }

  return true;
}



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TargetPass);
