#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Texture.h>

plGALTexture::plGALTexture(const plGALTextureCreationDescription& Description)
  : plGALResource(Description)
{
}

plGALTexture::~plGALTexture() = default;



PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Texture);
