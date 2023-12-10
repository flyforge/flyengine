#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

struct plGALRenderTargets;

class PLASMA_RENDERERCORE_DLL plTargetPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTargetPass, plRenderPipelinePass);

public:
  plTargetPass(const char* szName = "TargetPass");
  ~plTargetPass();

  const plGALTextureHandle* GetTextureHandle(const plGALRenderTargets& renderTargets, const plRenderPipelineNodePin* pPin);

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

private:
  bool VerifyInput(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, const char* szPinName);

protected:
  plRenderPipelineNodeInputPin m_PinColor0;
  plRenderPipelineNodeInputPin m_PinColor1;
  plRenderPipelineNodeInputPin m_PinColor2;
  plRenderPipelineNodeInputPin m_PinColor3;
  plRenderPipelineNodeInputPin m_PinColor4;
  plRenderPipelineNodeInputPin m_PinColor5;
  plRenderPipelineNodeInputPin m_PinColor6;
  plRenderPipelineNodeInputPin m_PinColor7;
  plRenderPipelineNodeInputPin m_PinDepthStencil;
};
