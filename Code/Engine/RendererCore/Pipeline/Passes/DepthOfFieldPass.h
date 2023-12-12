#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class PLASMA_RENDERERCORE_DLL plDepthOfFieldPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDepthOfFieldPass, plRenderPipelinePass);

public:
  plDepthOfFieldPass();
  ~plDepthOfFieldPass() override;

  bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  void ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  void UpdateConstantBuffer();

  plRenderPipelineNodeInputPin m_PinInput;
  plRenderPipelineNodeInputPin m_PinDepth;
  plRenderPipelineNodeOutputPin m_PinOutput;

  plShaderResourceHandle m_hShader;
  plConstantBufferStorageHandle m_hConstantBuffer;

  plGALTextureHandle m_hBokehTexture1;
  plGALTextureHandle m_hBokehTexture2;

  float m_fRadius;
};
