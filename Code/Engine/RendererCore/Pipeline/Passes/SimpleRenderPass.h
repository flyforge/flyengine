#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// \brief A very basic render pass that renders into the color target.
///
/// Can either works as passthrough or if no input is present creates
/// output targets matching the view's render target.
/// Needs to be connected to a plTargetPass to function.
class PLASMA_RENDERERCORE_DLL plSimpleRenderPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSimpleRenderPass, plRenderPipelinePass);

public:
  plSimpleRenderPass(const char* szName = "SimpleRenderPass");
  ~plSimpleRenderPass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

  void SetMessage(const char* szMessage);

protected:
  plRenderPipelineNodePassThrougPin m_PinColor;
  plRenderPipelineNodePassThrougPin m_PinDepthStencil;

  plString m_sMessage;
};
