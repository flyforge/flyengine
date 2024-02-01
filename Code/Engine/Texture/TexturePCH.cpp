#include <Texture/TexturePCH.h>

PL_STATICLINK_LIBRARY(Texture)
{
  if (bReturn)
    return;

  PL_STATICLINK_REFERENCE(Texture_Image_Conversions_DXTConversions);
  PL_STATICLINK_REFERENCE(Texture_Image_Conversions_DXTexConversions);
  PL_STATICLINK_REFERENCE(Texture_Image_Conversions_DXTexCpuConversions);
  PL_STATICLINK_REFERENCE(Texture_Image_Conversions_PixelConversions);
  PL_STATICLINK_REFERENCE(Texture_Image_Conversions_PlanarConversions);
  PL_STATICLINK_REFERENCE(Texture_Image_Formats_BmpFileFormat);
  PL_STATICLINK_REFERENCE(Texture_Image_Formats_DdsFileFormat);
  PL_STATICLINK_REFERENCE(Texture_Image_Formats_ExrFileFormat);
  PL_STATICLINK_REFERENCE(Texture_Image_Formats_StbImageFileFormats);
  PL_STATICLINK_REFERENCE(Texture_Image_Formats_TgaFileFormat);
  PL_STATICLINK_REFERENCE(Texture_Image_Formats_WicFileFormat);
  PL_STATICLINK_REFERENCE(Texture_Image_Implementation_ImageEnums);
  PL_STATICLINK_REFERENCE(Texture_Image_Implementation_ImageFormat);
  PL_STATICLINK_REFERENCE(Texture_TexConv_Implementation_Processor);
}
