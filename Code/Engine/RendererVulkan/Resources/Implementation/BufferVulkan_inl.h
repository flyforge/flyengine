
vk::Buffer plGALBufferVulkan::GetVkBuffer() const
{
  m_currentBuffer.m_currentFrame = m_pDeviceVulkan->GetCurrentFrame();
  return m_currentBuffer.m_buffer;
}

vk::IndexType plGALBufferVulkan::GetIndexType() const
{
  return m_indexType;
}

plVulkanAllocation plGALBufferVulkan::GetAllocation() const
{
  return m_currentBuffer.m_alloc;
}

const plVulkanAllocationInfo& plGALBufferVulkan::GetAllocationInfo() const
{
  return m_allocInfo;
}

vk::PipelineStageFlags plGALBufferVulkan::GetUsedByPipelineStage() const
{
  return m_stages;
}

vk::AccessFlags plGALBufferVulkan::GetAccessMask() const
{
  return m_access;
}
