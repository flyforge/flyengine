#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// \brief A forward render pass that renders all sky objects into the color target.
class PL_RENDERERCORE_DLL plSkyRenderPass : public plForwardRenderPass
{
  PL_ADD_DYNAMIC_REFLECTION(plSkyRenderPass, plForwardRenderPass);

public:
  plSkyRenderPass(const char* szName = "SkyRenderPass");
  ~plSkyRenderPass();

protected:
  virtual void RenderObjects(const plRenderViewContext& renderViewContext) override;
};
