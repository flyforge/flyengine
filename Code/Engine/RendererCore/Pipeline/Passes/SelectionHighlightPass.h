#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class PLASMA_RENDERERCORE_DLL plSelectionHighlightPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSelectionHighlightPass, plRenderPipelinePass);

public:
  plSelectionHighlightPass(const char* szName = "SelectionHighlightPass");
  ~plSelectionHighlightPass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

protected:
  plRenderPipelineNodePassThrougPin m_PinColor;
  plRenderPipelineNodeInputPin m_PinDepthStencil;

  plShaderResourceHandle m_hShader;
  plConstantBufferStorageHandle m_hConstantBuffer;

  plColor m_HighlightColor = plColorScheme::LightUI(plColorScheme::Yellow);
  float m_fOverlayOpacity = 0.1f;
};
