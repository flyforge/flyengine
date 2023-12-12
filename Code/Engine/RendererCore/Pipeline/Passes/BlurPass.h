#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// \brief Blurs input and writes it to an output buffer of the same format.
class PLASMA_RENDERERCORE_DLL plBlurPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plBlurPass, plRenderPipelinePass);

public:
  plBlurPass();
  ~plBlurPass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

  void SetRadius(plInt32 iRadius);
  plInt32 GetRadius() const;

protected:
  plRenderPipelineNodeInputPin m_PinInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  plInt32 m_iRadius;
  plConstantBufferStorageHandle m_hBlurCB;
  plShaderResourceHandle m_hShader;
};
