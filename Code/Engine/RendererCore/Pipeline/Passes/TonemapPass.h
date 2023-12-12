#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class PLASMA_RENDERERCORE_DLL plTonemapPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTonemapPass, plRenderPipelinePass);

public:
  plTonemapPass();
  ~plTonemapPass() override;

  bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  void UpdateConstantBuffer();

  plRenderPipelineNodeInputPin m_PinInput;
  plRenderPipelineNodeInputPin m_PinBloom;
  plRenderPipelineNodeOutputPin m_PinOutput;

  plEnum<plTonemapMode> m_eTonemapMode;

  float m_HDRMax;
  float m_Contrast;
  float m_Shoulder;
  float m_MidIn;
  float m_MidOut;

  plConstantBufferStorageHandle m_hConstantBuffer;
  plShaderResourceHandle m_hShader;
};
