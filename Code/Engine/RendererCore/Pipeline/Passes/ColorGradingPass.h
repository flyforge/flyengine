#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

class PLASMA_RENDERERCORE_DLL plColorGradingPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plColorGradingPass, plRenderPipelinePass);

public:
  plColorGradingPass();
  ~plColorGradingPass() override;

  bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  plRenderPipelineNodeInputPin m_PinInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  //  void SetVignettingTextureFile(const char* szFile);
  //  const char* GetVignettingTextureFile() const;

  void SetLUT1TextureFile(const char* szFile);
  const char* GetLUT1TextureFile() const;

  void SetLUT2TextureFile(const char* szFile);
  const char* GetLUT2TextureFile() const;

  plTexture3DResourceHandle m_hLUT1;
  plTexture3DResourceHandle m_hLUT2;

  plColor m_MoodColor;
  float m_fMoodStrength;
  float m_fSaturation;
  float m_fContrast;
  float m_fLut1Strength;
  float m_fLut2Strength;

  plConstantBufferStorageHandle m_hConstantBuffer;
  plShaderResourceHandle m_hShader;
};
