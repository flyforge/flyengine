
vk::ShaderModule plGALShaderVulkan::GetShader(plGALShaderStage::Enum stage) const
{
  return m_Shaders[stage];
}

plUInt32 plGALShaderVulkan::GetSetCount() const
{
  return m_SetBindings.GetCount();
}

const plGALShaderVulkan::DescriptorSetLayoutDesc& plGALShaderVulkan::GetDescriptorSetLayout(plUInt32 uiSet) const
{
  PLASMA_ASSERT_DEBUG(uiSet < m_descriptorSetLayoutDesc.GetCount(), "Set index out of range.");
  return m_descriptorSetLayoutDesc[uiSet];
}

plArrayPtr<const plShaderResourceBinding> plGALShaderVulkan::GetBindings(plUInt32 uiSet) const
{
  PLASMA_ASSERT_DEBUG(uiSet < m_descriptorSetLayoutDesc.GetCount(), "Set index out of range.");
  return m_SetBindings[uiSet].GetArrayPtr();
}