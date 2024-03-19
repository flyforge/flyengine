#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

plGALShader::plGALShader(const plGALShaderCreationDescription& Description)
  : plGALObject(Description)
{
}
plArrayPtr<const plShaderResourceBinding> plGALShader::GetBindingMapping() const
{
  return m_BindingMapping;
}

const plShaderResourceBinding* plGALShader::GetShaderResourceBinding(const plTempHashedString& sName) const
{
  for (auto& binding : m_BindingMapping)
  {
    if (binding.m_sName == sName)
    {
      return &binding;
    }
  }
  return nullptr;
}

plArrayPtr<const plShaderVertexInputAttribute> plGALShader::GetVertexInputAttributes() const
{
  if (m_Description.HasByteCodeForStage(plGALShaderStage::VertexShader))
  {
    return m_Description.m_ByteCodes[plGALShaderStage::VertexShader]->m_ShaderVertexInput;
  }
  return {};
}

plResult plGALShader::CreateBindingMapping()
{
  plHybridArray<plArrayPtr<const plShaderResourceBinding>, plGALShaderStage::ENUM_COUNT> resourceBinding;
  resourceBinding.SetCount(plGALShaderStage::ENUM_COUNT);
  for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_Description.HasByteCodeForStage((plGALShaderStage::Enum)stage))
    {
      resourceBinding[stage] = m_Description.m_ByteCodes[stage]->m_ShaderResourceBindings;
    }
  }
  return plShaderResourceBinding::CreateMergedShaderResourceBinding(resourceBinding, m_BindingMapping);
}

void plGALShader::DestroyBindingMapping()
{
  m_BindingMapping.Clear();
}

plGALShader::~plGALShader() = default;

plDelegate<void(plShaderUtils::plBuiltinShaderType type, plShaderUtils::plBuiltinShader& out_shader)> plShaderUtils::g_RequestBuiltinShaderCallback;


