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


