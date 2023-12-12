#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureAssetDocument, 6, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plTextureChannelMode, 1)
  PLASMA_ENUM_CONSTANT(plTextureChannelMode::RGBA)->AddAttributes(new plGroupAttribute("Multi", 0.0f)),
  PLASMA_ENUM_CONSTANT(plTextureChannelMode::RGB)->AddAttributes(new plGroupAttribute("Multi", 1.0f)),
  PLASMA_ENUM_CONSTANT(plTextureChannelMode::Red)->AddAttributes(new plGroupAttribute("Single", 0.0f)),
  PLASMA_ENUM_CONSTANT(plTextureChannelMode::Green)->AddAttributes(new plGroupAttribute("Single", 1.0f)),
  PLASMA_ENUM_CONSTANT(plTextureChannelMode::Blue)->AddAttributes(new plGroupAttribute("Single", 2.0f)),
  PLASMA_ENUM_CONSTANT(plTextureChannelMode::Alpha)->AddAttributes(new plGroupAttribute("Single", 3.0f))
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

plTextureAssetDocument::plTextureAssetDocument(const char* szDocumentPath)
  : plSimpleAssetDocument<plTextureAssetProperties>(szDocumentPath, plAssetDocEngineConnection::Simple)
{
  m_iTextureLod = -1;
}

static const char* ToWrapMode(plImageAddressMode::Enum mode)
{
  switch (mode)
  {
    case plImageAddressMode::Repeat:
      return "Repeat";
    case plImageAddressMode::Clamp:
      return "Clamp";
    case plImageAddressMode::ClampBorder:
      return "ClampBorder";
    case plImageAddressMode::Mirror:
      return "Mirror";
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      return "";
  }
}

const char* ToFilterMode(plTextureFilterSetting::Enum mode)
{
  switch (mode)
  {
    case plTextureFilterSetting::FixedNearest:
      return "Nearest";
    case plTextureFilterSetting::FixedBilinear:
      return "Bilinear";
    case plTextureFilterSetting::FixedTrilinear:
      return "Trilinear";
    case plTextureFilterSetting::FixedAnisotropic2x:
      return "Aniso2x";
    case plTextureFilterSetting::FixedAnisotropic4x:
      return "Aniso4x";
    case plTextureFilterSetting::FixedAnisotropic8x:
      return "Aniso8x";
    case plTextureFilterSetting::FixedAnisotropic16x:
      return "Aniso16x";
    case plTextureFilterSetting::LowestQuality:
      return "Lowest";
    case plTextureFilterSetting::LowQuality:
      return "Low";
    case plTextureFilterSetting::DefaultQuality:
      return "Default";
    case plTextureFilterSetting::HighQuality:
      return "High";
    case plTextureFilterSetting::HighestQuality:
      return "Highest";
  }

  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return "";
}

const char* ToUsageMode(plTexConvUsage::Enum mode)
{
  switch (mode)
  {
    case plTexConvUsage::Auto:
      return "Auto";
    case plTexConvUsage::Color:
      return "Color";
    case plTexConvUsage::Linear:
      return "Linear";
    case plTexConvUsage::Hdr:
      return "Hdr";
    case plTexConvUsage::NormalMap:
      return "NormalMap";
    case plTexConvUsage::NormalMap_Inverted:
      return "NormalMap_Inverted";
    case plTexConvUsage::BumpMap:
      return "BumpMap";
  }

  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return "";
}

const char* ToMipmapMode(plTexConvMipmapMode::Enum mode)
{
  switch (mode)
  {
    case plTexConvMipmapMode::None:
      return "None";
    case plTexConvMipmapMode::Linear:
      return "Linear";
    case plTexConvMipmapMode::Kaiser:
      return "Kaiser";
  }

  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return "";
}

const char* ToCompressionMode(plTexConvCompressionMode::Enum mode)
{
  switch (mode)
  {
    case plTexConvCompressionMode::None:
      return "None";
    case plTexConvCompressionMode::Medium:
      return "Medium";
    case plTexConvCompressionMode::High:
      return "High";
  }

  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return "";
}

plStatus plTextureAssetDocument::RunTexConv(const char* szTargetFile, const plAssetFileHeader& AssetHeader, bool bUpdateThumbnail, const plTextureAssetProfileConfig* pAssetConfig)
{
  const plTextureAssetProperties* pProp = GetProperties();

  QStringList arguments;
  plStringBuilder temp;

  // Asset Version
  {
    arguments << "-assetVersion";
    arguments << plConversionUtils::ToString(AssetHeader.GetFileVersion(), temp).GetData();
  }

  // Asset Hash
  {
    const plUInt64 uiHash64 = AssetHeader.GetFileHash();
    const plUInt32 uiHashLow32 = uiHash64 & 0xFFFFFFFF;
    const plUInt32 uiHashHigh32 = (uiHash64 >> 32) & 0xFFFFFFFF;

    temp.Format("{0}", plArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.Format("{0}", plArgU(uiHashHigh32, 8, true, 16, true));
    arguments << "-assetHashHigh";
    arguments << temp.GetData();
  }


  arguments << "-out";
  arguments << szTargetFile;

  const plStringBuilder sThumbnail = GetThumbnailFilePath();
  if (bUpdateThumbnail)
  {
    // Thumbnail
    const plStringBuilder sDir = sThumbnail.GetFileDirectory();
    plOSFile::CreateDirectoryStructure(sDir).IgnoreResult();

    arguments << "-thumbnailRes";
    arguments << "256";
    arguments << "-thumbnailOut";

    arguments << QString::fromUtf8(sThumbnail.GetData());
  }

  // low resolution data
  {
    plStringBuilder lowResPath = szTargetFile;
    plStringBuilder name = lowResPath.GetFileName();
    name.Append("-lowres");
    lowResPath.ChangeFileName(name);

    arguments << "-lowMips";
    arguments << "6";
    arguments << "-lowOut";

    arguments << QString::fromUtf8(lowResPath.GetData());
  }

  arguments << "-mipmaps";
  arguments << ToMipmapMode(pProp->m_MipmapMode);

  arguments << "-compression";
  arguments << ToCompressionMode(pProp->m_CompressionMode);

  arguments << "-usage";
  arguments << ToUsageMode(pProp->m_TextureUsage);

  if (pProp->m_bPremultipliedAlpha)
    arguments << "-premulalpha";

  if (pProp->m_bDilateColor)
  {
    arguments << "-dilate";
    // arguments << "8"; // default value
  }

  if (pProp->m_bFlipHorizontal)
    arguments << "-flip_horz";

  if (pProp->m_bPreserveAlphaCoverage)
  {
    arguments << "-mipsPreserveCoverage";
    arguments << "-mipsAlphaThreshold";
    temp.Format("{0}", plArgF(pProp->m_fAlphaThreshold, 2));
    arguments << temp.GetData();
  }

  if (pProp->m_TextureUsage == plTexConvUsage::Hdr)
  {
    arguments << "-hdrExposure";
    temp.Format("{0}", plArgF(pProp->m_fHdrExposureBias, 2));
    arguments << temp.GetData();
  }

  arguments << "-maxRes" << QString::number(pAssetConfig->m_uiMaxResolution);

  arguments << "-addressU" << ToWrapMode(pProp->m_AddressModeU);
  arguments << "-addressV" << ToWrapMode(pProp->m_AddressModeV);
  arguments << "-addressW" << ToWrapMode(pProp->m_AddressModeW);
  arguments << "-filter" << ToFilterMode(pProp->m_TextureFilter);

  const plInt32 iNumInputFiles = pProp->GetNumInputFiles();
  for (plInt32 i = 0; i < iNumInputFiles; ++i)
  {
    temp.Format("-in{0}", i);

    if (plStringUtils::IsNullOrEmpty(pProp->GetInputFile(i)))
      break;

    arguments << temp.GetData();
    arguments << QString(pProp->GetAbsoluteInputFilePath(i).GetData());
  }

  switch (pProp->GetChannelMapping())
  {
    case plTexture2DChannelMappingEnum::R1:
    {
      arguments << "-r";
      arguments << "in0.r"; // always linear
    }
    break;

    case plTexture2DChannelMappingEnum::RG1:
    {
      arguments << "-rg";
      arguments << "in0.rg"; // always linear
    }
    break;

    case plTexture2DChannelMappingEnum::R1_G2:
    {
      arguments << "-r";
      arguments << "in0.r";
      arguments << "-g";
      arguments << "in1.g"; // always linear
    }
    break;

    case plTexture2DChannelMappingEnum::RGB1:
    {
      arguments << "-rgb";
      arguments << "in0.rgb";
    }
    break;

    case plTexture2DChannelMappingEnum::RGB1_ABLACK:
    {
      arguments << "-rgb";
      arguments << "in0.rgb";
      arguments << "-a";
      arguments << "black";
    }
    break;

    case plTexture2DChannelMappingEnum::R1_G2_B3:
    {
      arguments << "-r";
      arguments << "in0.r";
      arguments << "-g";
      arguments << "in1.r";
      arguments << "-b";
      arguments << "in2.r";
    }
    break;

    case plTexture2DChannelMappingEnum::RGBA1:
    {
      arguments << "-rgba";
      arguments << "in0.rgba";
    }
    break;

    case plTexture2DChannelMappingEnum::RGB1_A2:
    {
      arguments << "-rgb";
      arguments << "in0.rgb";
      arguments << "-a";
      arguments << "in1.r";
    }
    break;

    case plTexture2DChannelMappingEnum::R1_G2_B3_A4:
    {
      arguments << "-r";
      arguments << "in0.r";
      arguments << "-g";
      arguments << "in1.r";
      arguments << "-b";
      arguments << "in2.r";
      arguments << "-a";
      arguments << "in3.r";
    }
    break;
  }

  PLASMA_SUCCEED_OR_RETURN(plQtEditorApp::GetSingleton()->ExecuteTool("TexConv", arguments, 180, plLog::GetThreadLocalLogSystem()));

  if (bUpdateThumbnail)
  {
    plUInt64 uiThumbnailHash = plAssetCurator::GetSingleton()->GetAssetReferenceHash(GetGuid());
    PLASMA_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, thumbnailInfo);
    InvalidateAssetThumbnail();
  }

  return plStatus(PLASMA_SUCCESS);
}


void plTextureAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  for (plUInt32 i = GetProperties()->GetNumInputFiles(); i < 4; ++i)
  {
    // remove unused dependencies
    pInfo->m_AssetTransformDependencies.Remove(GetProperties()->GetInputFile(i));
  }
}

void plTextureAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  if (m_bIsRenderTarget)
  {
    if (GetProperties()->m_bIsRenderTarget == false)
    {
      GetCommandHistory()->StartTransaction("MakeRenderTarget");
      GetObjectAccessor()->SetValue(GetPropertyObject(), "IsRenderTarget", true).IgnoreResult();
      GetCommandHistory()->FinishTransaction();
      GetCommandHistory()->ClearUndoHistory();
    }
  }
}

plTransformStatus plTextureAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  // PLASMA_ASSERT_DEV(plStringUtils::IsEqual(szPlatform, "PC"), "Platform '{0}' is not supported", szPlatform);

  const auto* pAssetConfig = pAssetProfile->GetTypeConfig<plTextureAssetProfileConfig>();

  const auto props = GetProperties();

  if (m_bIsRenderTarget)
  {
    plDeferredFileWriter file;
    file.SetOutput(szTargetFile);

    PLASMA_SUCCEED_OR_RETURN(AssetHeader.Write(file));

    // TODO: move this into a shared location, reuse in plTexConv::WriteTexHeader
    const plUInt8 uiTexFileFormatVersion = 5;
    file << uiTexFileFormatVersion;

    plGALResourceFormat::Enum format = plGALResourceFormat::Invalid;
    bool bIsSRGB = false;

    switch (props->m_RtFormat)
    {
      case plRenderTargetFormat::RGBA8:
        format = plGALResourceFormat::RGBAUByteNormalized;
        break;

      case plRenderTargetFormat::RGBA8sRgb:
        format = plGALResourceFormat::RGBAUByteNormalizedsRGB;
        bIsSRGB = true;
        break;

      case plRenderTargetFormat::RGB10:
        format = plGALResourceFormat::RG11B10Float;
        break;

      case plRenderTargetFormat::RGBA16:
        format = plGALResourceFormat::RGBAHalf;
        break;
    }

    file << bIsSRGB;
    file << (plUInt8)props->m_AddressModeU;
    file << (plUInt8)props->m_AddressModeV;
    file << (plUInt8)props->m_AddressModeW;
    file << (plUInt8)props->m_TextureFilter;

    plInt16 resX = 0, resY = 0;

    switch (props->m_Resolution)
    {
      case plTexture2DResolution::Fixed64x64:
        resX = 64;
        resY = 64;
        break;
      case plTexture2DResolution::Fixed128x128:
        resX = 128;
        resY = 128;
        break;
      case plTexture2DResolution::Fixed256x256:
        resX = 256;
        resY = 256;
        break;
      case plTexture2DResolution::Fixed512x512:
        resX = 512;
        resY = 512;
        break;
      case plTexture2DResolution::Fixed1024x1024:
        resX = 1024;
        resY = 1024;
        break;
      case plTexture2DResolution::Fixed2048x2048:
        resX = 2048;
        resY = 2048;
        break;
      case plTexture2DResolution::CVarRtResolution1:
        resX = -1;
        resY = 1;
        break;
      case plTexture2DResolution::CVarRtResolution2:
        resX = -1;
        resY = 2;
        break;
      default:
        PLASMA_ASSERT_NOT_IMPLEMENTED;
    }

    file << resX;
    file << resY;
    file << props->m_fCVarResolutionScale;
    file << (int)format;


    if (file.Close().Failed())
      return plTransformStatus(plFmt("Writing to target file failed: '{0}'", szTargetFile));

    return plTransformStatus();
  }
  else
  {
    const bool bUpdateThumbnail = pAssetProfile == plAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();

    plTransformStatus result = RunTexConv(szTargetFile, AssetHeader, bUpdateThumbnail, pAssetConfig);

    plFileStats stat;
    if (plOSFile::GetFileStats(szTargetFile, stat).Succeeded() && stat.m_uiFileSize == 0)
    {
      // if the file was touched, but nothing written to it, delete the file
      // might happen if TexConv crashed or had an error
      plOSFile::DeleteFile(szTargetFile).IgnoreResult();

      if (result.Succeeded())
        result = plTransformStatus("TexConv did not write an output file");
    }

    return result;
  }
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plTextureAssetDocumentGenerator>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

enum class TextureType
{
  Diffuse,
  Normal,
  Occlusion,
  Roughness,
  Metalness,
  ORM,
  Height,
  HDR,
  Linear,
};

plTextureAssetDocumentGenerator::plTextureAssetDocumentGenerator()
{
  AddSupportedFileType("tga");
  AddSupportedFileType("dds");
  AddSupportedFileType("jpg");
  AddSupportedFileType("jpeg");
  AddSupportedFileType("hdr");
  AddSupportedFileType("png");
}

plTextureAssetDocumentGenerator::~plTextureAssetDocumentGenerator() = default;

void plTextureAssetDocumentGenerator::GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  plStringBuilder baseOutputFile = sParentDirRelativePath;

  plStringBuilder baseFilename = baseOutputFile.GetFileName();

  while (baseFilename.TrimWordEnd("_") ||
         baseFilename.TrimWordEnd("K") ||
         baseFilename.TrimWordEnd("-") ||
         baseFilename.TrimWordEnd("1") ||
         baseFilename.TrimWordEnd("2") ||
         baseFilename.TrimWordEnd("3") ||
         baseFilename.TrimWordEnd("4") ||
         baseFilename.TrimWordEnd("5") ||
         baseFilename.TrimWordEnd("6") ||
         baseFilename.TrimWordEnd("7") ||
         baseFilename.TrimWordEnd("8") ||
         baseFilename.TrimWordEnd("9") ||
         baseFilename.TrimWordEnd("0") ||
         baseFilename.TrimWordEnd("gl"))
  {
  }

  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  TextureType tt = TextureType::Diffuse;

  if (plPathUtils::HasExtension(sParentDirRelativePath, "hdr"))
  {
    tt = TextureType::HDR;
  }
  else if (baseFilename.EndsWith_NoCase("_d") || baseFilename.EndsWith_NoCase("diffuse") || baseFilename.EndsWith_NoCase("diff") || baseFilename.EndsWith_NoCase("col") || baseFilename.EndsWith_NoCase("color"))
  {
    tt = TextureType::Diffuse;
  }
  else if (baseFilename.EndsWith_NoCase("_n") || baseFilename.EndsWith_NoCase("normal") || baseFilename.EndsWith_NoCase("normals") || baseFilename.EndsWith_NoCase("nrm") || baseFilename.EndsWith_NoCase("norm") || baseFilename.EndsWith_NoCase("_nor"))
  {
    tt = TextureType::Normal;
  }
  else if (baseFilename.EndsWith_NoCase("_arm") || baseFilename.EndsWith_NoCase("_orm"))
  {
    tt = TextureType::ORM;
  }
  else if (baseFilename.EndsWith_NoCase("_rough") || baseFilename.EndsWith_NoCase("roughness") || baseFilename.EndsWith_NoCase("_rgh"))
  {
    tt = TextureType::Roughness;
  }
  else if (baseFilename.EndsWith_NoCase("_ao"))
  {
    tt = TextureType::Occlusion;
  }
  else if (baseFilename.EndsWith_NoCase("_height") || baseFilename.EndsWith_NoCase("_disp"))
  {
    tt = TextureType::Height;
  }
  else if (baseFilename.EndsWith_NoCase("_metal") || baseFilename.EndsWith_NoCase("_met") || baseFilename.EndsWith_NoCase("metallic") || baseFilename.EndsWith_NoCase("metalness"))
  {
    tt = TextureType::Metalness;
  }
  else if (baseFilename.EndsWith_NoCase("_alpha"))
  {
    tt = TextureType::Linear;
  }

  plAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
  info.m_Priority = plAssetDocGeneratorPriority::DefaultPriority;
  info.m_sOutputFileParentRelative = baseOutputFile;

  // first add the default option
  switch (tt)
  {
    case TextureType::Diffuse:
    {
      info.m_sName = "TextureImport.Diffuse";
      info.m_sIcon = ":/AssetIcons/Texture_2D.svg";
      break;
    }

    case TextureType::Normal:
    {
      info.m_sName = "TextureImport.Normal";
      info.m_sIcon = ":/AssetIcons/Texture_Normals.svg";
      break;
    }

    case TextureType::Roughness:
    {
      info.m_sName = "TextureImport.Roughness";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::Occlusion:
    {
      info.m_sName = "TextureImport.Occlusion";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::Metalness:
    {
      info.m_sName = "TextureImport.Metalness";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::ORM:
    {
      info.m_sName = "TextureImport.ORM";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::Height:
    {
      info.m_sName = "TextureImport.Height";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }

    case TextureType::HDR:
    {
      info.m_sName = "TextureImport.HDR";
      info.m_sIcon = ":/AssetIcons/Texture_2D.svg";
      break;
    }

    case TextureType::Linear:
    {
      info.m_sName = "TextureImport.Linear";
      info.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
      break;
    }
  }

  // now add all the other options

  if (tt != TextureType::Diffuse)
  {
    plAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority = plAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative = baseOutputFile;
    info2.m_sName = "TextureImport.Diffuse";
    info2.m_sIcon = ":/AssetIcons/Texture_2D.svg";
  }

  if (tt != TextureType::Linear)
  {
    plAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority = plAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative = baseOutputFile;
    info2.m_sName = "TextureImport.Linear";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::Normal)
  {
    plAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority = plAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative = baseOutputFile;
    info2.m_sName = "TextureImport.Normal";
    info2.m_sIcon = ":/AssetIcons/Texture_Normals.svg";
  }

  if (tt != TextureType::Metalness)
  {
    plAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority = plAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative = baseOutputFile;
    info2.m_sName = "TextureImport.Metalness";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::Roughness)
  {
    plAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority = plAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative = baseOutputFile;
    info2.m_sName = "TextureImport.Roughness";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::Occlusion)
  {
    plAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority = plAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative = baseOutputFile;
    info2.m_sName = "TextureImport.Occlusion";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::ORM)
  {
    plAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority = plAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative = baseOutputFile;
    info2.m_sName = "TextureImport.ORM";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }

  if (tt != TextureType::Height)
  {
    plAssetDocumentGenerator::Info& info2 = out_Modes.ExpandAndGetRef();
    info2.m_Priority = plAssetDocGeneratorPriority::LowPriority;
    info2.m_sOutputFileParentRelative = baseOutputFile;
    info2.m_sName = "TextureImport.Height";
    info2.m_sIcon = ":/AssetIcons/Texture_Linear.svg";
  }
}

plStatus plTextureAssetDocumentGenerator::Generate(plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument)
{
  auto pApp = plQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, plDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return plStatus("Could not create target document");

  plTextureAssetDocument* pAssetDoc = plDynamicCast<plTextureAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return plStatus("Target document is not a valid plTextureAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input1", sDataDirRelativePath);
  accessor.SetValue("ChannelMapping", (int)plTexture2DChannelMappingEnum::RGB1);
  accessor.SetValue("Usage", (int)plTexConvUsage::Linear);

  if (info.m_sName == "TextureImport.Diffuse")
  {
    accessor.SetValue("Usage", (int)plTexConvUsage::Color);
  }
  else if (info.m_sName == "TextureImport.Normal")
  {
    accessor.SetValue("Usage", (int)plTexConvUsage::NormalMap);
  }
  else if (info.m_sName == "TextureImport.HDR")
  {
    accessor.SetValue("Usage", (int)plTexConvUsage::Hdr);
  }
  else if (info.m_sName == "TextureImport.Linear")
  {
  }
  else if (info.m_sName == "TextureImport.Occlusion")
  {
    accessor.SetValue("ChannelMapping", (int)plTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)plTextureFilterSetting::LowestQuality);
  }
  else if (info.m_sName == "TextureImport.Height")
  {
    accessor.SetValue("ChannelMapping", (int)plTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)plTextureFilterSetting::LowQuality);
  }
  else if (info.m_sName == "TextureImport.Roughness")
  {
    accessor.SetValue("ChannelMapping", (int)plTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)plTextureFilterSetting::LowQuality);
  }
  else if (info.m_sName == "TextureImport.Metalness")
  {
    accessor.SetValue("ChannelMapping", (int)plTexture2DChannelMappingEnum::R1);
    accessor.SetValue("TextureFilter", (int)plTextureFilterSetting::LowQuality);
  }
  else if (info.m_sName == "TextureImport.ORM")
  {
    accessor.SetValue("ChannelMapping", (int)plTexture2DChannelMappingEnum::RGB1);
    accessor.SetValue("TextureFilter", (int)plTextureFilterSetting::LowQuality);
  }

  return plStatus(PLASMA_SUCCESS);
}
