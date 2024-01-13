PLASMA_ALWAYS_INLINE ID3D12Resource* plGALRenderTargetViewD3D12::GetRenderTargetView() const
{
  return m_rtvresource;
}

PLASMA_ALWAYS_INLINE ID3D12Resource* plGALRenderTargetViewD3D12::GetDepthStencilView() const
{
  return m_dsvresource;
}

PLASMA_ALWAYS_INLINE ID3D12Resource* plGALRenderTargetViewD3D12::GetUnorderedAccessView() const
{
  return m_uavresource;
}