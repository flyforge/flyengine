#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

bool plGALRenderTargets::operator==(const plGALRenderTargets& other) const
{
  if (m_hDSTarget != other.m_hDSTarget)
    return false;

  for (plUInt8 uiRTIndex = 0; uiRTIndex < PL_GAL_MAX_RENDERTARGET_COUNT; ++uiRTIndex)
  {
    if (m_hRTs[uiRTIndex] != other.m_hRTs[uiRTIndex])
      return false;
  }
  return true;
}

bool plGALRenderTargets::operator!=(const plGALRenderTargets& other) const
{
  return !(*this == other);
}

plGALRenderTargetSetup::plGALRenderTargetSetup() = default;

plGALRenderTargetSetup& plGALRenderTargetSetup::SetRenderTarget(plUInt8 uiIndex, plGALRenderTargetViewHandle hRenderTarget)
{
  PL_ASSERT_DEV(uiIndex < PL_GAL_MAX_RENDERTARGET_COUNT, "Render target index out of bounds - should be less than PL_GAL_MAX_RENDERTARGET_COUNT");

  m_hRTs[uiIndex] = hRenderTarget;

  m_uiRTCount = plMath::Max(m_uiRTCount, static_cast<plUInt8>(uiIndex + 1u));

  return *this;
}

plGALRenderTargetSetup& plGALRenderTargetSetup::SetDepthStencilTarget(plGALRenderTargetViewHandle hDSTarget)
{
  m_hDSTarget = hDSTarget;

  return *this;
}

bool plGALRenderTargetSetup::operator==(const plGALRenderTargetSetup& other) const
{
  if (m_hDSTarget != other.m_hDSTarget)
    return false;

  if (m_uiRTCount != other.m_uiRTCount)
    return false;

  for (plUInt8 uiRTIndex = 0; uiRTIndex < m_uiRTCount; ++uiRTIndex)
  {
    if (m_hRTs[uiRTIndex] != other.m_hRTs[uiRTIndex])
      return false;
  }

  return true;
}

bool plGALRenderTargetSetup::operator!=(const plGALRenderTargetSetup& other) const
{
  return !(*this == other);
}

void plGALRenderTargetSetup::DestroyAllAttachedViews()
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plArrayPtr<plGALRenderTargetViewHandle> colorViews(m_hRTs);
  for (plGALRenderTargetViewHandle& hView : colorViews)
  {
    if (!hView.IsInvalidated())
    {
      pDevice->DestroyRenderTargetView(hView);
      hView.Invalidate();
    }
  }

  if (!m_hDSTarget.IsInvalidated())
  {
    pDevice->DestroyRenderTargetView(m_hDSTarget);
    m_hDSTarget.Invalidate();
  }
  m_uiRTCount = 0;
}

bool plGALRenderingSetup::operator==(const plGALRenderingSetup& other) const
{
  return m_RenderTargetSetup == other.m_RenderTargetSetup && m_ClearColor == other.m_ClearColor && m_uiRenderTargetClearMask == other.m_uiRenderTargetClearMask && m_fDepthClear == other.m_fDepthClear && m_uiStencilClear == other.m_uiStencilClear && m_bClearDepth == other.m_bClearDepth && m_bClearStencil == other.m_bClearStencil && m_bDiscardColor == other.m_bDiscardColor && m_bDiscardDepth == other.m_bDiscardDepth;
}

bool plGALRenderingSetup::operator!=(const plGALRenderingSetup& other) const
{
  return !(*this == other);
}


