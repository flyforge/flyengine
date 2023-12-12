#pragma once
#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

struct plUpscaleMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    /**
     * Uses AMD Fidelity FX Super Resolution as the upscale method.
     * Over better results than linear for almost no costs on performances.
     */
    FSR,

    /**
     * Uses simple bi-linear filtering as the upscale method.
     * The fastest way, but with quality drops.
     */
    BiLinear,

    Default = FSR
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plUpscaleMode);

struct plFSRUpscalePreset
{
  using StorageType = plUInt8;

  enum Enum
  {
    /**
     * Ultra Quality preset produces an image with quality virtually indistinguishable from native
     * rendering. It should be selected when the highest quality is desired.
     */
    UltraQuality,

    /**
     * Quality preset produces a super resolution image with quality representative of native rendering,
     * with a sizeable performance gain.
     */
    Quality,

    /**
     * Balanced preset produces a super resolution image approximating native rendering quality, with a major
     * performance gain compared to native.
     */
    Balanced,

    /**
     * Performance preset visibly impacts image quality and should only be selected in situations where needing
     * additional performance is critical.
     */
    Performance,

    Default = UltraQuality
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plFSRUpscalePreset);

class PLASMA_RENDERERCORE_DLL plUpscalePass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plUpscalePass, plRenderPipelinePass);

public:
  plUpscalePass();
  ~plUpscalePass() override;

  bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  void ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  plRenderPipelineNodeInputPin m_PinInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  plEnum<plUpscaleMode> m_eUpscaleMode;

  plEnum<plFSRUpscalePreset> m_eFSRPreset;
  bool m_bFSRSharpen;
  float m_fFSRSharpness;

  plShaderResourceHandle m_hShader;
  plConstantBufferStorageHandle m_hConstantBuffer;
};
