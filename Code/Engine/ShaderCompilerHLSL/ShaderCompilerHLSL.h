#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerHLSL/ShaderCompilerHLSLDLL.h>

struct ID3D11ShaderReflectionConstantBuffer;
struct _D3D11_SIGNATURE_PARAMETER_DESC;

class PLASMA_SHADERCOMPILERHLSL_DLL plShaderCompilerHLSL : public plShaderProgramCompiler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plShaderCompilerHLSL, plShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(plHybridArray<plString, 4>& ref_platforms) override
  {
    ref_platforms.PushBack("DX11_SM40_93");
    ref_platforms.PushBack("DX11_SM40");
    ref_platforms.PushBack("DX11_SM41");
    ref_platforms.PushBack("DX11_SM50");
  }

  virtual plResult Compile(plShaderProgramData& inout_data, plLogInterface* pLog) override;
  virtual plResult DefineShaderResourceBindings(plShaderProgramData& inout_data, plHashTable<plHashedString, plShaderResourceBinding>& ref_resourceBinding, plLogInterface* pLog) override;
  virtual void CreateShaderResourceDeclaration(plStringView sDeclaration, const plShaderResourceBinding& binding, plStringBuilder& out_sDeclaration) override;

private:
  void ReflectShaderStage(plShaderProgramData& inout_Data, plGALShaderStage::Enum Stage);
  plShaderConstantBufferLayout* ReflectConstantBufferLayout(plGALShaderByteCode& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection);
  void Initialize();
  static plGALResourceFormat::Enum GetPLASMAFormat(const _D3D11_SIGNATURE_PARAMETER_DESC& paramDesc);

private:
  plMap<const char*, plGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
