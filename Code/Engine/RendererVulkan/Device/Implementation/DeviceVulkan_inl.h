PL_ALWAYS_INLINE vk::Device plGALDeviceVulkan::GetVulkanDevice() const
{
  return m_device;
}

PL_ALWAYS_INLINE const plGALDeviceVulkan::Queue& plGALDeviceVulkan::GetGraphicsQueue() const
{
  return m_graphicsQueue;
}

PL_ALWAYS_INLINE const plGALDeviceVulkan::Queue& plGALDeviceVulkan::GetTransferQueue() const
{
  return m_transferQueue;
}

PL_ALWAYS_INLINE vk::PhysicalDevice plGALDeviceVulkan::GetVulkanPhysicalDevice() const
{
  return m_physicalDevice;
}

PL_ALWAYS_INLINE vk::Instance plGALDeviceVulkan::GetVulkanInstance() const
{
  return m_instance;
}


PL_ALWAYS_INLINE const plGALFormatLookupTableVulkan& plGALDeviceVulkan::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}

/*
inline ID3D11Query* plGALDeviceVulkan::GetTimestamp(plGALTimestampHandle hTimestamp)
{
  if (hTimestamp.m_uiIndex < m_Timestamps.GetCount())
  {
    return m_Timestamps[static_cast<plUInt32>(hTimestamp.m_uiIndex)];
  }

  return nullptr;
}

*/
