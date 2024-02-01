#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/UnorderedAccesView.h>

plGALUnorderedAccessView::plGALUnorderedAccessView(plGALResourceBase* pResource, const plGALUnorderedAccessViewCreationDescription& description)
  : plGALObject(description)
  , m_pResource(pResource)
{
  PL_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

plGALUnorderedAccessView::~plGALUnorderedAccessView() = default;


