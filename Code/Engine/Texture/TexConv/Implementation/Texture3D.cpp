#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

plResult plTexConvProcessor::Assemble3DTexture(plImage& dst) const
{
  PLASMA_PROFILE_SCOPE("Assemble3DTexture");

  const auto& images = m_Descriptor.m_InputImages;

  return plImageUtils::CreateVolumeTextureFromSingleFile(dst, images[0]);
}


PLASMA_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Texture3D);
