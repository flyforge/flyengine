#pragma once

#include <Foundation/Reflection/Reflection.h>

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class PLASMA_RENDERERCORE_DLL plTAAPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTAAPass, plRenderPipelinePass);

public:
  plTAAPass();
  ~plTAAPass() override;

  bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  void ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  void UpdateTAAConstantBuffer() const;
  void UpdateCopyConstantBuffer(plVec2I32 offset, plVec2U32 size) const;

  plRenderPipelineNodeInputPin m_PinInputColor;
  plRenderPipelineNodeInputPin m_PinInputVelocity;
  plRenderPipelineNodeInputPin m_PinInputHistory;
  plRenderPipelineNodeInputPin m_PinInputDepth;
  plRenderPipelineNodeOutputPin m_PinOutput;

  bool m_bUpsample;

  plShaderResourceHandle m_hTAAShader;
  plConstantBufferStorageHandle m_hTAAConstantBuffer;

  plShaderResourceHandle m_hCopyShader;
  plConstantBufferStorageHandle m_hCopyConstantBuffer;

  plGALTextureHandle m_hPreviousVelocity;
  plGALTextureHandle m_hHistory;
};