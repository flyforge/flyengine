#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class PLASMA_RENDERERCORE_DLL plReflectionFilterPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plReflectionFilterPass, plRenderPipelinePass);

public:
  plReflectionFilterPass();
  ~plReflectionFilterPass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

  plUInt32 GetInputCubemap() const;
  void SetInputCubemap(plUInt32 uiCubemapHandle);

protected:
  void UpdateFilteredSpecularConstantBuffer(plUInt32 uiMipMapIndex, plUInt32 uiNumMipMaps);
  void UpdateIrradianceConstantBuffer();

  plRenderPipelineNodeOutputPin m_PinFilteredSpecular;
  plRenderPipelineNodeOutputPin m_PinAvgLuminance;
  plRenderPipelineNodeOutputPin m_PinIrradianceData;

  float m_fIntensity;
  float m_fSaturation;
  plUInt32 m_uiSpecularOutputIndex;
  plUInt32 m_uiIrradianceOutputIndex;

  plGALTextureHandle m_hInputCubemap;

  plConstantBufferStorageHandle m_hFilteredSpecularConstantBuffer;
  plShaderResourceHandle m_hFilteredSpecularShader;

  plConstantBufferStorageHandle m_hIrradianceConstantBuffer;
  plShaderResourceHandle m_hIrradianceShader;
};
