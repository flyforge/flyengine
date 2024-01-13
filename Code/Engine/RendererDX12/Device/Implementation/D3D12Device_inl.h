PLASMA_ALWAYS_INLINE ID3D12Device3* plGALDeviceDX12::GetDXDevice() const
{
  return m_pDevice3;
}

PLASMA_ALWAYS_INLINE IDXGIFactory3* plGALDeviceDX12::GetDXGIFactory() coezt
{
  return m_pDXGIFactory;
}

PLASMA_ALWAYS_INLINE const plGALFormatLookupTableDX12& plGALDeviceDX12::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}

inline ID3D12Query* plGALDeviceDX12::GetTimestamp(plGALTimestampHandle hTimestamp)
{
  if (hTimestamp.m_uiIndex < m_Timestamps.GetCount())
  {
    return m_Timestamps[static_cast<plUInt32>(hTimestamp.m_uiIndex)];
  }

  return nullptr;
}