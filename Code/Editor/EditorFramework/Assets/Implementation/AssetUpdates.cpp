#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

////////////////////////////////////////////////////////////////////////
// plAssetCurator Asset Hashing and Status Updates
////////////////////////////////////////////////////////////////////////

plAssetInfo::TransformState plAssetCurator::HashAsset(plUInt64 uiSettingsHash, const plHybridArray<plString, 16>& assetTransformDependencies, const plHybridArray<plString, 16>& runtimeDependencies, plSet<plString>& missingDependencies, plSet<plString>& missingReferences, plUInt64& out_AssetHash, plUInt64& out_ThumbHash, bool bForce)
{
  CURATOR_PROFILE("HashAsset");
  plStringBuilder tmp;
  plAssetInfo::TransformState state = plAssetInfo::Unknown;
  {
    // hash of the main asset file
    out_AssetHash = uiSettingsHash;
    out_ThumbHash = uiSettingsHash;

    // Iterate dependencies
    for (const auto& dep : assetTransformDependencies)
    {
      plString sPath = dep;
      if (!AddAssetHash(sPath, false, out_AssetHash, out_ThumbHash, bForce))
      {
        missingDependencies.Insert(sPath);
      }
    }

    for (const auto& dep : runtimeDependencies)
    {
      plString sPath = dep;
      if (!AddAssetHash(sPath, true, out_AssetHash, out_ThumbHash, bForce))
      {
        missingReferences.Insert(sPath);
      }
    }
  }

  if (!missingReferences.IsEmpty())
  {
    out_ThumbHash = 0;
    state = plAssetInfo::MissingReference;
  }
  if (!missingDependencies.IsEmpty())
  {
    out_AssetHash = 0;
    out_ThumbHash = 0;
    state = plAssetInfo::MissingDependency;
  }

  return state;
}

bool plAssetCurator::AddAssetHash(plString& sPath, bool bIsReference, plUInt64& out_AssetHash, plUInt64& out_ThumbHash, bool bForce)
{
  if (sPath.IsEmpty())
    return true;

  if (plConversionUtils::IsStringUuid(sPath))
  {
    const plUuid guid = plConversionUtils::ConvertStringToUuid(sPath);
    plUInt64 assetHash = 0;
    plUInt64 thumbHash = 0;
    plAssetInfo::TransformState state = UpdateAssetTransformState(guid, assetHash, thumbHash, bForce);
    if (state == plAssetInfo::Unknown || state == plAssetInfo::MissingDependency || state == plAssetInfo::MissingReference)
    {
      plLog::Error("Failed to hash dependency asset '{0}'", sPath);
      return false;
    }

    // Thumbs hash is affected by both transform dependencies and references.
    out_ThumbHash += thumbHash;
    if (!bIsReference)
    {
      // References do not affect the asset hash.
      out_AssetHash += assetHash;
    }
    return true;
  }

  if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
  {
    if (sPath.EndsWith(".color"))
    {
      // TODO: detect non-file assets and skip already in dependency gather function.
      return true;
    }
    plLog::Error("Failed to make path absolute '{0}'", sPath);
    return false;
  }
  plFileStats statDep;
  if (plOSFile::GetFileStats(sPath, statDep).Failed())
  {
    plLog::Error("Failed to retrieve file stats '{0}'", sPath);
    return false;
  }

  plFileStatus fileStatus;
  plTimestamp previousModificationTime;
  {
    PLASMA_LOCK(m_CuratorMutex);
    fileStatus = m_ReferencedFiles[sPath];
    previousModificationTime = fileStatus.m_Timestamp;
  }

  // if the file has been modified, make sure to get updated data
  if (!fileStatus.m_Timestamp.Compare(statDep.m_LastModificationTime, plTimestamp::CompareMode::Identical))
  {
    CURATOR_PROFILE(sPath);
    plFileReader file;
    if (file.Open(sPath).Failed())
    {
      plLog::Error("Failed to open file '{0}'", sPath);
      return false;
    }
    fileStatus.m_Timestamp = statDep.m_LastModificationTime;
    fileStatus.m_uiHash = plAssetCurator::HashFile(file, nullptr);
    fileStatus.m_Status = plFileStatus::Status::Valid;
  }

  {
    PLASMA_LOCK(m_CuratorMutex);
    plFileStatus& refFile = m_ReferencedFiles[sPath];
    // Only update the status if the file status has not been changed between the locks or we might write stale data to it.
    if (refFile.m_Timestamp.Compare(previousModificationTime, plTimestamp::CompareMode::Identical))
    {
      refFile = fileStatus;
    }
  }

  // Thumbs hash is affected by both transform dependencies and references.
  out_ThumbHash += fileStatus.m_uiHash;
  if (!bIsReference)
  {
    // References do not affect the asset hash.
    out_AssetHash += fileStatus.m_uiHash;
  }
  return true;
}

plResult plAssetCurator::EnsureAssetInfoUpdated(const plUuid& assetGuid)
{
  PLASMA_LOCK(m_CuratorMutex);
  plAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
    return PLASMA_FAILURE;

  // It is not safe here to pass pInfo->m_sAbsolutePath into EnsureAssetInfoUpdated
  // as the function is meant to change the very instance we are passing in.
  plStringBuilder sAbsPath = pInfo->m_sAbsolutePath;
  return EnsureAssetInfoUpdated(sAbsPath);
}

static plResult PatchAssetGuid(const char* szAbsFilePath, plUuid oldGuid, plUuid newGuid)
{
  const plDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (plDocumentManager::FindDocumentTypeFromPath(szAbsFilePath, true, pTypeDesc).Failed())
    return PLASMA_FAILURE;

  if (plDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(szAbsFilePath))
  {
    pTypeDesc->m_pManager->CloseDocument(pDocument);
  }

  plUInt32 uiTries = 0;
  while (pTypeDesc->m_pManager->CloneDocument(szAbsFilePath, szAbsFilePath, newGuid).Failed())
  {
    if (uiTries >= 5)
      return PLASMA_FAILURE;

    plThreadUtils::Sleep(plTime::Milliseconds(50 * (uiTries + 1)));
    uiTries++;
  }

  return PLASMA_SUCCESS;
}

plResult plAssetCurator::EnsureAssetInfoUpdated(const char* szAbsFilePath)
{
  CURATOR_PROFILE(szAbsFilePath);
  plFileStats fs;
  {
    CURATOR_PROFILE("GetFileStats");
    if (plOSFile::GetFileStats(szAbsFilePath, fs).Failed())
      return PLASMA_FAILURE;
  }
  {
    // If the file stat matches our stored timestamp, we are still up to date.
    PLASMA_LOCK(m_CuratorMutex);
    if (m_ReferencedFiles[szAbsFilePath].m_Timestamp.Compare(fs.m_LastModificationTime, plTimestamp::CompareMode::Identical))
      return PLASMA_SUCCESS;
  }

  // Read document info outside the lock
  plFileStatus fileStatus;
  plUniquePtr<plAssetInfo> pNewAssetInfo;
  PLASMA_SUCCEED_OR_RETURN(ReadAssetDocumentInfo(szAbsFilePath, fileStatus, pNewAssetInfo));
  PLASMA_ASSERT_DEV(pNewAssetInfo != nullptr && pNewAssetInfo->m_Info != nullptr, "Info should be valid on success.");

  PLASMA_LOCK(m_CuratorMutex);
  plFileStatus& RefFile = m_ReferencedFiles[szAbsFilePath];
  plUuid oldGuid = RefFile.m_AssetGuid;
  // if it already has a valid GUID, an plAssetInfo object must exist
  bool bNew = !RefFile.m_AssetGuid.IsValid(); // Under this current location the asset is not known.
  PLASMA_VERIFY(bNew == !m_KnownAssets.Contains(RefFile.m_AssetGuid), "guid set in file-status but no asset is actually known under that guid");

  RefFile = fileStatus;
  plAssetInfo* pOldAssetInfo = nullptr;
  if (bNew)
  {
    // now the GUID must be valid
    PLASMA_ASSERT_DEV(pNewAssetInfo->m_Info->m_DocumentID.IsValid(), "Asset header read for '{0}', but its GUID is invalid! Corrupted document?", szAbsFilePath);
    PLASMA_ASSERT_DEV(RefFile.m_AssetGuid == pNewAssetInfo->m_Info->m_DocumentID, "UpdateAssetInfo broke the GUID!");

    // Was the asset already known? Decide whether it was moved (ok) or duplicated (bad)
    m_KnownAssets.TryGetValue(pNewAssetInfo->m_Info->m_DocumentID, pOldAssetInfo);
    if (pOldAssetInfo != nullptr)
    {
      if (pNewAssetInfo->m_sAbsolutePath == pOldAssetInfo->m_sAbsolutePath)
      {
        // As it is a new asset, this should actually never be the case.
        UntrackDependencies(pOldAssetInfo);
        pOldAssetInfo->Update(pNewAssetInfo);
        TrackDependencies(pOldAssetInfo);
        UpdateAssetTransformState(RefFile.m_AssetGuid, plAssetInfo::TransformState::Unknown);
        SetAssetExistanceState(*pOldAssetInfo, plAssetExistanceState::FileModified);
        UpdateSubAssets(*pOldAssetInfo);
        RefFile.m_AssetGuid = pOldAssetInfo->m_Info->m_DocumentID;
        return PLASMA_SUCCESS;
      }
      else
      {
        plFileStats fsOldLocation;
        if (plOSFile::GetFileStats(pOldAssetInfo->m_sAbsolutePath, fsOldLocation).Failed())
        {
          // Asset moved, remove old file and asset info.
          m_ReferencedFiles.Remove(pOldAssetInfo->m_sAbsolutePath);
          UntrackDependencies(pOldAssetInfo);
          pOldAssetInfo->Update(pNewAssetInfo);
          TrackDependencies(pOldAssetInfo);
          UpdateAssetTransformState(RefFile.m_AssetGuid, plAssetInfo::TransformState::Unknown);
          SetAssetExistanceState(*pOldAssetInfo,
            plAssetExistanceState::FileModified); // asset was only moved, prevent added event (could have been modified though)
          UpdateSubAssets(*pOldAssetInfo);
          RefFile.m_AssetGuid = pOldAssetInfo->m_Info->m_DocumentID;
          return PLASMA_SUCCESS;
        }
        else
        {
          // Unfortunately we only know about duplicates in the order in which the filesystem tells us about files
          // That means we currently always adjust the GUID of the second, third, etc. file that we look at
          // even if we might know that changing another file makes more sense
          // This works well for when the editor is running and someone copies a file.

          plLog::Error("Two assets have identical GUIDs: '{0}' and '{1}'", pNewAssetInfo->m_sAbsolutePath, pOldAssetInfo->m_sAbsolutePath);

          const plUuid mod = plUuid::StableUuidForString(szAbsFilePath);
          plUuid newGuid = pNewAssetInfo->m_Info->m_DocumentID;
          newGuid.CombineWithSeed(mod);

          if (PatchAssetGuid(szAbsFilePath, pNewAssetInfo->m_Info->m_DocumentID, newGuid).Failed())
          {
            plLog::Error("Failed to adjust GUID of asset: '{0}'", szAbsFilePath);
            m_ReferencedFiles.Remove(szAbsFilePath);
            return PLASMA_FAILURE;
          }

          plLog::Warning("Adjusted GUID of asset to make it unique: '{0}'", szAbsFilePath);

          // now let's try that again
          m_ReferencedFiles.Remove(szAbsFilePath);
          return EnsureAssetInfoUpdated(szAbsFilePath);
        }
      }
    }

    // and we can store the new plAssetInfo data under that GUID
    pOldAssetInfo = pNewAssetInfo.Release();
    m_KnownAssets[RefFile.m_AssetGuid] = pOldAssetInfo;
    TrackDependencies(pOldAssetInfo);
    UpdateAssetTransformState(pOldAssetInfo->m_Info->m_DocumentID, plAssetInfo::TransformState::Unknown);
    UpdateSubAssets(*pOldAssetInfo);
  }
  else
  {
    // Guid changed, different asset found, mark old as deleted and add new one.
    if (oldGuid != RefFile.m_AssetGuid)
    {
      SetAssetExistanceState(*m_KnownAssets[oldGuid], plAssetExistanceState::FileRemoved);
      RemoveAssetTransformState(oldGuid);

      if (RefFile.m_AssetGuid.IsValid())
      {
        pOldAssetInfo = pNewAssetInfo.Release();
        m_KnownAssets[RefFile.m_AssetGuid] = pOldAssetInfo;
        TrackDependencies(pOldAssetInfo);
        // Don't call SetAssetExistanceState on newly created assets as their data structure is initialized in UpdateSubAssets for the first time.
        UpdateSubAssets(*pOldAssetInfo);
      }
    }
    else
    {
      // Update asset info
      pOldAssetInfo = m_KnownAssets[RefFile.m_AssetGuid];
      UntrackDependencies(pOldAssetInfo);
      pOldAssetInfo->Update(pNewAssetInfo);
      TrackDependencies(pOldAssetInfo);
      SetAssetExistanceState(*pOldAssetInfo, plAssetExistanceState::FileModified);
      UpdateSubAssets(*pOldAssetInfo);
    }
  }

  InvalidateAssetTransformState(RefFile.m_AssetGuid);
  return PLASMA_SUCCESS;
}

void plAssetCurator::TrackDependencies(plAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_AssetTransformDependencies, m_InverseDependency, m_UnresolvedDependencies, true);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_RuntimeDependencies, m_InverseReferences, m_UnresolvedReferences, true);

  const plString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, "");
  auto it = m_InverseReferences.FindOrAdd(sTargetFile);
  it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const plString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, outputIt.Key());
    it = m_InverseReferences.FindOrAdd(sTargetFile2);
    it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  }

  UpdateUnresolvedTrackedFiles(m_InverseDependency, m_UnresolvedDependencies);
  UpdateUnresolvedTrackedFiles(m_InverseReferences, m_UnresolvedReferences);
}

void plAssetCurator::UntrackDependencies(plAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_AssetTransformDependencies, m_InverseDependency, m_UnresolvedDependencies, false);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_RuntimeDependencies, m_InverseReferences, m_UnresolvedReferences, false);

  const plString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, "");
  auto it = m_InverseReferences.FindOrAdd(sTargetFile);
  it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const plString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, outputIt.Key());
    it = m_InverseReferences.FindOrAdd(sTargetFile2);
    it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  }
}

void plAssetCurator::UpdateTrackedFiles(const plUuid& assetGuid, const plSet<plString>& files, plMap<plString, plHybridArray<plUuid, 1>>& inverseTracker, plSet<std::tuple<plUuid, plUuid>>& unresolved, bool bAdd)
{
  for (const auto& dep : files)
  {
    plString sPath = dep;

    if (sPath.IsEmpty())
      continue;

    if (plConversionUtils::IsStringUuid(sPath))
    {
      const plUuid guid = plConversionUtils::ConvertStringToUuid(sPath);
      const plAssetInfo* pInfo = GetAssetInfo(guid);

      if (!bAdd)
      {
        unresolved.Remove(std::tuple<plUuid, plUuid>(assetGuid, guid));
        if (pInfo == nullptr)
          continue;
      }

      if (pInfo == nullptr && bAdd)
      {
        unresolved.Insert(std::tuple<plUuid, plUuid>(assetGuid, guid));
        continue;
      }

      sPath = pInfo->m_sAbsolutePath;
    }
    else
    {
      if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
      {
        continue;
      }
    }
    auto it = inverseTracker.FindOrAdd(sPath);
    if (bAdd)
    {
      it.Value().PushBack(assetGuid);
    }
    else
    {
      it.Value().RemoveAndCopy(assetGuid);
    }
  }
}

void plAssetCurator::UpdateUnresolvedTrackedFiles(plMap<plString, plHybridArray<plUuid, 1>>& inverseTracker, plSet<std::tuple<plUuid, plUuid>>& unresolved)
{
  for (auto it = unresolved.GetIterator(); it.IsValid();)
  {
    auto& t = *it;
    const plUuid& assetGuid = std::get<0>(t);
    const plUuid& depGuid = std::get<1>(t);
    if (const plAssetInfo* pInfo = GetAssetInfo(depGuid))
    {
      plString sPath = pInfo->m_sAbsolutePath;
      auto itTracker = inverseTracker.FindOrAdd(sPath);
      itTracker.Value().PushBack(assetGuid);
      it = unresolved.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

plResult plAssetCurator::ReadAssetDocumentInfo(const char* szAbsFilePath, plFileStatus& stat, plUniquePtr<plAssetInfo>& out_assetInfo)
{
  CURATOR_PROFILE(szAbsFilePath);

  plFileStats fs;
  if (plOSFile::GetFileStats(szAbsFilePath, fs).Failed())
    return PLASMA_FAILURE;
  stat.m_Timestamp = fs.m_LastModificationTime;
  stat.m_Status = plFileStatus::Status::Valid;

  // try to read the asset file
  plFileReader file;
  if (file.Open(szAbsFilePath) == PLASMA_FAILURE)
  {
    stat.m_Timestamp.Invalidate();
    stat.m_uiHash = 0;
    stat.m_Status = plFileStatus::Status::FileLocked;

    plLog::Error("Failed to open asset file '{0}'", szAbsFilePath);
    return PLASMA_FAILURE;
  }

  out_assetInfo = PLASMA_DEFAULT_NEW(plAssetInfo);
  plUniquePtr<plAssetDocumentInfo> docInfo;
  auto itFile = m_CachedFiles.Find(szAbsFilePath);
  {
    PLASMA_LOCK(m_CachedAssetsMutex);
    auto itAsset = m_CachedAssets.Find(szAbsFilePath);
    if (itAsset.IsValid())
    {
      docInfo = std::move(itAsset.Value());
      m_CachedAssets.Remove(itAsset);
    }
  }

  // update the paths
  {
    plStringBuilder sDataDir = GetSingleton()->FindDataDirectoryForAsset(szAbsFilePath);
    sDataDir.PathParentDirectory();

    plStringBuilder sRelPath = szAbsFilePath;
    sRelPath.MakeRelativeTo(sDataDir).IgnoreResult();

    out_assetInfo->m_sDataDirParentRelativePath = sRelPath;
    out_assetInfo->m_sDataDirRelativePath = plStringView(out_assetInfo->m_sDataDirParentRelativePath.FindSubString("/") + 1);
    out_assetInfo->m_sAbsolutePath = szAbsFilePath;
  }

  // figure out which manager should handle this asset type
  {
    const plDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (out_assetInfo->m_pDocumentTypeDescriptor == nullptr)
    {
      if (plDocumentManager::FindDocumentTypeFromPath(szAbsFilePath, false, pTypeDesc).Failed())
      {
        PLASMA_REPORT_FAILURE("Invalid asset setup");
      }

      out_assetInfo->m_pDocumentTypeDescriptor = static_cast<const plAssetDocumentTypeDescriptor*>(pTypeDesc);
    }
  }

  plDefaultMemoryStreamStorage storage;
  plMemoryStreamReader MemReader(&storage);
  MemReader.SetDebugSourceInformation(out_assetInfo->m_sAbsolutePath);

  plMemoryStreamWriter MemWriter(&storage);

  if (docInfo && itFile.IsValid() && itFile.Value().m_Timestamp.Compare(stat.m_Timestamp, plTimestamp::CompareMode::Identical))
  {
    stat.m_uiHash = itFile.Value().m_uiHash;
  }
  else
  {
    // compute the hash for the asset file
    stat.m_uiHash = plAssetCurator::HashFile(file, &MemWriter);
  }
  file.Close();

  // and finally actually read the asset file (header only) and store the information in the plAssetDocumentInfo member
  if (docInfo && itFile.IsValid() && itFile.Value().m_Timestamp.Compare(stat.m_Timestamp, plTimestamp::CompareMode::Identical))
  {
    out_assetInfo->m_Info = std::move(docInfo);
    stat.m_AssetGuid = out_assetInfo->m_Info->m_DocumentID;
  }
  else
  {
    plStatus ret = out_assetInfo->GetManager()->ReadAssetDocumentInfo(out_assetInfo->m_Info, MemReader);
    if (ret.Failed())
    {
      plLog::Error("Failed to read asset document info for asset file '{0}'", szAbsFilePath);
      return PLASMA_FAILURE;
    }
    PLASMA_ASSERT_DEV(out_assetInfo->m_Info != nullptr, "Info should be valid on suceess.");

    if (out_assetInfo->m_Info == NULL)
      return PLASMA_FAILURE;

    // here we get the GUID out of the document
    // this links the 'file' to the 'asset'
    stat.m_AssetGuid = out_assetInfo->m_Info->m_DocumentID;
  }

  return PLASMA_SUCCESS;
}

void plAssetCurator::UpdateSubAssets(plAssetInfo& assetInfo)
{
  CURATOR_PROFILE("UpdateSubAssets");
  if (assetInfo.m_ExistanceState == plAssetExistanceState::FileRemoved)
  {
    return;
  }

  if (assetInfo.m_ExistanceState == plAssetExistanceState::FileAdded)
  {
    auto& mainSub = m_KnownSubAssets[assetInfo.m_Info->m_DocumentID];
    mainSub.m_bMainAsset = true;
    mainSub.m_ExistanceState = plAssetExistanceState::FileAdded;
    mainSub.m_pAssetInfo = &assetInfo;
    mainSub.m_Data.m_Guid = assetInfo.m_Info->m_DocumentID;
    mainSub.m_Data.m_sSubAssetsDocumentTypeName = assetInfo.m_Info->m_sAssetsDocumentTypeName;
  }

  {
    plHybridArray<plSubAssetData, 4> subAssets;
    {
      CURATOR_PROFILE("FillOutSubAssetList");
      assetInfo.GetManager()->FillOutSubAssetList(*assetInfo.m_Info.Borrow(), subAssets);
    }

    for (const plUuid& sub : assetInfo.m_SubAssets)
    {
      m_KnownSubAssets[sub].m_ExistanceState = plAssetExistanceState::FileRemoved;
      m_SubAssetChanged.Insert(sub);
    }

    for (const plSubAssetData& data : subAssets)
    {
      const bool bExisted = m_KnownSubAssets.Find(data.m_Guid).IsValid();
      //PLASMA_ASSERT_DEV(bExisted == assetInfo.m_SubAssets.Contains(data.m_Guid), "Implementation error: m_KnownSubAssets and assetInfo.m_SubAssets are out of sync.");

      plSubAsset sub;
      sub.m_bMainAsset = false;
      sub.m_ExistanceState = bExisted ? plAssetExistanceState::FileModified : plAssetExistanceState::FileAdded;
      sub.m_pAssetInfo = &assetInfo;
      sub.m_Data = data;
      m_KnownSubAssets.Insert(data.m_Guid, sub);

      if (!bExisted)
      {
        assetInfo.m_SubAssets.Insert(sub.m_Data.m_Guid);
        m_SubAssetChanged.Insert(sub.m_Data.m_Guid);
      }
    }

    for (auto it = assetInfo.m_SubAssets.GetIterator(); it.IsValid();)
    {
      if (m_KnownSubAssets[it.Key()].m_ExistanceState == plAssetExistanceState::FileRemoved)
      {
        it = assetInfo.m_SubAssets.Remove(it);
      }
      else
      {
        ++it;
      }
    }
  }
}

plUInt64 plAssetCurator::HashFile(plStreamReader& InputStream, plStreamWriter* pPassThroughStream)
{
  plHashStreamWriter64 hsw;

  CURATOR_PROFILE("HashFile");
  plUInt8 cachedBytes[1024 * 10];

  while (true)
  {
    const plUInt64 uiRead = InputStream.ReadBytes(cachedBytes, PLASMA_ARRAY_SIZE(cachedBytes));

    if (uiRead == 0)
      break;

    hsw.WriteBytes(cachedBytes, uiRead).IgnoreResult();

    if (pPassThroughStream != nullptr)
      pPassThroughStream->WriteBytes(cachedBytes, uiRead).IgnoreResult();
  }

  return hsw.GetHashValue();
}

void plAssetCurator::RemoveAssetTransformState(const plUuid& assetGuid)
{
  PLASMA_LOCK(m_CuratorMutex);

  for (int i = 0; i < plAssetInfo::TransformState::COUNT; i++)
  {
    m_TransformState[i].Remove(assetGuid);
  }
  m_TransformStateStale.Remove(assetGuid);
}


void plAssetCurator::InvalidateAssetTransformState(const plUuid& assetGuid)
{
  PLASMA_LOCK(m_CuratorMutex);

  plAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
  {
    // We do not set pAssetInfo->m_TransformState because that is user facing and
    // as after updating the state it might just be the same as before we instead add
    // it to the queue here to prevent flickering in the GUI.
    m_TransformStateStale.Insert(assetGuid);
    // Increasing m_LastStateUpdate will ensure that asset hash/state computations
    // that are in flight will not be written back to the asset.
    pAssetInfo->m_LastStateUpdate++;
    pAssetInfo->m_AssetHash = 0;
    pAssetInfo->m_ThumbHash = 0;
    auto it = m_InverseDependency.Find(pAssetInfo->m_sAbsolutePath);
    if (it.IsValid())
    {
      for (const plUuid& guid : it.Value())
      {
        InvalidateAssetTransformState(guid);
      }
    }

    auto it2 = m_InverseReferences.Find(pAssetInfo->m_sAbsolutePath);
    if (it2.IsValid())
    {
      for (const plUuid& guid : it2.Value())
      {
        InvalidateAssetTransformState(guid);
      }
    }
  }
}

void plAssetCurator::UpdateAssetTransformState(const plUuid& assetGuid, plAssetInfo::TransformState state)
{
  PLASMA_LOCK(m_CuratorMutex);

  plAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
  {
    m_TransformStateStale.Remove(assetGuid);
    for (int i = 0; i < plAssetInfo::TransformState::COUNT; i++)
    {
      m_TransformState[i].Remove(assetGuid);
    }
    m_TransformState[state].Insert(assetGuid);

    const bool bStateChanged = pAssetInfo->m_TransformState != state;

    if (bStateChanged)
    {
      pAssetInfo->m_TransformState = state;
      m_SubAssetChanged.Insert(assetGuid);
      for (const auto& key : pAssetInfo->m_SubAssets)
      {
        m_SubAssetChanged.Insert(key);
      }
    }

    switch (state)
    {
      case plAssetInfo::TransformState::TransformError:
      {
        // Transform errors are unexpected and invalidate any previously computed
        // state of assets depending on this one.
        auto it = m_InverseDependency.Find(pAssetInfo->m_sAbsolutePath);
        if (it.IsValid())
        {
          for (const plUuid& guid : it.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        auto it2 = m_InverseReferences.Find(pAssetInfo->m_sAbsolutePath);
        if (it2.IsValid())
        {
          for (const plUuid& guid : it2.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        break;
      }

      case plAssetInfo::TransformState::Unknown:
      {
        InvalidateAssetTransformState(assetGuid);
        break;
      }

      case plAssetInfo::TransformState::UpToDate:
      {
        if (bStateChanged)
        {
          plString sThumbPath = pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pAssetInfo->m_sAbsolutePath);
          plQtImageCache::GetSingleton()->InvalidateCache(sThumbPath);

          for (auto& subAssetUuid : pAssetInfo->m_SubAssets)
          {
            plSubAsset* pSubAsset;
            if (m_KnownSubAssets.TryGetValue(subAssetUuid, pSubAsset))
            {
              sThumbPath = pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pAssetInfo->m_sAbsolutePath, pSubAsset->m_Data.m_sName);
              plQtImageCache::GetSingleton()->InvalidateCache(sThumbPath);
            }
          }
        }
        break;
      }

      default:
        break;
    }
  }
}

void plAssetCurator::UpdateAssetTransformLog(const plUuid& assetGuid, plDynamicArray<plLogEntry>& logEntries)
{
  plAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
  {
    pAssetInfo->m_LogEntries.Clear();
    pAssetInfo->m_LogEntries.Swap(logEntries);
  }
}


void plAssetCurator::SetAssetExistanceState(plAssetInfo& assetInfo, plAssetExistanceState::Enum state)
{
  PLASMA_ASSERT_DEBUG(m_CuratorMutex.IsLocked(), "");

  // Only the main thread tick function is allowed to change from FileAdded to FileModified to inform views.
  // A modified 'added' file is still added until the added state was addressed.
  if (assetInfo.m_ExistanceState != plAssetExistanceState::FileAdded || state != plAssetExistanceState::FileModified)
    assetInfo.m_ExistanceState = state;

  for (plUuid subGuid : assetInfo.m_SubAssets)
  {
    auto& existanceState = GetSubAssetInternal(subGuid)->m_ExistanceState;
    if (existanceState != plAssetExistanceState::FileAdded || state != plAssetExistanceState::FileModified)
    {
      existanceState = state;
      m_SubAssetChanged.Insert(subGuid);
    }
  }

  auto& existanceState = GetSubAssetInternal(assetInfo.m_Info->m_DocumentID)->m_ExistanceState;
  if (existanceState != plAssetExistanceState::FileAdded || state != plAssetExistanceState::FileModified)
  {
    existanceState = state;
    m_SubAssetChanged.Insert(assetInfo.m_Info->m_DocumentID);
  }
}


////////////////////////////////////////////////////////////////////////
// plUpdateTask
////////////////////////////////////////////////////////////////////////

plUpdateTask::plUpdateTask(plOnTaskFinishedCallback onTaskFinished)
{
  ConfigureTask("plUpdateTask", plTaskNesting::Maybe, onTaskFinished);
}

plUpdateTask::~plUpdateTask() = default;

void plUpdateTask::Execute()
{
  plUuid assetGuid;
  {
    PLASMA_LOCK(plAssetCurator::GetSingleton()->m_CuratorMutex);
    if (!plAssetCurator::GetSingleton()->GetNextAssetToUpdate(assetGuid, m_sAssetPath))
      return;
  }

  const plDocumentTypeDescriptor* pTypeDescriptor = nullptr;
  if (plDocumentManager::FindDocumentTypeFromPath(m_sAssetPath, false, pTypeDescriptor).Failed())
    return;

  plUInt64 uiAssetHash = 0;
  plUInt64 uiThumbHash = 0;

  // Do not log update errors done on the background thread. Only if done explicitly on the main thread or the GUI will not be responsive
  // if the user deleted some base asset and everything starts complaining about it.
  plLogEntryDelegate logger([&](plLogEntry& entry) -> void {}, plLogMsgType::All);
  plLogSystemScope logScope(&logger);

  plAssetCurator::GetSingleton()->IsAssetUpToDate(assetGuid, plAssetCurator::GetSingleton()->GetActiveAssetProfile(), static_cast<const plAssetDocumentTypeDescriptor*>(pTypeDescriptor), uiAssetHash, uiThumbHash);
}
