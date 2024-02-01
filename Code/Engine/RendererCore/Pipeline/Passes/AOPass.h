#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class PL_RENDERERCORE_DLL plAOPass : public plRenderPipelinePass
{
  PL_ADD_DYNAMIC_REFLECTION(plAOPass, plRenderPipelinePass);

public:
  plAOPass();
  ~plAOPass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

  void SetFadeOutStart(float fStart);
  float GetFadeOutStart() const;

  void SetFadeOutEnd(float fEnd);
  float GetFadeOutEnd() const;

protected:
  void CreateSamplerState();

  plRenderPipelineNodeInputPin m_PinDepthInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  float m_fRadius = 1.0f;
  float m_fMaxScreenSpaceRadius = 1.0f;
  float m_fContrast = 2.0f;
  float m_fIntensity = 0.7f;

  float m_fFadeOutStart = 80.0f;
  float m_fFadeOutEnd = 100.0f;

  float m_fPositionBias = 5.0f;
  float m_fMipLevelScale = 10.0f;
  float m_fDepthBlurThreshold = 2.0f;

  plConstantBufferStorageHandle m_hDownscaleConstantBuffer;
  plConstantBufferStorageHandle m_hSSAOConstantBuffer;

  plTexture2DResourceHandle m_hNoiseTexture;

  plGALSamplerStateHandle m_hSSAOSamplerState;

  plShaderResourceHandle m_hDownscaleShader;
  plShaderResourceHandle m_hSSAOShader;
  plShaderResourceHandle m_hBlurShader;
};
