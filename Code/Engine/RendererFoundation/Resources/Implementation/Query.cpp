#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Query.h>

plGALQuery::plGALQuery(const plGALQueryCreationDescription& Description)
  : plGALResource<plGALQueryCreationDescription>(Description)

{
}

plGALQuery::~plGALQuery() = default;

PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Query);
