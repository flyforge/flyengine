#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Buffer.h>

plGALBuffer::plGALBuffer(const plGALBufferCreationDescription& Description)
  : plGALResource(Description)
{
}

plGALBuffer::~plGALBuffer() = default;



PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Buffer);
