#pragma once

#include <Texture/Image/ImageFormat.h>

/// \brief Helper class containing methods to convert between plImageFormat::Enum and platform-specific image formats.
class PL_TEXTURE_DLL plImageFormatMappings
{
public:
  /// \brief Maps an plImageFormat::Enum to an equivalent Direct3D DXGI_FORMAT.
  static plUInt32 ToDxgiFormat(plImageFormat::Enum format);

  /// \brief Maps a Direct3D DXGI_FORMAT to an equivalent plImageFormat::Enum.
  static plImageFormat::Enum FromDxgiFormat(plUInt32 uiDxgiFormat);

  /// \brief Maps an plImageFormat::Enum to an equivalent FourCC code.
  static plUInt32 ToFourCc(plImageFormat::Enum format);

  /// \brief Maps a FourCC code to an equivalent plImageFormat::Enum.
  static plImageFormat::Enum FromFourCc(plUInt32 uiFourCc);
};
