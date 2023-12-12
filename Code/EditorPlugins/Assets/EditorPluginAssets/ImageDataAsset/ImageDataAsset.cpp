#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAsset.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plImageDataAssetDocument, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plImageDataAssetDocument::plImageDataAssetDocument(const char* szDocumentPath)
  : plSimpleAssetDocument<plImageDataAssetProperties>(szDocumentPath, plAssetDocEngineConnection::None)
{
}

plTransformStatus plImageDataAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  const bool bUpdateThumbnail = pAssetProfile == plAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();

  plStatus result = RunTexConv(szTargetFile, AssetHeader, bUpdateThumbnail);

  plFileStats stat;
  if (plOSFile::GetFileStats(szTargetFile, stat).Succeeded() && stat.m_uiFileSize == 0)
  {
    // if the file was touched, but nothing written to it, delete the file
    // might happen if TexConv crashed or had an error
    plOSFile::DeleteFile(szTargetFile).IgnoreResult();
    result.m_Result = PLASMA_FAILURE;
  }

  if (result.Succeeded())
  {
    plImageDataAssetEvent e;
    e.m_Type = plImageDataAssetEvent::Type::Transformed;
    m_Events.Broadcast(e);
  }

  return result;
}

plStatus plImageDataAssetDocument::RunTexConv(const char* szTargetFile, const plAssetFileHeader& AssetHeader, bool bUpdateThumbnail)
{
  const plImageDataAssetProperties* pProp = GetProperties();

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

  arguments << "-mipmaps";
  arguments << "None";

  arguments << "-type";
  arguments << "2D";

  arguments << "-compression";
  arguments << "None";

  arguments << "-usage";
  arguments << "Linear";

  // arguments << "-maxRes" << QString::number(pAssetConfig->m_uiMaxResolution);


  {
    arguments << "-in0";

    plStringBuilder sPath = pProp->m_sInputFile;
    sPath.MakeCleanPath();

    if (!sPath.IsAbsolutePath())
    {
      plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
    }

    arguments << QString(sPath.GetData());
  }

  arguments << "-rgba";
  arguments << "in0.rgba";

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
