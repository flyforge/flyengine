#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if PL_DISABLED(PL_PLATFORM_WINDOWS_UWP)

/// EXR file format support using TinyEXR.
class PL_TEXTURE_DLL plExrFileFormat : public plImageFileFormat
{
public:
  plResult ReadImageHeader(plStreamReader& ref_stream, plImageHeader& ref_header, plStringView sFileExtension) const override;
  plResult ReadImage(plStreamReader& ref_stream, plImage& ref_image, plStringView sFileExtension) const override;
  plResult WriteImage(plStreamWriter& ref_stream, const plImageView& image, plStringView sFileExtension) const override;

  bool CanReadFileType(plStringView sExtension) const override;
  bool CanWriteFileType(plStringView sExtension) const override;
};

#endif
