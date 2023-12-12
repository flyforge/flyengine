#pragma once

#include <RendererCore/Shader/ShaderResource.h>

/// \brief a deferred render pass that renders all opaque objects into multiple targets.
class PLASMA_RENDERERCORE_DLL plOpaqueDeferredLightingPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plOpaqueDeferredLightingPass, plRenderPipelinePass);

public:
  plOpaqueDeferredLightingPass(const char* szName = "OpaqueDeferredLightingPass");
  ~plOpaqueDeferredLightingPass() override;

  bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:

  plRenderPipelineNodeInputPin m_PinSSAO;

  plRenderPipelineNodePassThrougPin m_PinAlbedo;
  plRenderPipelineNodePassThrougPin m_PinNormal;
  plRenderPipelineNodePassThrougPin m_PinMaterial;
  plRenderPipelineNodePassThrougPin m_PinMaterialAdvanced;
  plRenderPipelineNodePassThrougPin m_PinVelocity;
  plRenderPipelineNodePassThrougPin m_PinClearCoat;
  plRenderPipelineNodePassThrougPin m_PinDepthStencil;

  plRenderPipelineNodeOutputPin m_PinOutput;

  plShaderResourceHandle m_hShader;
};
