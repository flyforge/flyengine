#include <EditorFramework/EditorFrameworkPCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Serialization/DdlSerializer.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAssetDocumentManager, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plAssetDocumentManager::plAssetDocumentManager() = default;
plAssetDocumentManager::~plAssetDocumentManager() = default;

plStatus plAssetDocumentManager::CloneDocument(plStringView sPath, plStringView sClonePath, plUuid& inout_cloneGuid)
{
  plStatus res = SUPER::CloneDocument(sPath, sClonePath, inout_cloneGuid);
  if (res.Succeeded())
  {
    // Cloned documents are usually opened right after cloning. To make sure this does not fail we need to inform the asset curator of the newly added asset document.
    plAssetCurator::GetSingleton()->NotifyOfFileChange(sClonePath);
  }
  return res;
}

void plAssetDocumentManager::ComputeAssetProfileHash(const plPlatformProfile* pAssetProfile)
{
  m_uiAssetProfileHash = ComputeAssetProfileHashImpl(DetermineFinalTargetProfile(pAssetProfile));

  if (GeneratesProfileSpecificAssets())
  {
    PL_ASSERT_DEBUG(m_uiAssetProfileHash != 0, "Assets that generate a profile-specific output must compute a hash for the profile settings.");
  }
  else
  {
    PL_ASSERT_DEBUG(m_uiAssetProfileHash == 0, "Only assets that generate per-profile outputs may specify an asset profile hash.");
    m_uiAssetProfileHash = 0;
  }
}

plUInt64 plAssetDocumentManager::ComputeAssetProfileHashImpl(const plPlatformProfile* pAssetProfile) const
{
  return 0;
}

plStatus plAssetDocumentManager::ReadAssetDocumentInfo(plUniquePtr<plAssetDocumentInfo>& out_pInfo, plStreamReader& inout_stream) const
{
  plAbstractObjectGraph graph;

  if (plAbstractGraphDdlSerializer::ReadHeader(inout_stream, &graph).Failed())
    return plStatus("Failed to read asset document");

  plRttiConverterContext context;
  plRttiConverterReader rttiConverter(&graph, &context);

  auto* pHeaderNode = graph.GetNodeByName("Header");

  if (pHeaderNode == nullptr)
    return plStatus("Document does not contain a 'Header'");

  plAssetDocumentInfo* pEntry = rttiConverter.CreateObjectFromNode(pHeaderNode).Cast<plAssetDocumentInfo>();
  PL_ASSERT_DEBUG(pEntry != nullptr, "Failed to deserialize plAssetDocumentInfo!");
  out_pInfo = plUniquePtr<plAssetDocumentInfo>(pEntry, plFoundation::GetDefaultAllocator());
  return plStatus(PL_SUCCESS);
}

plString plAssetDocumentManager::GenerateResourceThumbnailPath(plStringView sDocumentPath, plStringView sSubAssetName)
{
  plStringBuilder sRelativePath;
  if (sSubAssetName.IsEmpty())
  {
    sRelativePath = sDocumentPath;
  }
  else
  {
    sRelativePath = sDocumentPath.GetFileDirectory();

    plStringBuilder sValidFileName;
    plPathUtils::MakeValidFilename(sSubAssetName, '_', sValidFileName);
    sRelativePath.AppendPath(sValidFileName);
  }

  plString sProjectDir = plAssetCurator::GetSingleton()->FindDataDirectoryForAsset(sRelativePath);

  sRelativePath.MakeRelativeTo(sProjectDir).IgnoreResult();
  sRelativePath.Append(".jpg");

  plStringBuilder sFinalPath(sProjectDir, "/AssetCache/Thumbnails/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

bool plAssetDocumentManager::IsThumbnailUpToDate(plStringView sDocumentPath, plStringView sSubAssetName, plUInt64 uiThumbnailHash, plUInt32 uiTypeVersion)
{
  CURATOR_PROFILE(szDocumentPath);
  plString sThumbPath = GenerateResourceThumbnailPath(sDocumentPath, sSubAssetName);
  plFileReader file;
  if (file.Open(sThumbPath, 256).Failed())
    return false;

  plAssetDocument::ThumbnailInfo thumbnailInfo;

  const plUInt64 uiHeaderSize = thumbnailInfo.GetSerializedSize();
  plUInt64 uiFileSize = file.GetFileSize();

  if (uiFileSize < uiHeaderSize)
    return false;

  file.SkipBytes(uiFileSize - uiHeaderSize);

  if (thumbnailInfo.Deserialize(file).Failed())
  {
    return false;
  }

  return thumbnailInfo.IsThumbnailUpToDate(uiThumbnailHash, uiTypeVersion);
}

void plAssetDocumentManager::AddEntriesToAssetTable(plStringView sDataDirectory, const plPlatformProfile* pAssetProfile, plDelegate<void(plStringView sGuid, plStringView sPath, plStringView sType)> addEntry) const {}

plString plAssetDocumentManager::GetAssetTableEntry(const plSubAsset* pSubAsset, plStringView sDataDirectory, const plPlatformProfile* pAssetProfile) const
{
  return GetRelativeOutputFileName(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor, sDataDirectory, pSubAsset->m_pAssetInfo->m_Path, "", pAssetProfile);
}

plString plAssetDocumentManager::GetAbsoluteOutputFileName(const plAssetDocumentTypeDescriptor* pTypeDesc, plStringView sDocumentPath, plStringView sOutputTag, const plPlatformProfile* pAssetProfile) const
{
  plStringBuilder sProjectDir = plAssetCurator::GetSingleton()->FindDataDirectoryForAsset(sDocumentPath);

  plString sRelativePath = GetRelativeOutputFileName(pTypeDesc, sProjectDir, sDocumentPath, sOutputTag, pAssetProfile);
  plStringBuilder sFinalPath(sProjectDir, "/AssetCache/", sRelativePath);
  sFinalPath.MakeCleanPath();

  return sFinalPath;
}

plString plAssetDocumentManager::GetRelativeOutputFileName(const plAssetDocumentTypeDescriptor* pTypeDesc, plStringView sDataDirectory, plStringView sDocumentPath, plStringView sOutputTag, const plPlatformProfile* pAssetProfile) const
{
  const plPlatformProfile* pPlatform = plAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile);
  PL_ASSERT_DEBUG(sOutputTag.IsEmpty(), "The output tag '{}' for '{}' is not supported, override GetRelativeOutputFileName", sOutputTag, sDocumentPath);

  plStringBuilder sRelativePath(sDocumentPath);
  sRelativePath.MakeRelativeTo(sDataDirectory).IgnoreResult();
  GenerateOutputFilename(sRelativePath, pPlatform, pTypeDesc->m_sResourceFileExtension, GeneratesProfileSpecificAssets());

  return sRelativePath;
}

bool plAssetDocumentManager::IsOutputUpToDate(plStringView sDocumentPath, const plDynamicArray<plString>& outputs, plUInt64 uiHash, const plAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  CURATOR_PROFILE(sDocumentPath);
  if (!IsOutputUpToDate(sDocumentPath, "", uiHash, pTypeDescriptor))
    return false;

  for (const plString& sOutput : outputs)
  {
    if (!IsOutputUpToDate(sDocumentPath, sOutput, uiHash, pTypeDescriptor))
      return false;
  }
  return true;
}

bool plAssetDocumentManager::IsOutputUpToDate(plStringView sDocumentPath, plStringView sOutputTag, plUInt64 uiHash, const plAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  const plString sTargetFile = GetAbsoluteOutputFileName(pTypeDescriptor, sDocumentPath, sOutputTag);
  return plAssetDocumentManager::IsResourceUpToDate(sTargetFile, uiHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion());
}

const plPlatformProfile* plAssetDocumentManager::DetermineFinalTargetProfile(const plPlatformProfile* pAssetProfile)
{
  if (pAssetProfile == nullptr)
  {
    return plAssetCurator::GetSingleton()->GetActiveAssetProfile();
  }

  return pAssetProfile;
}

plResult plAssetDocumentManager::TryOpenAssetDocument(const char* szPathOrGuid)
{
  plAssetCurator::plLockedSubAsset pSubAsset;

  if (plConversionUtils::IsStringUuid(szPathOrGuid))
  {
    plUuid matGuid;
    matGuid = plConversionUtils::ConvertStringToUuid(szPathOrGuid);

    pSubAsset = plAssetCurator::GetSingleton()->GetSubAsset(matGuid);
  }
  else
  {
    // I think this is even wrong, either the string is a GUID, or it is not an asset at all, in which case we cannot find it this way
    // either left as an exercise for whoever needs non-asset references
    pSubAsset = plAssetCurator::GetSingleton()->FindSubAsset(szPathOrGuid);
  }

  if (pSubAsset)
  {
    plQtEditorApp::GetSingleton()->OpenDocumentQueued(pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath());
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

bool plAssetDocumentManager::IsResourceUpToDate(const char* szResourceFile, plUInt64 uiHash, plUInt16 uiTypeVersion)
{
  CURATOR_PROFILE(szResourceFile);
  plFileReader file;
  if (file.Open(szResourceFile, 256).Failed())
    return false;

  // this might happen if writing to the file failed
  if (file.GetFileSize() == 0)
    return false;

  plAssetFileHeader AssetHeader;
  AssetHeader.Read(file).IgnoreResult();

  return AssetHeader.IsFileUpToDate(uiHash, uiTypeVersion);
}

void plAssetDocumentManager::GenerateOutputFilename(plStringBuilder& inout_sRelativeDocumentPath, const plPlatformProfile* pAssetProfile, const char* szExtension, bool bPlatformSpecific)
{
  inout_sRelativeDocumentPath.ChangeFileExtension(szExtension);
  inout_sRelativeDocumentPath.MakeCleanPath();

  if (bPlatformSpecific)
  {
    const plPlatformProfile* pPlatform = plAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile);
    inout_sRelativeDocumentPath.Prepend(pPlatform->GetConfigName(), "/");
  }
  else
    inout_sRelativeDocumentPath.Prepend("Common/");
}
