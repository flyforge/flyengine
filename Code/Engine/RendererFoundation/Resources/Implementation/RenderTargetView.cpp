#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/RenderTargetView.h>


plGALRenderTargetView::plGALRenderTargetView(plGALTexture* pTexture, const plGALRenderTargetViewCreationDescription& description)
  : plGALObject(description)
  , m_pTexture(pTexture)
{
  PL_ASSERT_DEV(m_pTexture != nullptr, "Texture must not be null");
}

plGALRenderTargetView::~plGALRenderTargetView() = default;


