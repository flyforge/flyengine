
PL_ALWAYS_INLINE ID3D11Device* plGALDeviceDX11::GetDXDevice() const
{
  return m_pDevice;
}

PL_ALWAYS_INLINE ID3D11Device3* plGALDeviceDX11::GetDXDevice3() const
{
  return m_pDevice3;
}

PL_ALWAYS_INLINE ID3D11DeviceContext* plGALDeviceDX11::GetDXImmediateContext() const
{
  return m_pImmediateContext;
}

PL_ALWAYS_INLINE IDXGIFactory1* plGALDeviceDX11::GetDXGIFactory() const
{
  return m_pDXGIFactory;
}

PL_ALWAYS_INLINE const plGALFormatLookupTableDX11& plGALDeviceDX11::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}

inline ID3D11Query* plGALDeviceDX11::GetTimestamp(plGALTimestampHandle hTimestamp)
{
  if (hTimestamp.m_uiIndex < m_Timestamps.GetCount())
  {
    return m_Timestamps[static_cast<plUInt32>(hTimestamp.m_uiIndex)];
  }

  return nullptr;
}
