#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class PLASMA_RENDERERCORE_DLL plBloomPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plBloomPass, plRenderPipelinePass);

public:
  plBloomPass();
  ~plBloomPass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

protected:
  void UpdateConstantBuffer(plVec2 pixelSize, const plColor& tintColor);

  plRenderPipelineNodeInputPin m_PinInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  float m_fRadius = 0.2f;
  float m_fThreshold = 1.0f;
  float m_fIntensity = 0.3f;
  plColorGammaUB m_InnerTintColor = plColor::White;
  plColorGammaUB m_MidTintColor = plColor::White;
  plColorGammaUB m_OuterTintColor = plColor::White;
  plConstantBufferStorageHandle m_hConstantBuffer;
  plShaderResourceHandle m_hShader;
};
