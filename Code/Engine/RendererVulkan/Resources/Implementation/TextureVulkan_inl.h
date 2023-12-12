vk::Image plGALTextureVulkan::GetImage() const
{
  return m_image;
}

vk::ImageLayout plGALTextureVulkan::GetPreferredLayout() const
{
  return m_preferredLayout;
}

vk::ImageLayout plGALTextureVulkan::GetPreferredLayout(vk::ImageLayout targetLayout) const
{
  return targetLayout;
  //#TODO_VULKAN Maintaining UAVs in general layout causes verification failures. For now, switch back and forth between layouts.
  //return m_preferredLayout == vk::ImageLayout::eGeneral ? vk::ImageLayout::eGeneral : targetLayout;
}

vk::PipelineStageFlags plGALTextureVulkan::GetUsedByPipelineStage() const
{
  return m_stages;
}

vk::AccessFlags plGALTextureVulkan::GetAccessMask() const
{
  return m_access;
}

plVulkanAllocation plGALTextureVulkan::GetAllocation() const
{
  return m_alloc;
}

const plVulkanAllocationInfo& plGALTextureVulkan::GetAllocationInfo() const
{
  return m_allocInfo;
}

bool plGALTextureVulkan::GetFormatOverrideEnabled() const
{
  return m_formatOverride;
}

bool plGALTextureVulkan::IsLinearLayout() const
{
  return m_bLinearCPU;
}

plGALTextureVulkan::StagingMode plGALTextureVulkan::GetStagingMode() const
{
  return m_stagingMode;
}

plGALTextureHandle plGALTextureVulkan::GetStagingTexture() const
{
  return m_hStagingTexture;
}

plGALBufferHandle plGALTextureVulkan::GetStagingBuffer() const
{
  return m_hStagingBuffer;
}
