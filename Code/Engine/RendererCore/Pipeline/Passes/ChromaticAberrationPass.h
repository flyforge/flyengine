#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class PLASMA_RENDERERCORE_DLL plChromaticAberrationPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plChromaticAberrationPass, plRenderPipelinePass);

public:
  plChromaticAberrationPass();
  ~plChromaticAberrationPass() override;

  bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  void ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  void UpdateConstantBuffer() const;

  void SetOffsetTextureFile(const char* szFile);
  PLASMA_NODISCARD const char* GetOffsetTextureFile() const;

  plRenderPipelineNodeInputPin m_PinInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  plTexture2DResourceHandle m_hOffsetTexture;
  plConstantBufferStorageHandle m_hConstantBuffer;
  plShaderResourceHandle m_hShader;

  float m_fStrength;
};
