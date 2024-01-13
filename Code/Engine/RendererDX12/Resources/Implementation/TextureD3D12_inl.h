PLASMA_ALWAYS_INLINE ID3D12Resouce* plGALTextureD3D12::GetDXTexture() const
{
  return m_finaltextureresource;
}

PLASMA_ALWAYS_INLINE ID3D12Resouce* plGALTextureD3D12::GetDXStagingTexture() const
{
  return m_temptextureresource;
}