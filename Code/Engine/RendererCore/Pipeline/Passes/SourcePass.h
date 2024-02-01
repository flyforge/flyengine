#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

struct plSourceFormat
{
  using StorageType = plUInt8;

  enum Enum
  {
    Color4Channel8BitNormalized_sRGB,
    Color4Channel8BitNormalized,
    Color4Channel16BitFloat,
    Color4Channel32BitFloat,
    Color3Channel11_11_10BitFloat,
    Depth16Bit,
    Depth24BitStencil8Bit,
    Depth32BitFloat,

    Default = Color4Channel8BitNormalized_sRGB
  };
};
PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plSourceFormat);


class PL_RENDERERCORE_DLL plSourcePass : public plRenderPipelinePass
{
  PL_ADD_DYNAMIC_REFLECTION(plSourcePass, plRenderPipelinePass);

public:
  plSourcePass(const char* szName = "SourcePass");
  ~plSourcePass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

protected:
  plRenderPipelineNodeOutputPin m_PinOutput;

  plEnum<plSourceFormat> m_Format;
  plEnum<plGALMSAASampleCount> m_MsaaMode;
  plColor m_ClearColor;
  bool m_bClear;
};
