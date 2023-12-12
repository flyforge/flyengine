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
  ~plBloomPass() override;

  bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  void ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  void UpdateBloomConstantBuffer(plVec2 pixelSize) const;
  void UpdateSPDConstantBuffer(plUInt32 uiWorkGroupCount);

  plRenderPipelineNodeInputPin m_PinInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  // SPD shaderDatas
  plShaderResourceHandle m_hDownscaleShader;
  plGALBufferHandle m_hDownsampleAtomicCounterBuffer;
  plArrayPtr<plUInt32> m_DownsampleAtomicCounter;
  plConstantBufferStorageHandle m_hSPDConstantBuffer;

  // Bloom shader data
  plShaderResourceHandle m_hBloomShader;
  plConstantBufferStorageHandle m_hBloomConstantBuffer;

  float m_fIntensity;
  float m_fBloomThreshold;
  plUInt32 m_uiMipCount;
  plGALTextureHandle m_hBloomTexture;
};
