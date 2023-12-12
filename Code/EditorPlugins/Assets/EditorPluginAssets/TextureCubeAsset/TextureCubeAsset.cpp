#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureCubeAssetDocument, 3, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plTextureCubeChannelMode, 1)
  PLASMA_ENUM_CONSTANTS(plTextureCubeChannelMode::RGB, plTextureCubeChannelMode::Red, plTextureCubeChannelMode::Green, plTextureCubeChannelMode::Blue, plTextureCubeChannelMode::Alpha)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

const char* ToFilterMode(plTextureFilterSetting::Enum mode);
const char* ToUsageMode(plTexConvUsage::Enum mode);
const char* ToCompressionMode(plTexConvCompressionMode::Enum mode);
const char* ToMipmapMode(plTexConvMipmapMode::Enum mode);

plTextureCubeAssetDocument::plTextureCubeAssetDocument(const char* szDocumentPath)
  : plSimpleAssetDocument<plTextureCubeAssetProperties>(szDocumentPath, plAssetDocEngineConnection::Simple)
{
  m_iTextureLod = -1;
}

plStatus plTextureCubeAssetDocument::RunTexConv(const char* szTargetFile, const plAssetFileHeader& AssetHeader, bool bUpdateThumbnail)
{
  const plTextureCubeAssetProperties* pProp = GetProperties();

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

  if (pProp->m_TextureUsage == plTexConvUsage::Hdr)
  {
    arguments << "-hdrExposure";
    temp.Format("{0}", plArgF(pProp->m_fHdrExposureBias, 2));
    arguments << temp.GetData();
  }

  // TODO: downscale steps and min/max resolution

  arguments << "-mipmaps";
  arguments << ToMipmapMode(pProp->m_MipmapMode);

  arguments << "-compression";
  arguments << ToCompressionMode(pProp->m_CompressionMode);

  arguments << "-usage";
  arguments << ToUsageMode(pProp->m_TextureUsage);

  arguments << "-filter" << ToFilterMode(pProp->m_TextureFilter);

  arguments << "-type";
  arguments << "Cubemap";

  switch (pProp->m_ChannelMapping)
  {
    case plTextureCubeChannelMappingEnum::RGB1:
      arguments << "-rgb"
                << "in0";
      break;

    case plTextureCubeChannelMappingEnum::RGB1TO6:
      arguments << "-rgb0"
                << "in0";
      arguments << "-rgb1"
                << "in1";
      arguments << "-rgb2"
                << "in2";
      arguments << "-rgb3"
                << "in3";
      arguments << "-rgb4"
                << "in4";
      arguments << "-rgb5"
                << "in5";
      break;


    case plTextureCubeChannelMappingEnum::RGBA1:
      arguments << "-rgba"
                << "in0";
      break;

    case plTextureCubeChannelMappingEnum::RGBA1TO6:
      arguments << "-rgba0"
                << "in0";
      arguments << "-rgba1"
                << "in1";
      arguments << "-rgba2"
                << "in2";
      arguments << "-rgba3"
                << "in3";
      arguments << "-rgba4"
                << "in4";
      arguments << "-rgba5"
                << "in5";
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  const plInt32 iNumInputFiles = pProp->GetNumInputFiles();
  for (plInt32 i = 0; i < iNumInputFiles; ++i)
  {
    if (plStringUtils::IsNullOrEmpty(pProp->GetInputFile(i)))
      break;

    temp.Format("-in{0}", i);
    arguments << temp.GetData();
    arguments << QString(pProp->GetAbsoluteInputFilePath(i).GetData());
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

void plTextureCubeAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  switch (GetProperties()->m_ChannelMapping)
  {
    case plTextureCubeChannelMappingEnum::RGB1:
    case plTextureCubeChannelMappingEnum::RGBA1:
    {
      // remove file dependencies, that aren't used
      pInfo->m_AssetTransformDependencies.Remove(GetProperties()->GetInputFile1());
      pInfo->m_AssetTransformDependencies.Remove(GetProperties()->GetInputFile2());
      pInfo->m_AssetTransformDependencies.Remove(GetProperties()->GetInputFile3());
      pInfo->m_AssetTransformDependencies.Remove(GetProperties()->GetInputFile4());
      pInfo->m_AssetTransformDependencies.Remove(GetProperties()->GetInputFile5());
      break;
    }

    case plTextureCubeChannelMappingEnum::RGB1TO6:
    case plTextureCubeChannelMappingEnum::RGBA1TO6:
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }
}

plTransformStatus plTextureCubeAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  // PLASMA_ASSERT_DEV(plStringUtils::IsEqual(szPlatform, "PC"), "Platform '{0}' is not supported", szPlatform);
  const bool bUpdateThumbnail = pAssetProfile == plAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();

  plTransformStatus result = RunTexConv(szTargetFile, AssetHeader, bUpdateThumbnail);

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

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureCubeAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plTextureCubeAssetDocumentGenerator>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plTextureCubeAssetDocumentGenerator::plTextureCubeAssetDocumentGenerator()
{
  AddSupportedFileType("dds");
  AddSupportedFileType("hdr");

  // these formats would need to use 6 files for the faces
  // more elaborate detection and mapping would need to be implemented
  // AddSupportedFileType("tga");
  // AddSupportedFileType("jpg");
  // AddSupportedFileType("jpeg");
  // AddSupportedFileType("png");
}

plTextureCubeAssetDocumentGenerator::~plTextureCubeAssetDocumentGenerator() {}

void plTextureCubeAssetDocumentGenerator::GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  plStringBuilder baseOutputFile = sParentDirRelativePath;

  const plStringBuilder baseFilename = baseOutputFile.GetFileName();
  const bool isHDR = plPathUtils::HasExtension(sParentDirRelativePath, "hdr");

  /// \todo Make this configurable
  const bool isCubemap = ((baseFilename.FindSubString_NoCase("cubemap") != nullptr) || (baseFilename.FindSubString_NoCase("skybox") != nullptr));

  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  if (isHDR)
  {
    {
      plAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
      info.m_Priority = isCubemap ? plAssetDocGeneratorPriority::HighPriority : plAssetDocGeneratorPriority::Undecided;
      info.m_sName = "CubemapImport.SkyboxHDR";
      info.m_sOutputFileParentRelative = baseOutputFile;
      info.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
    }
  }
  else
  {
    {
      plAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
      info.m_Priority = isCubemap ? plAssetDocGeneratorPriority::HighPriority : plAssetDocGeneratorPriority::Undecided;
      info.m_sName = "CubemapImport.Skybox";
      info.m_sOutputFileParentRelative = baseOutputFile;
      info.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
    }
  }
}

plStatus plTextureCubeAssetDocumentGenerator::Generate(plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument)
{
  auto pApp = plQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, plDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return plStatus("Could not create target document");

  plTextureCubeAssetDocument* pAssetDoc = plDynamicCast<plTextureCubeAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return plStatus("Target document is not a valid plTextureCubeAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input1", sDataDirRelativePath);
  accessor.SetValue("ChannelMapping", (int)plTextureCubeChannelMappingEnum::RGB1);

  if (info.m_sName == "CubemapImport.SkyboxHDR")
  {
    accessor.SetValue("Usage", (int)plTexConvUsage::Hdr);
  }
  else if (info.m_sName == "CubemapImport.Skybox")
  {
    accessor.SetValue("Usage", (int)plTexConvUsage::Color);
  }

  return plStatus(PLASMA_SUCCESS);
}
