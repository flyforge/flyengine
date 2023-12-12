#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerHLSL/ShaderCompilerHLSLDLL.h>

struct ID3D11ShaderReflectionConstantBuffer;

class PLASMA_SHADERCOMPILERHLSL_DLL plShaderCompilerHLSL : public plShaderProgramCompiler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plShaderCompilerHLSL, plShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(plHybridArray<plString, 4>& Platforms) override
  {
    Platforms.PushBack("DX11_SM40_93");
    Platforms.PushBack("DX11_SM40");
    Platforms.PushBack("DX11_SM41");
    Platforms.PushBack("DX11_SM50");
  }

  virtual plResult Compile(plShaderProgramData& inout_Data, plLogInterface* pLog) override;

private:
  void ReflectShaderStage(plShaderProgramData& inout_Data, plGALShaderStage::Enum Stage);
  plShaderConstantBufferLayout* ReflectConstantBufferLayout(plShaderStageBinary& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection);
};
