#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Texture/TextureDLL.h>

class plStreamReader;
class plStreamWriter;
class plImage;
class plImageView;
class plStringBuilder;
class plImageHeader;

class PLASMA_TEXTURE_DLL plImageFileFormat : public plEnumerable<plImageFileFormat>
{
public:
  /// \brief Reads only the header information for an image and ignores the data. Much faster than reading the entire image, if the pixel data is not needed.
  virtual plResult ReadImageHeader(plStreamReader& inout_stream, plImageHeader& ref_header, plStringView sFileExtension) const = 0;

  /// \brief Reads the data from the given stream and creates the image from it. Errors are written to the given plLogInterface.
  virtual plResult ReadImage(plStreamReader& inout_stream, plImage& ref_image, plStringView sFileExtension) const = 0;

  /// \brief Writes the data to the given stream in this format. Errors are written to the given plLogInterface.
  virtual plResult WriteImage(plStreamWriter& inout_stream, const plImageView& image, plStringView sFileExtension) const = 0;

  /// \brief Should return true, if files with the given extension can be read.
  virtual bool CanReadFileType(plStringView sExtension) const = 0;

  /// \brief Should return true, if files with the given extension can be written.
  virtual bool CanWriteFileType(plStringView sExtension) const = 0;

  /// \brief Returns an plImageFileFormat that can read the given extension. Returns nullptr if there is no appropriate plImageFileFormat.
  static plImageFileFormat* GetReaderFormat(plStringView sExtension);

  /// \brief Returns an plImageFileFormat that can write the given extension. Returns nullptr if there is no appropriate plImageFileFormat.
  static plImageFileFormat* GetWriterFormat(plStringView sExtension);

  static plResult ReadImageHeader(plStringView sFileName, plImageHeader& ref_header);

  PLASMA_DECLARE_ENUMERABLE_CLASS(plImageFileFormat);
};
