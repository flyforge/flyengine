#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// \brief Depth aware blur on input and writes it to an output buffer of the same format.
///
/// In theory it is mathematical nonsense to separate a bilateral blur, but it is common praxis and works good enough.
/// (Thus the name "separated" in contrast to "separable")
class PLASMA_RENDERERCORE_DLL plSeparatedBilateralBlurPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSeparatedBilateralBlurPass, plRenderPipelinePass);

public:
  plSeparatedBilateralBlurPass();
  ~plSeparatedBilateralBlurPass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

  void SetRadius(plUInt32 uiRadius);
  plUInt32 GetRadius() const;

  void SetGaussianSigma(float fSigma);
  float GetGaussianSigma() const;

  void SetSharpness(float fSharpness);
  float GetSharpness() const;

protected:
  plRenderPipelineNodeInputPin m_PinBlurSourceInput;
  plRenderPipelineNodeInputPin m_PinDepthInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  plUInt32 m_uiRadius;
  float m_fGaussianSigma;
  float m_fSharpness;
  plConstantBufferStorageHandle m_hBilateralBlurCB;
  plShaderResourceHandle m_hShader;
};
