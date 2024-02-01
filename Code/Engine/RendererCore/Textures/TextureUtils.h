#pragma once

#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <Texture/Image/Image.h>

struct PL_RENDERERCORE_DLL plTextureUtils
{
  static plGALResourceFormat::Enum ImageFormatToGalFormat(plImageFormat::Enum format, bool bSRGB);
  static plImageFormat::Enum GalFormatToImageFormat(plGALResourceFormat::Enum format, bool bRemoveSRGB);
  static plImageFormat::Enum GalFormatToImageFormat(plGALResourceFormat::Enum format);


  static void ConfigureSampler(plTextureFilterSetting::Enum filter, plGALSamplerStateCreationDescription& out_sampler);

  /// \brief If enabled, textures are always loaded to full quality immediately. Mostly necessary for image comparison unit tests.
  static bool s_bForceFullQualityAlways;
};
