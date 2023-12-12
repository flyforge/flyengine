#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class PLASMA_RENDERERCORE_DLL plMotionBlurPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMotionBlurPass, plRenderPipelinePass);

public:
  plMotionBlurPass();
  ~plMotionBlurPass() override;

  bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  void ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  plRenderPipelineNodeInputPin m_PinInputColor;
  plRenderPipelineNodeInputPin m_PinInputVelocity;
  plRenderPipelineNodeInputPin m_PinInputDepth;
  plRenderPipelineNodeOutputPin m_PinOutput;

  plShaderResourceHandle m_hShader;
  plConstantBufferStorageHandle m_hConstantBuffer;

  plUInt32 m_fSamples;
  float m_fStrength;
  plEnum<plMotionBlurMode> m_eMode;
};
