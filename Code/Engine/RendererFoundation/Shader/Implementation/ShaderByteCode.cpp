#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Shader/ShaderByteCode.h>

plGALShaderByteCode::plGALShaderByteCode() {}

plGALShaderByteCode::plGALShaderByteCode(const plArrayPtr<const plUInt8>& byteCode)
{
  CopyFrom(byteCode);
}

void plGALShaderByteCode::CopyFrom(const plArrayPtr<const plUInt8>& pByteCode)
{
  PLASMA_ASSERT_DEV(pByteCode.GetPtr() != nullptr && pByteCode.GetCount() != 0, "Byte code is invalid!");

  m_Source = pByteCode;
}



PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_ShaderByteCode);
