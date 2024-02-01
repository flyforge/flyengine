#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ResourceView.h>


plGALResourceView::plGALResourceView(plGALResourceBase* pResource, const plGALResourceViewCreationDescription& description)
  : plGALObject(description)
  , m_pResource(pResource)
{
  PL_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

plGALResourceView::~plGALResourceView() = default;


