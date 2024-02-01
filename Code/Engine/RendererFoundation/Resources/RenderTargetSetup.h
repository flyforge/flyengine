
#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// \brief This class can be used to define the render targets to be used by an plView.
struct PL_RENDERERFOUNDATION_DLL plGALRenderTargets
{
  bool operator==(const plGALRenderTargets& other) const;
  bool operator!=(const plGALRenderTargets& other) const;

  plGALTextureHandle m_hRTs[PL_GAL_MAX_RENDERTARGET_COUNT];
  plGALTextureHandle m_hDSTarget;
};

// \brief This class can be used to construct render target setups on the stack.
class PL_RENDERERFOUNDATION_DLL plGALRenderTargetSetup
{
public:
  plGALRenderTargetSetup();

  plGALRenderTargetSetup& SetRenderTarget(plUInt8 uiIndex, plGALRenderTargetViewHandle hRenderTarget);
  plGALRenderTargetSetup& SetDepthStencilTarget(plGALRenderTargetViewHandle hDSTarget);

  bool operator==(const plGALRenderTargetSetup& other) const;
  bool operator!=(const plGALRenderTargetSetup& other) const;

  inline plUInt8 GetRenderTargetCount() const;

  inline plGALRenderTargetViewHandle GetRenderTarget(plUInt8 uiIndex) const;
  inline plGALRenderTargetViewHandle GetDepthStencilTarget() const;

  void DestroyAllAttachedViews();

protected:
  plGALRenderTargetViewHandle m_hRTs[PL_GAL_MAX_RENDERTARGET_COUNT];
  plGALRenderTargetViewHandle m_hDSTarget;

  plUInt8 m_uiRTCount = 0;
};

struct PL_RENDERERFOUNDATION_DLL plGALRenderingSetup
{
  bool operator==(const plGALRenderingSetup& other) const;
  bool operator!=(const plGALRenderingSetup& other) const;

  plGALRenderTargetSetup m_RenderTargetSetup;
  plColor m_ClearColor = plColor(0, 0, 0, 0);
  plUInt32 m_uiRenderTargetClearMask = 0x0;
  float m_fDepthClear = 1.0f;
  plUInt8 m_uiStencilClear = 0;
  bool m_bClearDepth = false;
  bool m_bClearStencil = false;
  bool m_bDiscardColor = false;
  bool m_bDiscardDepth = false;
};

#include <RendererFoundation/Resources/Implementation/RenderTargetSetup_inl.h>
