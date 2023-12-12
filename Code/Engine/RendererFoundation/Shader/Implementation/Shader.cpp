#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

plGALShader::plGALShader(const plGALShaderCreationDescription& Description)
  : plGALObject(Description)
{
}

plGALShader::~plGALShader() {}

plDelegate<void(plShaderUtils::plBuiltinShaderType type, plShaderUtils::plBuiltinShader& out_shader)> plShaderUtils::g_RequestBuiltinShaderCallback;

PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_Shader);
