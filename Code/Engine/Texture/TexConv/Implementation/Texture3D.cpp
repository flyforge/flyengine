#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

plResult plTexConvProcessor::Assemble3DTexture(plImage& dst) const
{
  PL_PROFILE_SCOPE("Assemble3DTexture");

  const auto& images = m_Descriptor.m_InputImages;

  return plImageUtils::CreateVolumeTextureFromSingleFile(dst, images[0]);
}


