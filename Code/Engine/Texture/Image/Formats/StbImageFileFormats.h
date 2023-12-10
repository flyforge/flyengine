#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

/// Png and jpeg file format support using stb_image.
///
/// stb_image also supports other formats, but we stick to our own loader code where we can.
/// Also, stb HDR image support is not handled here yet.
class PLASMA_TEXTURE_DLL plStbImageFileFormats : public plImageFileFormat
{
public:
  virtual plResult ReadImageHeader(plStreamReader& inout_stream, plImageHeader& ref_header, plStringView sFileExtension) const override;
  virtual plResult ReadImage(plStreamReader& inout_stream, plImage& ref_image, plStringView sFileExtension) const override;
  virtual plResult WriteImage(plStreamWriter& inout_stream, const plImageView& image, plStringView sFileExtension) const override;

  virtual bool CanReadFileType(plStringView sExtension) const override;
  virtual bool CanWriteFileType(plStringView sExtension) const override;
};
