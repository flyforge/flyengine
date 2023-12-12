#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerDXC/ShaderCompilerDXCDLL.h>

struct SpvReflectDescriptorBinding;

class PLASMA_SHADERCOMPILERDXC_DLL plShaderCompilerDXC : public plShaderProgramCompiler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plShaderCompilerDXC, plShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(plHybridArray<plString, 4>& Platforms) override { Platforms.PushBack("VULKAN"); }

  virtual plResult Compile(plShaderProgramData& inout_Data, plLogInterface* pLog) override;

private:
  plResult ReflectShaderStage(plShaderProgramData& inout_Data, plGALShaderStage::Enum Stage);
  plShaderConstantBufferLayout* ReflectConstantBufferLayout(plShaderStageBinary& pStageBinary, const SpvReflectDescriptorBinding& pConstantBufferReflection);
  plResult FillResourceBinding(plShaderStageBinary& shaderBinary, plShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  plResult FillSRVResourceBinding(plShaderStageBinary& shaderBinary, plShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  plResult FillUAVResourceBinding(plShaderStageBinary& shaderBinary, plShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);

  plResult Initialize();

private:
  plMap<const char*, plGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
