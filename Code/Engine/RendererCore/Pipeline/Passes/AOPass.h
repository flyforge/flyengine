#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class PLASMA_RENDERERCORE_DLL plAOPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAOPass, plRenderPipelinePass);

public:
  plAOPass();
  ~plAOPass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

  void SetFadeOutStart(float fStart);
  float GetFadeOutStart() const;

  void SetFadeOutEnd(float fEnd);
  float GetFadeOutEnd() const;

protected:
  void CreateSamplerState();

  plRenderPipelineNodeInputPin m_PinDepthInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  float m_fRadius;
  float m_fMaxScreenSpaceRadius;
  float m_fContrast;
  float m_fIntensity;

  float m_fFadeOutStart;
  float m_fFadeOutEnd;

  float m_fPositionBias;
  float m_fMipLevelScale;
  float m_fDepthBlurThreshold;

  plConstantBufferStorageHandle m_hDownscaleConstantBuffer;
  plConstantBufferStorageHandle m_hSSAOConstantBuffer;

  plTexture2DResourceHandle m_hNoiseTexture;

  plGALSamplerStateHandle m_hSSAOSamplerState;

  plShaderResourceHandle m_hDownscaleShader;
  plShaderResourceHandle m_hSSAOShader;
  plShaderResourceHandle m_hBlurShader;
};
