#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

class PLASMA_RENDERERCORE_DLL plSourcePass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSourcePass, plRenderPipelinePass);

public:
  plSourcePass(const char* szName = "SourcePass");
  ~plSourcePass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  plRenderPipelineNodeOutputPin m_PinOutput;

  plGALResourceFormat::Enum m_Format;
  plGALMSAASampleCount::Enum m_MsaaMode;
  plColor m_ClearColor;
  bool m_bClear;
};
