#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureCubeAssetDocument, 3, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_ENUM(plTextureCubeChannelMode, 1)
  PL_ENUM_CONSTANTS(plTextureCubeChannelMode::RGB, plTextureCubeChannelMode::Red, plTextureCubeChannelMode::Green, plTextureCubeChannelMode::Blue, plTextureCubeChannelMode::Alpha)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

const char* ToFilterMode(plTextureFilterSetting::Enum mode);
const char* ToUsageMode(plTexConvUsage::Enum mode);
const char* ToCompressionMode(plTexConvCompressionMode::Enum mode);
const char* ToMipmapMode(plTexConvMipmapMode::Enum mode);

plTextureCubeAssetDocument::plTextureCubeAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plTextureCubeAssetProperties>(sDocumentPath, plAssetDocEngineConnection::Simple)
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

    temp.SetFormat("{0}", plArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.SetFormat("{0}", plArgU(uiHashHigh32, 8, true, 16, true));
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
    temp.SetFormat("{0}", plArgF(pProp->m_fHdrExposureBias, 2));
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
      PL_ASSERT_NOT_IMPLEMENTED;
  }

  const plInt32 iNumInputFiles = pProp->GetNumInputFiles();
  for (plInt32 i = 0; i < iNumInputFiles; ++i)
  {
    if (plStringUtils::IsNullOrEmpty(pProp->GetInputFile(i)))
      break;

    temp.SetFormat("-in{0}", i);
    arguments << temp.GetData();
    arguments << QString(pProp->GetAbsoluteInputFilePath(i).GetData());
  }

  PL_SUCCEED_OR_RETURN(plQtEditorApp::GetSingleton()->ExecuteTool("TexConv", arguments, 180, plLog::GetThreadLocalLogSystem()));

  if (bUpdateThumbnail)
  {
    plUInt64 uiThumbnailHash = plAssetCurator::GetSingleton()->GetAssetReferenceHash(GetGuid());
    PL_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, thumbnailInfo);
    InvalidateAssetThumbnail();
  }

  return plStatus(PL_SUCCESS);
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
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile1());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile2());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile3());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile4());
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetInputFile5());
      break;
    }

    case plTextureCubeChannelMappingEnum::RGB1TO6:
    case plTextureCubeChannelMappingEnum::RGBA1TO6:
      break;

    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }
}

plTransformStatus plTextureCubeAssetDocument::InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
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

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureCubeAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plTextureCubeAssetDocumentGenerator>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plTextureCubeAssetDocumentGenerator::plTextureCubeAssetDocumentGenerator()
{
  AddSupportedFileType("dds");
  AddSupportedFileType("hdr");
  AddSupportedFileType("exr");

  // these formats would need to use 6 files for the faces
  // more elaborate detection and mapping would need to be implemented
  // AddSupportedFileType("tga");
  // AddSupportedFileType("jpg");
  // AddSupportedFileType("jpeg");
  // AddSupportedFileType("png");
}

plTextureCubeAssetDocumentGenerator::~plTextureCubeAssetDocumentGenerator() = default;

void plTextureCubeAssetDocumentGenerator::GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const
{
  const plStringBuilder baseFilename = sAbsInputFile.GetFileName();
  const bool isHDR = sAbsInputFile.HasExtension("hdr") || sAbsInputFile.HasExtension("exr");

  const bool isCubemap = ((baseFilename.FindSubString_NoCase("cubemap") != nullptr) || (baseFilename.FindSubString_NoCase("skybox") != nullptr));

  // TODO: if (sAbsInputFile.IsEmpty()) -> CubemapImport.SkyboxAuto

  if (isHDR)
  {
    {
      plAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
      info.m_Priority = isCubemap ? plAssetDocGeneratorPriority::HighPriority : plAssetDocGeneratorPriority::Undecided;
      info.m_sName = "CubemapImport.SkyboxHDR";
      info.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
    }
  }
  else
  {
    {
      plAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
      info.m_Priority = isCubemap ? plAssetDocGeneratorPriority::HighPriority : plAssetDocGeneratorPriority::Undecided;
      info.m_sName = "CubemapImport.Skybox";
      info.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
    }
  }
}

plStatus plTextureCubeAssetDocumentGenerator::Generate(plStringView sInputFileAbs, plStringView sMode, plDynamicArray<plDocument*>& out_generatedDocuments)
{
  plStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  plOSFile::FindFreeFilename(sOutFile);

  auto pApp = plQtEditorApp::GetSingleton();

  plStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  plDocument* pDoc = pApp->CreateDocument(sOutFile, plDocumentFlags::None);
  if (pDoc == nullptr)
    return plStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  plTextureCubeAssetDocument* pAssetDoc = plDynamicCast<plTextureCubeAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input1", sInputFileRel.GetView());
  accessor.SetValue("ChannelMapping", (int)plTextureCubeChannelMappingEnum::RGB1);

  if (sMode == "CubemapImport.SkyboxHDR")
  {
    accessor.SetValue("Usage", (int)plTexConvUsage::Hdr);
  }
  else if (sMode == "CubemapImport.Skybox")
  {
    accessor.SetValue("Usage", (int)plTexConvUsage::Color);
  }

  return plStatus(PL_SUCCESS);
}
