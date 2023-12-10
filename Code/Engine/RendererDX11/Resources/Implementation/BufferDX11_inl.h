
PLASMA_ALWAYS_INLINE ID3D11Buffer* plGALBufferDX11::GetDXBuffer() const
{
  return m_pDXBuffer;
}

PLASMA_ALWAYS_INLINE DXGI_FORMAT plGALBufferDX11::GetIndexFormat() const
{
  return m_IndexFormat;
}
