#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

class PL_TEXTURE_DLL plBmpFileFormat : public plImageFileFormat
{
public:
  virtual plResult ReadImageHeader(plStreamReader& inout_stream, plImageHeader& ref_header, plStringView sFileExtension) const override;

  virtual plResult ReadImage(plStreamReader& inout_stream, plImage& ref_image, plStringView sFileExtension) const override;
  virtual plResult WriteImage(plStreamWriter& inout_stream, const plImageView& image, plStringView sFileExtension) const override;

  virtual bool CanReadFileType(plStringView sExtension) const override;
  virtual bool CanWriteFileType(plStringView sExtension) const override;
};
