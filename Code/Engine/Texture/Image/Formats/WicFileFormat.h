#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)

/// \brief File format implementation for loading TIFF files using WIC
class PL_TEXTURE_DLL plWicFileFormat : public plImageFileFormat
{
public:
  plWicFileFormat();
  virtual ~plWicFileFormat();

  virtual plResult ReadImageHeader(plStreamReader& inout_stream, plImageHeader& ref_header, plStringView sFileExtension) const override;
  virtual plResult ReadImage(plStreamReader& inout_stream, plImage& ref_image, plStringView sFileExtension) const override;
  virtual plResult WriteImage(plStreamWriter& inout_stream, const plImageView& image, plStringView sFileExtension) const override;

  virtual bool CanReadFileType(plStringView sExtension) const override;
  virtual bool CanWriteFileType(plStringView sExtension) const override;

private:
  mutable bool m_bTryCoInit = true; // Helper for keeping track of whether we have tried to init COM exactly once
  mutable bool m_bCoUninitOnShutdown =
    false; // Helper for keeping track of whether we have to uninitialize COM (because we were the first to initialize it)

  plResult ReadFileData(plStreamReader& stream, plDynamicArray<plUInt8>& storage) const;
};

#endif
