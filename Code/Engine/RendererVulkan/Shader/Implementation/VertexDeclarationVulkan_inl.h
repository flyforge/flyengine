

plArrayPtr<const vk::VertexInputAttributeDescription> plGALVertexDeclarationVulkan::GetAttributes() const
{
  return m_attributes.GetArrayPtr();
}

plArrayPtr<const vk::VertexInputBindingDescription> plGALVertexDeclarationVulkan::GetBindings() const
{
  return m_bindings.GetArrayPtr();
}
