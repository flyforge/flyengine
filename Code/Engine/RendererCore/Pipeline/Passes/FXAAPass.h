#pragma once

#include <Foundation/Reflection/Reflection.h>

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

struct PLASMA_RENDERERCORE_DLL plEdgeThresholdQuality
{
  using StorageType = plUInt8;

  enum Enum : StorageType
  {
    Little,
    LowQuality,
    DefaultQuality,
    HighQuality,
    Overkill,

    Default = DefaultQuality,
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plEdgeThresholdQuality);

struct PLASMA_RENDERERCORE_DLL plEdgeThresholdMinQuality
{
  using StorageType = plUInt8;

  enum Enum : StorageType
  {
    UpperLimit,
    HighQuality,
    VisibleLimit,

    Default = UpperLimit,
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plEdgeThresholdMinQuality);

class PLASMA_RENDERERCORE_DLL plFXAAPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plFXAAPass, plRenderPipelinePass);

public:
  plFXAAPass();
  ~plFXAAPass() override;

  bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  void ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  void UpdateConstantBuffer() const;

  plRenderPipelineNodeInputPin m_PinInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  float m_fSPARAmount;
  plEnum<plEdgeThresholdQuality> m_eEdgeThreshold;
  plEnum<plEdgeThresholdMinQuality> m_eEdgeThresholdMin;

  plShaderResourceHandle m_hShader;
  plConstantBufferStorageHandle m_hConstantBuffer;
};
