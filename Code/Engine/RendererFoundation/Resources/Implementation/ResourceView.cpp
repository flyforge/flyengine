#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ResourceView.h>


plGALResourceView::plGALResourceView(plGALResourceBase* pResource, const plGALResourceViewCreationDescription& description)
  : plGALObject(description)
  , m_pResource(pResource)
{
  PLASMA_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

plGALResourceView::~plGALResourceView() {}



PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_ResourceView);
