#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

class PL_RENDERERCORE_DLL plTonemapPass : public plRenderPipelinePass
{
  PL_ADD_DYNAMIC_REFLECTION(plTonemapPass, plRenderPipelinePass);

public:
  plTonemapPass();
  ~plTonemapPass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

protected:
  plRenderPipelineNodeInputPin m_PinColorInput;
  plRenderPipelineNodeInputPin m_PinBloomInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  void SetVignettingTextureFile(const char* szFile);
  const char* GetVignettingTextureFile() const;

  void SetLUT1TextureFile(const char* szFile);
  const char* GetLUT1TextureFile() const;

  void SetLUT2TextureFile(const char* szFile);
  const char* GetLUT2TextureFile() const;

  plTexture2DResourceHandle m_hVignettingTexture;
  plTexture2DResourceHandle m_hNoiseTexture;
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
