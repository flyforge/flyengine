#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

class PLASMA_RENDERERCORE_DLL plMsaaUpscalePass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMsaaUpscalePass, plRenderPipelinePass);

public:
  plMsaaUpscalePass();
  ~plMsaaUpscalePass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

protected:
  plRenderPipelineNodeInputPin m_PinInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  plEnum<plGALMSAASampleCount> m_MsaaMode = plGALMSAASampleCount::None;
  plShaderResourceHandle m_hShader;
};
