

PLASMA_ALWAYS_INLINE ID3D11RenderTargetView* plGALRenderTargetViewDX11::GetRenderTargetView() const
{
  return m_pRenderTargetView;
}

PLASMA_ALWAYS_INLINE ID3D11DepthStencilView* plGALRenderTargetViewDX11::GetDepthStencilView() const
{
  return m_pDepthStencilView;
}

PLASMA_ALWAYS_INLINE ID3D11UnorderedAccessView* plGALRenderTargetViewDX11::GetUnorderedAccessView() const
{
  return m_pUnorderedAccessView;
}