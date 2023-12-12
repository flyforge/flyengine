
vk::ShaderModule plGALShaderVulkan::GetShader(plGALShaderStage::Enum stage) const
{
  return m_Shaders[stage];
}

const plGALShaderVulkan::DescriptorSetLayoutDesc& plGALShaderVulkan::GetDescriptorSetLayout() const
{
  return m_descriptorSetLayoutDesc;
}

const plArrayPtr<const plGALShaderVulkan::BindingMapping> plGALShaderVulkan::GetBindingMapping() const
{
  return m_BindingMapping;
}

const plArrayPtr<const plGALShaderVulkan::VertexInputAttribute> plGALShaderVulkan::GetVertexInputAttributes() const
{
  return m_VertexInputAttributes;
}
