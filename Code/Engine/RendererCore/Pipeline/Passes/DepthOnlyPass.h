#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// \brief A render pass that renders into a depth target only.
class PLASMA_RENDERERCORE_DLL plDepthOnlyPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDepthOnlyPass, plRenderPipelinePass);

public:
  plDepthOnlyPass(const char* szName = "DepthOnlyPass");
  ~plDepthOnlyPass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  plRenderPipelineNodePassThrougPin m_PinDepthStencil;
};
