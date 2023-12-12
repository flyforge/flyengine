#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Query.h>

plGALQuery::plGALQuery(const plGALQueryCreationDescription& Description)
  : plGALResource<plGALQueryCreationDescription>(Description)
  , m_bStarted(false)
{
}

plGALQuery::~plGALQuery() {}

PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Query);
