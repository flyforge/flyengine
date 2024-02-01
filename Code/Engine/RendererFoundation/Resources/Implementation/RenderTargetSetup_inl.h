
#pragma once

plUInt8 plGALRenderTargetSetup::GetRenderTargetCount() const
{
  return m_uiRTCount;
}

plGALRenderTargetViewHandle plGALRenderTargetSetup::GetRenderTarget(plUInt8 uiIndex) const
{
  PL_ASSERT_DEBUG(uiIndex < m_uiRTCount, "Render target index out of range");

  return m_hRTs[uiIndex];
}

plGALRenderTargetViewHandle plGALRenderTargetSetup::GetDepthStencilTarget() const
{
  return m_hDSTarget;
}
