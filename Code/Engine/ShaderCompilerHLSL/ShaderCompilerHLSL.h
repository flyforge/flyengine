#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerHLSL/ShaderCompilerHLSLDLL.h>

struct ID3D11ShaderReflectionConstantBuffer;
struct _D3D11_SIGNATURE_PARAMETER_DESC;

class PL_SHADERCOMPILERHLSL_DLL plShaderCompilerHLSL : public plShaderProgramCompiler
{
  PL_ADD_DYNAMIC_REFLECTION(plShaderCompilerHLSL, plShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(plHybridArray<plString, 4>& out_platforms) override
  {
    out_platforms.PushBack("DX11_SM40_93");
    out_platforms.PushBack("DX11_SM40");
    out_platforms.PushBack("DX11_SM41");
    out_platforms.PushBack("DX11_SM50");
  }

  virtual plResult ModifyShaderSource(plShaderProgramData& inout_data, plLogInterface* pLog) override;
  virtual plResult Compile(plShaderProgramData& inout_data, plLogInterface* pLog) override;

private:
  plResult DefineShaderResourceBindings(const plShaderProgramData& data, plHashTable<plHashedString, plShaderResourceBinding>& inout_resourceBinding, plLogInterface* pLog);

  void CreateNewShaderResourceDeclaration(plStringView sPlatform, plStringView sDeclaration, const plShaderResourceBinding& binding, plStringBuilder& out_sDeclaration);

  void ReflectShaderStage(plShaderProgramData& inout_Data, plGALShaderStage::Enum Stage);
  plShaderConstantBufferLayout* ReflectConstantBufferLayout(plGALShaderByteCode& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection);
  void Initialize();
  static plGALResourceFormat::Enum GetPLFormat(const _D3D11_SIGNATURE_PARAMETER_DESC& paramDesc);

private:
  plMap<const char*, plGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
