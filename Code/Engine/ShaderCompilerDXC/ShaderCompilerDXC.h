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
  virtual plResult DefineShaderResourceBindings(plShaderProgramData& inout_data, plHashTable<plHashedString, plShaderResourceBinding>& ref_resourceBinding, plLogInterface* pLog) override;
  virtual void CreateShaderResourceDeclaration(plStringView sDeclaration, const plShaderResourceBinding& binding, plStringBuilder& out_sDeclaration) override;

private:
  plResult ReflectShaderStage(plShaderProgramData& inout_Data, plGALShaderStage::Enum Stage);
  plShaderConstantBufferLayout* ReflectConstantBufferLayout(plGALShaderByteCode& pStageBinary, const SpvReflectDescriptorBinding& pConstantBufferReflection);
  plResult FillResourceBinding(plGALShaderByteCode& shaderBinary, plShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  plResult FillSRVResourceBinding(plGALShaderByteCode& shaderBinary, plShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  plResult FillUAVResourceBinding(plGALShaderByteCode& shaderBinary, plShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  static plGALShaderTextureType::Enum GetTextureType(const SpvReflectDescriptorBinding& info);
  plResult Initialize();

private:
  plMap<const char*, plGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
