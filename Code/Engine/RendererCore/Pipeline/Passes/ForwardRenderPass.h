#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

struct plForwardRenderShadingQuality
{
  using StorageType = plInt8;

  enum Enum
  {
    Normal,
    Simplified,

    Default = Normal,
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plForwardRenderShadingQuality);

/// \brief A standard forward render pass that renders into the color target.
class PL_RENDERERCORE_DLL plForwardRenderPass : public plRenderPipelinePass
{
  PL_ADD_DYNAMIC_REFLECTION(plForwardRenderPass, plRenderPipelinePass);

public:
  plForwardRenderPass(const char* szName = "ForwardRenderPass");
  ~plForwardRenderPass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

protected:
  virtual void SetupResources(plGALPass* pGALPass, const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs);
  virtual void SetupPermutationVars(const plRenderViewContext& renderViewContext);
  virtual void SetupLighting(const plRenderViewContext& renderViewContext);

  virtual void RenderObjects(const plRenderViewContext& renderViewContext) = 0;

  plRenderPipelineNodePassThrougPin m_PinColor;
  plRenderPipelineNodePassThrougPin m_PinVelocity;
  plRenderPipelineNodePassThrougPin m_PinMaterial;
  plRenderPipelineNodePassThrougPin m_PinDepthStencil;

  plEnum<plForwardRenderShadingQuality> m_ShadingQuality;
};
