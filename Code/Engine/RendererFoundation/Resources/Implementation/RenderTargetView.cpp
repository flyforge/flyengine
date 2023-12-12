#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/RenderTargetView.h>


plGALRenderTargetView::plGALRenderTargetView(plGALTexture* pTexture, const plGALRenderTargetViewCreationDescription& description)
  : plGALObject(description)
  , m_pTexture(pTexture)
{
  PLASMA_ASSERT_DEV(m_pTexture != nullptr, "Texture must not be null");
}

plGALRenderTargetView::~plGALRenderTargetView() {}



PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_RenderTargetView);
