#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerDXC/ShaderCompilerDXCDLL.h>

struct SpvReflectDescriptorBinding;
struct SpvReflectBlockVariable;

class PL_SHADERCOMPILERDXC_DLL plShaderCompilerDXC : public plShaderProgramCompiler
{
  PL_ADD_DYNAMIC_REFLECTION(plShaderCompilerDXC, plShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(plHybridArray<plString, 4>& out_platforms) override { out_platforms.PushBack("VULKAN"); }

  virtual plResult ModifyShaderSource(plShaderProgramData& inout_data, plLogInterface* pLog) override;
  virtual plResult Compile(plShaderProgramData& inout_Data, plLogInterface* pLog) override;

private:
  /// \brief Sets fixed set / slot bindings to each resource.
  /// The end result will have these properties:
  /// 1. Every binding name has a unique set / slot.
  /// 2. Bindings that already had a fixed set or slot (e.g. != -1) should not have these changed.
  /// 2. Set / slots can only be the same for two bindings if they have been changed to plGALShaderResourceType::TextureAndSampler.
  plResult DefineShaderResourceBindings(const plShaderProgramData& data, plHashTable<plHashedString, plShaderResourceBinding>& inout_resourceBinding, plLogInterface* pLog);

  void CreateNewShaderResourceDeclaration(plStringView sPlatform, plStringView sDeclaration, const plShaderResourceBinding& binding, plStringBuilder& out_sDeclaration);

  plResult ReflectShaderStage(plShaderProgramData& inout_Data, plGALShaderStage::Enum Stage);
  plShaderConstantBufferLayout* ReflectConstantBufferLayout(plGALShaderByteCode& pStageBinary, const char* szName, const SpvReflectBlockVariable& block);
  plResult FillResourceBinding(plGALShaderByteCode& shaderBinary, plShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  plResult FillSRVResourceBinding(plGALShaderByteCode& shaderBinary, plShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  plResult FillUAVResourceBinding(plGALShaderByteCode& shaderBinary, plShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  static plGALShaderTextureType::Enum GetTextureType(const SpvReflectDescriptorBinding& info);
  plResult Initialize();

private:
  plMap<const char*, plGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
