#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

plResult plTexConvProcessor::AssembleCubemap(plImage& dst) const
{
  PLASMA_PROFILE_SCOPE("AssembleCubemap");

  const auto& cm = m_Descriptor.m_ChannelMappings;
  const auto& images = m_Descriptor.m_InputImages;

  if (m_Descriptor.m_ChannelMappings.GetCount() == 6)
  {
    plImageView faces[6];
    faces[0] = images[cm[0].m_Channel[0].m_iInputImageIndex];
    faces[1] = images[cm[1].m_Channel[0].m_iInputImageIndex];
    faces[2] = images[cm[2].m_Channel[0].m_iInputImageIndex];
    faces[3] = images[cm[3].m_Channel[0].m_iInputImageIndex];
    faces[4] = images[cm[4].m_Channel[0].m_iInputImageIndex];
    faces[5] = images[cm[5].m_Channel[0].m_iInputImageIndex];

    if (plImageUtils::CreateCubemapFrom6Files(dst, faces).Failed())
    {
      plLog::Error("Failed to assemble cubemap from 6 images. Images must be square, with power-of-two resolutions.");
      return PLASMA_FAILURE;
    }
  }
  else
  {
    if (plImageUtils::CreateCubemapFromSingleFile(dst, images[0]).Failed())
    {
      plLog::Error("Failed to assemble cubemap from single image.");
      return PLASMA_FAILURE;
    }
  }

  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_TextureCube);
