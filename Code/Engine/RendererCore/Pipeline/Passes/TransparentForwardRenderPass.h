#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// \brief A forward render pass that renders all transparent objects into the color target.
class PLASMA_RENDERERCORE_DLL plTransparentForwardRenderPass : public plForwardRenderPass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTransparentForwardRenderPass, plForwardRenderPass);

public:
  plTransparentForwardRenderPass(const char* szName = "TransparentForwardRenderPass");
  ~plTransparentForwardRenderPass();

  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  virtual void SetupResources(plGALPass* pGALPass, const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual void RenderObjects(const plRenderViewContext& renderViewContext) override;

  void UpdateSceneColorTexture(const plRenderViewContext& renderViewContext, plGALTextureHandle hSceneColorTexture, plGALTextureHandle hCurrentColorTexture);
  void CreateSamplerState();

  plRenderPipelineNodeInputPin m_PinResolvedDepth;

  plGALSamplerStateHandle m_hSceneColorSamplerState;
};
