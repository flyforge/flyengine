#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>

const plUInt8 plGALIndexType::s_Size[plGALIndexType::ENUM_COUNT] = {
  0,               // None
  sizeof(plInt16), // UShort
  sizeof(plInt32)  // UInt
};

const char* plGALShaderStage::Names[ENUM_COUNT] = {
  "VertexShader",
  "HullShader",
  "DomainShader",
  "GeometryShader",
  "PixelShader",
  "ComputeShader",
};

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plGALMSAASampleCount, 1)
  PLASMA_ENUM_CONSTANTS(
    plGALMSAASampleCount::None, plGALMSAASampleCount::TwoSamples, plGALMSAASampleCount::FourSamples, plGALMSAASampleCount::EightSamples)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Basics);
