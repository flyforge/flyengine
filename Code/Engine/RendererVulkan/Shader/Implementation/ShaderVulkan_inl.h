
vk::ShaderModule plGALShaderVulkan::GetShader(plGALShaderStage::Enum stage) const
{
  return m_Shaders[stage];
}

plUInt32 plGALShaderVulkan::GetSetCount() const
{
  return m_SetBindings.GetCount();
}

vk::DescriptorSetLayout plGALShaderVulkan::GetDescriptorSetLayout(plUInt32 uiSet) const
{
  PL_ASSERT_DEBUG(uiSet < m_descriptorSetLayout.GetCount(), "Set index out of range.");
  return m_descriptorSetLayout[uiSet];
}

plArrayPtr<const plShaderResourceBinding> plGALShaderVulkan::GetBindings(plUInt32 uiSet) const
{
  PL_ASSERT_DEBUG(uiSet < m_SetBindings.GetCount(), "Set index out of range.");
  return m_SetBindings[uiSet].GetArrayPtr();
}

vk::PushConstantRange plGALShaderVulkan::GetPushConstantRange() const
{
  return m_pushConstants;
}
