#pragma once

#include <RendererCore/Shader/ShaderResource.h>

/// \brief a deferred render pass that renders all opaque objects into multiple targets.
class PLASMA_RENDERERCORE_DLL plOpaqueDeferredRenderPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plOpaqueDeferredRenderPass, plRenderPipelinePass);

public:
  plOpaqueDeferredRenderPass(const char* szName = "OpaqueDeferredRenderPass");
  ~plOpaqueDeferredRenderPass() override;

  bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  void SetupPermutationVars(const plRenderViewContext& renderViewContext);

  void RenderObjects(const plRenderViewContext& renderViewContext);

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

  bool m_bWriteDepth;
};
