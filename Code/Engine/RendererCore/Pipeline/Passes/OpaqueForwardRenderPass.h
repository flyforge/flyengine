#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// \brief A forward render pass that renders all opaque objects into the color target.
class PLASMA_RENDERERCORE_DLL plOpaqueForwardRenderPass : public plForwardRenderPass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plOpaqueForwardRenderPass, plForwardRenderPass);

public:
  plOpaqueForwardRenderPass(const char* szName = "OpaqueForwardRenderPass");
  ~plOpaqueForwardRenderPass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

protected:
  virtual void SetupResources(plGALPass* pGALPass, const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual void SetupPermutationVars(const plRenderViewContext& renderViewContext) override;

  virtual void RenderObjects(const plRenderViewContext& renderViewContext) override;

  plRenderPipelineNodeInputPin m_PinSSAO;
  // plRenderPipelineNodeOutputPin m_PinNormal;
  // plRenderPipelineNodeOutputPin m_PinSpecularColorRoughness;

  bool m_bWriteDepth = true;

  plTexture2DResourceHandle m_hWhiteTexture;
};
