#include <TexConv/TexConvPCH.h>

#include <TexConv/TexConv.h>

#include <Foundation/Utilities/CommandLineOptions.h>

plCommandLineOptionEnum opt_Mode("_TexConv", "-mode", "Mode determines which arguments need to be set.\n\
  In compare mode the mean-square error (MSE) is returned. 0 if it is below the threshold.\
",
  "Convert | Compare", 0);

plCommandLineOptionPath opt_Out("_TexConv", "-out",
  "Absolute path to main output file.\n\
   ext = tga, dds, plTexture2D, plTexture3D, plTextureCube or plTextureAtlas.",
  "");


plCommandLineOptionDoc opt_In("_TexConv", "-inX", "\"File\"",
  "Specifies input image X.\n\
   X = 0 .. 63, e.g. -in0, -in1, etc.\n\
   If X is not given, X equals 0.",
  "");

plCommandLineOptionDoc opt_Channels("_TexConv", "-r;-rg;-rgb;-rgba", "inX.rgba",
  "\
  Specifies how many output channels are used (1 - 4) and from which input image to take the data.\n\
  Examples:\n\
  -rgba in0 -> Output has 4 channels, all taken from input image 0.\n\
  -rgb in0 -> Output has 3 channels, all taken from input image 0.\n\
  -rgb in0 -a in1.r -> Output has 4 channels, RGB taken from input image 0 (RGB) Alpha taken from input 1 (Red).\n\
  -rgb in0.bgr -> Output has 3 channels, taken from image 0 and swapped blue and red.\n\
  -r in0.r -g in1.r -b in2.r -a in3.r -> Output has 4 channels, each one taken from another input image (Red).\n\
  -rgb0 in0 -rgb1 in1 -rgb2 in2 -rgb3 in3 -rgb4 in4 -rgb5 in5 -> Output has 3 channels and six faces (-type Cubemap), built from 6 images.\n\
",
  "");

plCommandLineOptionBool opt_MipsPreserveCoverage("_TexConv", "-mipsPreserveCoverage", "Whether to preserve alpha-coverage in mipmaps for alpha-tested geometry.", false);

plCommandLineOptionBool opt_FlipHorz("_TexConv", "-flip_horz", "Whether to flip the output horizontally.", false);

plCommandLineOptionBool opt_Dilate("_TexConv", "-dilate", "Dilate/smear color from opaque areas into transparent areas.", false);

plCommandLineOptionInt opt_DilateStrength("_TexConv", "-dilateStrength", "How many pixels to smear the image, if -dilate is enabled.", 8, 1, 255);

plCommandLineOptionBool opt_Premulalpha("_TexConv", "-premulalpha", "Whether to multiply the alpha channel into the RGB channels.", false);

plCommandLineOptionInt opt_ThumbnailRes("_TexConv", "-thumbnailRes", "Thumbnail resolution. Should be a power-of-two.", 0, 32, 1024);

plCommandLineOptionPath opt_ThumbnailOut("_TexConv", "-thumbnailOut",
  "\
  Path to 2D thumbnail file.\n\
  ext = tga, jpg, png\n\
",
  "");

plCommandLineOptionPath opt_LowOut("_TexConv", "-lowOut",
  "\
  Path to low-resolution output file.\n\
  ext = Same as main output\n\
",
  "");

plCommandLineOptionInt opt_LowMips("_TexConv", "-lowMips", "Number of mipmaps to use from main result as low-res data.", 0, 0, 8);

plCommandLineOptionInt opt_MinRes("_TexConv", "-minRes", "The minimum resolution allowed for the output.", 16, 4, 8 * 1024);

plCommandLineOptionInt opt_MaxRes("_TexConv", "-maxRes", "The maximum resolution allowed for the output.", 1024 * 8, 4, 16 * 1024);

plCommandLineOptionInt opt_Downscale("_TexConv", "-downscale", "How often to half the input texture resolution.", 0, 0, 10);

plCommandLineOptionFloat opt_MipsAlphaThreshold("_TexConv", "-mipsAlphaThreshold", "Alpha threshold used by renderer for alpha-testing, when alpha-coverage should be preserved.", 0.5f, 0.01f, 0.99f);

plCommandLineOptionFloat opt_HdrExposure("_TexConv", "-hdrExposure", "For scaling HDR image brightness up or down.", 0.0f, -20.0f, +20.0f);

plCommandLineOptionFloat opt_Clamp("_TexConv", "-clamp", "Input values will be clamped to [-value ; +value].", 64000.0f, -64000.0f, 64000.0f);

plCommandLineOptionInt opt_AssetVersion("_TexConv", "-assetVersion", "Asset version number to embed in pl specific output formats", 0, 1, 0xFFFF);

plCommandLineOptionString opt_AssetHashLow("_TexConv", "-assetHashLow", "Low part of a 64 bit asset hash value.\n\
Has to be specified as a HEX value.\n\
Required to be non-zero when using pl specific output formats.\n\
Example: -assetHashLow 0xABCDABCD",
  "");

plCommandLineOptionString opt_AssetHashHigh("_TexConv", "-assetHashHigh", "High part of a 64 bit asset hash value.\n\
Has to be specified as a HEX value.\n\
Required to be non-zero when using pl specific output formats.\n\
Example: -assetHashHigh 0xABCDABCD",
  "");

plCommandLineOptionEnum opt_Type("_TexConv", "-type", "The type of output to generate.", "2D = 1 | Volume = 2 | Cubemap = 3 | Atlas = 4", 1);

plCommandLineOptionEnum opt_Compression("_TexConv", "-compression", "Compression strength for output format.", "Medium = 1 | High = 2 | None = 0", 1);

plCommandLineOptionEnum opt_Usage("_TexConv", "-usage", "What type of data the image contains. Affects which final output format is used and how mipmaps are generated.", "Auto = 0 | Color = 1 | Linear = 2 | HDR = 3 | NormalMap = 4 | NormalMap_Inverted = 5 | BumpMap = 6", 0);

plCommandLineOptionEnum opt_Mipmaps("_TexConv", "-mipmaps", "Whether to generate mipmaps and with which algorithm.", "None = 0 |Linear = 1 | Kaiser = 2", 1);

plCommandLineOptionEnum opt_AddressU("_TexConv", "-addressU", "Which texture address mode to use along U. Only supported by pl-specific output formats.", "Repeat = 0 | Clamp = 1 | ClampBorder = 2 | Mirror = 3", 0);
plCommandLineOptionEnum opt_AddressV("_TexConv", "-addressV", "Which texture address mode to use along V. Only supported by pl-specific output formats.", "Repeat = 0 | Clamp = 1 | ClampBorder = 2 | Mirror = 3", 0);
plCommandLineOptionEnum opt_AddressW("_TexConv", "-addressW", "Which texture address mode to use along W. Only supported by pl-specific output formats.", "Repeat = 0 | Clamp = 1 | ClampBorder = 2 | Mirror = 3", 0);

plCommandLineOptionEnum opt_Filter("_TexConv", "-filter", "Which texture filter mode to use at runtime. Only supported by pl-specific output formats.", "Default = 9 | Lowest = 7 | Low = 8 | High = 10 | Highest = 11 | Nearest = 0 | Linear = 1 | Trilinear = 2 | Aniso2x = 3 | Aniso4x = 4 | Aniso8x = 5 | Aniso16x = 6", 9);

plCommandLineOptionEnum opt_BumpMapFilter("_TexConv", "-bumpMapFilter", "Filter used to approximate the x/y bump map gradients.", "Finite = 0 | Sobel = 1 | Scharr = 2", 0);

plCommandLineOptionEnum opt_Platform("_TexConv", "-platform", "What platform to generate the textures for.", "PC | Android", 0);

plCommandLineOptionString opt_CompareHtmlTitle("_TexConv", "-cmpHtml", "Title for the compare result HTML. If empty no HTML file is written.", "");
plCommandLineOptionPath opt_CompareActual("_TexConv", "-cmpImg", "Path to an image to compare with another.", "");
plCommandLineOptionPath opt_CompareExpected("_TexConv", "-cmpRef", "Path to a reference image to compare against.", "");
plCommandLineOptionInt opt_CompareThreshold("_TexConv", "-cmpMSE", "The error threshold for the comparison to be considered as failed.\n\
  No output files are written, if the image difference is below this value.",
  100, 0);
plCommandLineOptionBool opt_CompareRelaxed("_TexConv", "-cmpRelaxed", "Use a more lenient comparison method.\nUseful for images with single-pixel wide rasterized lines.", false);


plResult plTexConv::ParseCommandLine()
{
  if (plCommandLineOption::LogAvailableOptions(plCommandLineOption::LogAvailableModes::IfHelpRequested, "_TexConv"))
    return PL_FAILURE;

  PL_SUCCEED_OR_RETURN(ParseMode());

  if (m_Mode == plTexConvMode::Compare)
  {
    PL_SUCCEED_OR_RETURN(ParseCompareMode());
  }
  else
  {
    PL_SUCCEED_OR_RETURN(ParseOutputFiles());
    PL_SUCCEED_OR_RETURN(DetectOutputFormat());

    PL_SUCCEED_OR_RETURN(ParseOutputType());
    PL_SUCCEED_OR_RETURN(ParseAssetHeader());
    PL_SUCCEED_OR_RETURN(ParseTargetPlatform());
    PL_SUCCEED_OR_RETURN(ParseCompressionMode());
    PL_SUCCEED_OR_RETURN(ParseUsage());
    PL_SUCCEED_OR_RETURN(ParseMipmapMode());
    PL_SUCCEED_OR_RETURN(ParseWrapModes());
    PL_SUCCEED_OR_RETURN(ParseFilterModes());
    PL_SUCCEED_OR_RETURN(ParseResolutionModifiers());
    PL_SUCCEED_OR_RETURN(ParseMiscOptions());
    PL_SUCCEED_OR_RETURN(ParseInputFiles());
    PL_SUCCEED_OR_RETURN(ParseChannelMappings());
    PL_SUCCEED_OR_RETURN(ParseBumpMapFilter());
  }

  return PL_SUCCESS;
}

plResult plTexConv::ParseMode()
{
  switch (opt_Mode.GetOptionValue(plCommandLineOption::LogMode::FirstTime))
  {
    case 0:
      m_Mode = plTexConvMode::Convert;
      return PL_SUCCESS;

    case 1:
      m_Mode = plTexConvMode::Compare;
      return PL_SUCCESS;
  }

  plLog::Error("Invalid mode selected.");
  return PL_FAILURE;
}

plResult plTexConv::ParseCompareMode()
{
  m_sOutputFile = opt_Out.GetOptionValue(plCommandLineOption::LogMode::Always);

  if (m_sOutputFile.IsEmpty())
  {
    plLog::Warning("Output path is not specified. Use option '-out \"path\"' to set the prefix path for the output files.");
  }

  m_sHtmlTitle = opt_CompareHtmlTitle.GetOptionValue(plCommandLineOption::LogMode::FirstTime);

  plStringBuilder tmp, res;
  const auto pCmd = plCommandLineUtils::GetGlobalInstance();

  m_Comparer.m_Descriptor.m_sActualFile = opt_CompareActual.GetOptionValue(plCommandLineOption::LogMode::FirstTime);
  m_Comparer.m_Descriptor.m_sExpectedFile = opt_CompareExpected.GetOptionValue(plCommandLineOption::LogMode::FirstTime);
  m_Comparer.m_Descriptor.m_MeanSquareErrorThreshold = opt_CompareThreshold.GetOptionValue(plCommandLineOption::LogMode::FirstTime);
  m_Comparer.m_Descriptor.m_bRelaxedComparison = opt_CompareRelaxed.GetOptionValue(plCommandLineOption::LogMode::FirstTime);

  if (m_Comparer.m_Descriptor.m_sActualFile.IsEmpty())
  {
    plLog::Error("Image to compare is not specified.");
    return PL_FAILURE;
  }

  if (m_Comparer.m_Descriptor.m_sExpectedFile.IsEmpty())
  {
    plLog::Error("Reference image to compare against is not specified.");
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

plResult plTexConv::ParseOutputType()
{
  if (m_sOutputFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_OutputType = plTexConvOutputType::None;
    return PL_SUCCESS;
  }

  plInt32 value = opt_Type.GetOptionValue(plCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_OutputType = static_cast<plTexConvOutputType::Enum>(value);

  if (m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::Texture2D)
  {
    if (!m_bOutputSupports2D)
    {
      plLog::Error("2D textures are not supported by the chosen output file format.");
      return PL_FAILURE;
    }
  }
  else if (m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::Cubemap)
  {
    if (!m_bOutputSupportsCube)
    {
      plLog::Error("Cubemap textures are not supported by the chosen output file format.");
      return PL_FAILURE;
    }
  }
  else if (m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::Atlas)
  {
    if (!m_bOutputSupportsAtlas)
    {
      plLog::Error("Atlas textures are not supported by the chosen output file format.");
      return PL_FAILURE;
    }

    if (!ParseFile("-atlasDesc", m_Processor.m_Descriptor.m_sTextureAtlasDescFile))
      return PL_FAILURE;
  }
  else if (m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::Volume)
  {
    if (!m_bOutputSupports3D)
    {
      plLog::Error("Volume textures are not supported by the chosen output file format.");
      return PL_FAILURE;
    }
  }
  else
  {
    PL_ASSERT_NOT_IMPLEMENTED;
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

plResult plTexConv::ParseInputFiles()
{
  if (m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::Atlas)
    return PL_SUCCESS;

  plStringBuilder tmp, res;
  const auto pCmd = plCommandLineUtils::GetGlobalInstance();

  auto& files = m_Processor.m_Descriptor.m_InputFiles;

  for (plUInt32 i = 0; i < 64; ++i)
  {
    tmp.SetFormat("-in{0}", i);

    res = pCmd->GetAbsolutePathOption(tmp);

    // stop once an option was not found
    if (res.IsEmpty())
      break;

    files.EnsureCount(i + 1);
    files[i] = res;
  }

  // if no numbered inputs were given, try '-in', ignore it otherwise
  if (files.IsEmpty())
  {
    // short version for -in1
    res = pCmd->GetAbsolutePathOption("-in");

    if (!res.IsEmpty())
    {
      files.PushBack(res);
    }
  }

  if (m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::Cubemap)
  {
    // 0 = +X = Right
    // 1 = -X = Left
    // 2 = +Y = Top
    // 3 = -Y = Bottom
    // 4 = +Z = Front
    // 5 = -Z = Back

    if (files.IsEmpty() && (pCmd->GetOptionIndex("-right") != -1 || pCmd->GetOptionIndex("-px") != -1))
    {
      files.SetCount(6);

      files[0] = pCmd->GetAbsolutePathOption("-right", 0, files[0]);
      files[1] = pCmd->GetAbsolutePathOption("-left", 0, files[1]);
      files[2] = pCmd->GetAbsolutePathOption("-top", 0, files[2]);
      files[3] = pCmd->GetAbsolutePathOption("-bottom", 0, files[3]);
      files[4] = pCmd->GetAbsolutePathOption("-front", 0, files[4]);
      files[5] = pCmd->GetAbsolutePathOption("-back", 0, files[5]);

      files[0] = pCmd->GetAbsolutePathOption("-px", 0, files[0]);
      files[1] = pCmd->GetAbsolutePathOption("-nx", 0, files[1]);
      files[2] = pCmd->GetAbsolutePathOption("-py", 0, files[2]);
      files[3] = pCmd->GetAbsolutePathOption("-ny", 0, files[3]);
      files[4] = pCmd->GetAbsolutePathOption("-pz", 0, files[4]);
      files[5] = pCmd->GetAbsolutePathOption("-nz", 0, files[5]);
    }
  }

  for (plUInt32 i = 0; i < files.GetCount(); ++i)
  {
    if (files[i].IsEmpty())
    {
      plLog::Error("Input file {} is not specified", i);
      return PL_FAILURE;
    }

    plLog::Info("Input file {}: '{}'", i, files[i]);
  }

  if (m_Processor.m_Descriptor.m_InputFiles.IsEmpty())
  {
    plLog::Error("No input files were specified. Use \'-in \"path/to/file\"' to specify an input file. Use '-in0', '-in1' etc. to specify "
                 "multiple input files.");
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

plResult plTexConv::ParseOutputFiles()
{
  m_sOutputFile = opt_Out.GetOptionValue(plCommandLineOption::LogMode::Always);

  m_sOutputThumbnailFile = opt_ThumbnailOut.GetOptionValue(plCommandLineOption::LogMode::Always);

  if (!m_sOutputThumbnailFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_uiThumbnailOutputResolution = opt_ThumbnailRes.GetOptionValue(plCommandLineOption::LogMode::Always);
  }

  m_sOutputLowResFile = opt_LowOut.GetOptionValue(plCommandLineOption::LogMode::Always);

  if (!m_sOutputLowResFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_uiLowResMipmaps = opt_LowMips.GetOptionValue(plCommandLineOption::LogMode::Always);
  }

  return PL_SUCCESS;
}

plResult plTexConv::ParseUsage()
{
  if (m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::Atlas)
    return PL_SUCCESS;

  const plInt32 value = opt_Usage.GetOptionValue(plCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_Usage = static_cast<plTexConvUsage::Enum>(value);
  return PL_SUCCESS;
}

plResult plTexConv::ParseMipmapMode()
{
  if (!m_bOutputSupportsMipmaps)
  {
    plLog::Info("Selected output format does not support -mipmap options.");

    m_Processor.m_Descriptor.m_MipmapMode = plTexConvMipmapMode::None;
    return PL_SUCCESS;
  }

  const plInt32 value = opt_Mipmaps.GetOptionValue(plCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_MipmapMode = static_cast<plTexConvMipmapMode::Enum>(value);

  m_Processor.m_Descriptor.m_bPreserveMipmapCoverage = opt_MipsPreserveCoverage.GetOptionValue(plCommandLineOption::LogMode::Always);

  if (m_Processor.m_Descriptor.m_bPreserveMipmapCoverage)
  {
    m_Processor.m_Descriptor.m_fMipmapAlphaThreshold = opt_MipsAlphaThreshold.GetOptionValue(plCommandLineOption::LogMode::Always);
  }

  return PL_SUCCESS;
}

plResult plTexConv::ParseTargetPlatform()
{
  plInt32 value = opt_Platform.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified);

  m_Processor.m_Descriptor.m_TargetPlatform = static_cast<plTexConvTargetPlatform::Enum>(value);
  return PL_SUCCESS;
}

plResult plTexConv::ParseCompressionMode()
{
  if (!m_bOutputSupportsCompression)
  {
    plLog::Info("Selected output format does not support -compression options.");

    m_Processor.m_Descriptor.m_CompressionMode = plTexConvCompressionMode::None;
    return PL_SUCCESS;
  }

  const plInt32 value = opt_Compression.GetOptionValue(plCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_CompressionMode = static_cast<plTexConvCompressionMode::Enum>(value);
  return PL_SUCCESS;
}

plResult plTexConv::ParseWrapModes()
{
  // cubemaps do not require any wrap mode settings
  if (m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::Cubemap || m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::Atlas || m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::None)
    return PL_SUCCESS;

  {
    plInt32 value = opt_AddressU.GetOptionValue(plCommandLineOption::LogMode::Always);
    m_Processor.m_Descriptor.m_AddressModeU = static_cast<plImageAddressMode::Enum>(value);
  }
  {
    plInt32 value = opt_AddressV.GetOptionValue(plCommandLineOption::LogMode::Always);
    m_Processor.m_Descriptor.m_AddressModeV = static_cast<plImageAddressMode::Enum>(value);
  }

  if (m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::Volume)
  {
    plInt32 value = opt_AddressW.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified);
    m_Processor.m_Descriptor.m_AddressModeW = static_cast<plImageAddressMode::Enum>(value);
  }

  return PL_SUCCESS;
}

plResult plTexConv::ParseFilterModes()
{
  if (!m_bOutputSupportsFiltering)
  {
    plLog::Info("Selected output format does not support -filter options.");
    return PL_SUCCESS;
  }

  plInt32 value = opt_Filter.GetOptionValue(plCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_FilterMode = static_cast<plTextureFilterSetting::Enum>(value);
  return PL_SUCCESS;
}

plResult plTexConv::ParseResolutionModifiers()
{
  if (m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::None)
    return PL_SUCCESS;

  m_Processor.m_Descriptor.m_uiMinResolution = opt_MinRes.GetOptionValue(plCommandLineOption::LogMode::Always);
  m_Processor.m_Descriptor.m_uiMaxResolution = opt_MaxRes.GetOptionValue(plCommandLineOption::LogMode::Always);
  m_Processor.m_Descriptor.m_uiDownscaleSteps = opt_Downscale.GetOptionValue(plCommandLineOption::LogMode::Always);

  return PL_SUCCESS;
}

plResult plTexConv::ParseMiscOptions()
{
  if (m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::Texture2D || m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::None)
  {
    m_Processor.m_Descriptor.m_bFlipHorizontal = opt_FlipHorz.GetOptionValue(plCommandLineOption::LogMode::Always);

    m_Processor.m_Descriptor.m_bPremultiplyAlpha = opt_Premulalpha.GetOptionValue(plCommandLineOption::LogMode::Always);

    if (opt_Dilate.GetOptionValue(plCommandLineOption::LogMode::Always))
    {
      m_Processor.m_Descriptor.m_uiDilateColor = static_cast<plUInt8>(opt_DilateStrength.GetOptionValue(plCommandLineOption::LogMode::Always));
    }
  }

  if (m_Processor.m_Descriptor.m_Usage == plTexConvUsage::Hdr)
  {
    m_Processor.m_Descriptor.m_fHdrExposureBias = opt_HdrExposure.GetOptionValue(plCommandLineOption::LogMode::Always);
  }

  m_Processor.m_Descriptor.m_fMaxValue = opt_Clamp.GetOptionValue(plCommandLineOption::LogMode::Always);

  return PL_SUCCESS;
}

plResult plTexConv::ParseAssetHeader()
{
  const plStringView ext = plPathUtils::GetFileExtension(m_sOutputFile);

  if (!ext.StartsWith_NoCase("pl"))
    return PL_SUCCESS;

  m_Processor.m_Descriptor.m_uiAssetVersion = (plUInt16)opt_AssetVersion.GetOptionValue(plCommandLineOption::LogMode::Always);

  plUInt32 uiHashLow = 0;
  plUInt32 uiHashHigh = 0;
  if (plConversionUtils::ConvertHexStringToUInt32(opt_AssetHashLow.GetOptionValue(plCommandLineOption::LogMode::Always), uiHashLow).Failed() ||
      plConversionUtils::ConvertHexStringToUInt32(opt_AssetHashHigh.GetOptionValue(plCommandLineOption::LogMode::Always), uiHashHigh).Failed())
  {
    plLog::Error("'-assetHashLow 0xHEX32' and '-assetHashHigh 0xHEX32' have not been specified correctly.");
    return PL_FAILURE;
  }

  m_Processor.m_Descriptor.m_uiAssetHash = (static_cast<plUInt64>(uiHashHigh) << 32) | static_cast<plUInt64>(uiHashLow);

  if (m_Processor.m_Descriptor.m_uiAssetHash == 0)
  {
    plLog::Error("'-assetHashLow 0xHEX32' and '-assetHashHigh 0xHEX32' have not been specified correctly.");
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

plResult plTexConv::ParseBumpMapFilter()
{
  const plInt32 value = opt_BumpMapFilter.GetOptionValue(plCommandLineOption::LogMode::Always);

  m_Processor.m_Descriptor.m_BumpMapFilter = static_cast<plTexConvBumpMapFilter::Enum>(value);
  return PL_SUCCESS;
}
