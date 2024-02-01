#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

////////////////////////////////////////////////////////////////////////
// plAssetCurator Asset Hashing and Status Updates
////////////////////////////////////////////////////////////////////////

plAssetInfo::TransformState plAssetCurator::HashAsset(plUInt64 uiSettingsHash, const plHybridArray<plString, 16>& assetTransformDeps, const plHybridArray<plString, 16>& assetThumbnailDeps, plSet<plString>& missingTransformDeps, plSet<plString>& missingThumbnailDeps, plUInt64& out_AssetHash, plUInt64& out_ThumbHash, bool bForce)
{
  CURATOR_PROFILE("HashAsset");
  plStringBuilder tmp;
  plAssetInfo::TransformState state = plAssetInfo::Unknown;
  {
    // hash of the main asset file
    out_AssetHash = uiSettingsHash;
    out_ThumbHash = uiSettingsHash;

    // Iterate dependencies
    for (const auto& dep : assetTransformDeps)
    {
      plString sPath = dep;
      if (!AddAssetHash(sPath, false, out_AssetHash, out_ThumbHash, bForce))
      {
        missingTransformDeps.Insert(sPath);
      }
    }

    for (const auto& dep : assetThumbnailDeps)
    {
      plString sPath = dep;
      if (!AddAssetHash(sPath, true, out_AssetHash, out_ThumbHash, bForce))
      {
        missingThumbnailDeps.Insert(sPath);
      }
    }
  }

  if (!missingThumbnailDeps.IsEmpty())
  {
    out_ThumbHash = 0;
    state = plAssetInfo::MissingThumbnailDependency;
  }
  if (!missingTransformDeps.IsEmpty())
  {
    out_AssetHash = 0;
    out_ThumbHash = 0;
    state = plAssetInfo::MissingTransformDependency;
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
    if (state == plAssetInfo::Unknown || state == plAssetInfo::MissingTransformDependency || state == plAssetInfo::MissingThumbnailDependency || state == plAssetInfo::CircularDependency)
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

  plFileStatus fileStatus;
  plResult res = plFileSystemModel::GetSingleton()->HashFile(sPath, fileStatus);
  if (res.Failed())
  {
    return false;
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

static plResult PatchAssetGuid(plStringView sAbsFilePath, plUuid oldGuid, plUuid newGuid)
{
  const plDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (plDocumentManager::FindDocumentTypeFromPath(sAbsFilePath, true, pTypeDesc).Failed())
    return PL_FAILURE;

  plUInt32 uiTries = 0;

  plStringBuilder sTemp;
  plStringBuilder sTempTarget = plOSFile::GetTempDataFolder();
  sTempTarget.AppendPath(plPathUtils::GetFileNameAndExtension(sAbsFilePath));
  sTempTarget.ChangeFileName(plConversionUtils::ToString(newGuid, sTemp));

  sTemp = sAbsFilePath;
  while (pTypeDesc->m_pManager->CloneDocument(sTemp, sTempTarget, newGuid).Failed())
  {
    if (uiTries >= 5)
      return PL_FAILURE;

    plThreadUtils::Sleep(plTime::MakeFromMilliseconds(50 * (uiTries + 1)));
    uiTries++;
  }

  plResult res = plOSFile::CopyFile(sTempTarget, sAbsFilePath);
  plOSFile::DeleteFile(sTempTarget).IgnoreResult();
  return res;
}

plResult plAssetCurator::EnsureAssetInfoUpdated(const plDataDirPath& absFilePath, const plFileStatus& stat, bool bForce)
{
  CURATOR_PROFILE(absFilePath);

  plFileSystemModel* pFiles = plFileSystemModel::GetSingleton();

  // Read document info outside the lock
  plUniquePtr<plAssetInfo> pNewAssetInfo;
  PL_SUCCEED_OR_RETURN(ReadAssetDocumentInfo(absFilePath, stat, pNewAssetInfo));
  PL_ASSERT_DEV(pNewAssetInfo != nullptr && pNewAssetInfo->m_Info != nullptr, "Info should be valid on success.");


  PL_LOCK(m_CuratorMutex);
  const plUuid oldGuid = stat.m_DocumentID;
  // if it already has a valid GUID, an plAssetInfo object must exist
  const bool bNewAssetFile = !stat.m_DocumentID.IsValid(); // Under this current location the asset is not known.
  plUuid newGuid = pNewAssetInfo->m_Info->m_DocumentID;

  plAssetInfo* pCurrentAssetInfo = nullptr;
  // Was the asset already known? Decide whether it was moved (ok) or duplicated (bad)
  m_KnownAssets.TryGetValue(pNewAssetInfo->m_Info->m_DocumentID, pCurrentAssetInfo);

  plEnum<plAssetExistanceState> newExistanceState = plAssetExistanceState::FileUnchanged;
  if (bNewAssetFile && pCurrentAssetInfo != nullptr)
  {
    plFileStats fsOldLocation;
    const bool IsSameFile = plFileSystemModel::IsSameFile(pNewAssetInfo->m_Path, pCurrentAssetInfo->m_Path);
    const plResult statCheckOldLocation = plOSFile::GetFileStats(pCurrentAssetInfo->m_Path, fsOldLocation);

    if (statCheckOldLocation.Succeeded() && !IsSameFile)
    {
      // DUPLICATED
      // Unfortunately we only know about duplicates in the order in which the filesystem tells us about files
      // That means we currently always adjust the GUID of the second, third, etc. file that we look at
      // even if we might know that changing another file makes more sense
      // This works well for when the editor is running and someone copies a file.

      plLog::Error("Two assets have identical GUIDs: '{0}' and '{1}'", pNewAssetInfo->m_Path.GetAbsolutePath(), pCurrentAssetInfo->m_Path.GetAbsolutePath());

      const plUuid mod = plUuid::MakeStableUuidFromString(absFilePath);
      plUuid replacementGuid = pNewAssetInfo->m_Info->m_DocumentID;
      replacementGuid.CombineWithSeed(mod);

      if (PatchAssetGuid(absFilePath, pNewAssetInfo->m_Info->m_DocumentID, replacementGuid).Failed())
      {
        plLog::Error("Failed to adjust GUID of asset: '{0}'", absFilePath);
        pFiles->NotifyOfChange(absFilePath);
        return PL_FAILURE;
      }

      plLog::Warning("Adjusted GUID of asset to make it unique: '{0}'", absFilePath);

      // now let's try that again
      pFiles->NotifyOfChange(absFilePath);
      return PL_SUCCESS;
    }
    else
    {
      // MOVED
      // Notify old location to removed stale entry.
      pFiles->UnlinkDocument(pCurrentAssetInfo->m_Path).IgnoreResult();
      pFiles->NotifyOfChange(pCurrentAssetInfo->m_Path);
      newExistanceState = plAssetExistanceState::FileMoved;
    }
  }

  // Guid changed, different asset found, mark old as deleted and add new one.
  if (!bNewAssetFile && oldGuid != pNewAssetInfo->m_Info->m_DocumentID)
  {
    // OVERWRITTEN
    SetAssetExistanceState(*m_KnownAssets[oldGuid], plAssetExistanceState::FileRemoved);
    RemoveAssetTransformState(oldGuid);
    newExistanceState = plAssetExistanceState::FileAdded;
  }

  if (pCurrentAssetInfo)
  {
    UntrackDependencies(pCurrentAssetInfo);
    pCurrentAssetInfo->Update(pNewAssetInfo);
    // Only update if it was not already set to not overwrite, e.g. FileMoved.
    if (newExistanceState == plAssetExistanceState::FileUnchanged)
      newExistanceState = plAssetExistanceState::FileModified;
  }
  else
  {
    pCurrentAssetInfo = pNewAssetInfo.Release();
    m_KnownAssets[newGuid] = pCurrentAssetInfo;
    newExistanceState = plAssetExistanceState::FileAdded;
  }

  TrackDependencies(pCurrentAssetInfo);
  CheckForCircularDependencies(pCurrentAssetInfo).IgnoreResult();
  UpdateAssetTransformState(newGuid, plAssetInfo::TransformState::Unknown);
  // Don't call SetAssetExistanceState on newly created assets as their data structure is initialized in UpdateSubAssets for the first time.
  if (newExistanceState != plAssetExistanceState::FileAdded)
    SetAssetExistanceState(*pCurrentAssetInfo, newExistanceState);
  UpdateSubAssets(*pCurrentAssetInfo);

  InvalidateAssetTransformState(newGuid);
  pFiles->LinkDocument(absFilePath, pCurrentAssetInfo->m_Info->m_DocumentID).AssertSuccess("Failed to link document in file system model");
  return PL_SUCCESS;
}

void plAssetCurator::TrackDependencies(plAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_TransformDependencies, m_InverseTransformDeps, m_UnresolvedTransformDeps, true);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_ThumbnailDependencies, m_InverseThumbnailDeps, m_UnresolvedThumbnailDeps, true);

  const plString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_Path, "");
  auto it = m_InverseThumbnailDeps.FindOrAdd(sTargetFile);
  it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const plString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_Path, outputIt.Key());
    it = m_InverseThumbnailDeps.FindOrAdd(sTargetFile2);
    it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  }

  // Depending on the order of loading, dependencies might be unresolved until the dependency itself is loaded into the curator.
  // If pAssetInfo was previously an unresolved dependency, these two calls will update the inverse dep tables now that it can be resolved.
  UpdateUnresolvedTrackedFiles(m_InverseTransformDeps, m_UnresolvedTransformDeps);
  UpdateUnresolvedTrackedFiles(m_InverseThumbnailDeps, m_UnresolvedThumbnailDeps);
}

void plAssetCurator::UntrackDependencies(plAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_TransformDependencies, m_InverseTransformDeps, m_UnresolvedTransformDeps, false);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_ThumbnailDependencies, m_InverseThumbnailDeps, m_UnresolvedThumbnailDeps, false);

  const plString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_Path, "");
  auto it = m_InverseThumbnailDeps.FindOrAdd(sTargetFile);
  it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const plString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_Path, outputIt.Key());
    it = m_InverseThumbnailDeps.FindOrAdd(sTargetFile2);
    it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  }
}

plResult plAssetCurator::CheckForCircularDependencies(plAssetInfo* pAssetInfo)
{
  plSet<plUuid> inverseHull;
  GenerateInverseTransitiveHull(pAssetInfo, inverseHull, true, true);

  plResult res = PL_SUCCESS;
  for (const auto& sDep : pAssetInfo->m_Info->m_TransformDependencies)
  {
    if (plConversionUtils::IsStringUuid(sDep))
    {
      const plUuid guid = plConversionUtils::ConvertStringToUuid(sDep);
      if (inverseHull.Contains(guid))
      {
        pAssetInfo->m_CircularDependencies.Insert(sDep);
        res = PL_FAILURE;
      }
    }
  }

  for (const auto& sDep : pAssetInfo->m_Info->m_ThumbnailDependencies)
  {
    if (plConversionUtils::IsStringUuid(sDep))
    {
      const plUuid guid = plConversionUtils::ConvertStringToUuid(sDep);
      if (inverseHull.Contains(guid))
      {
        pAssetInfo->m_CircularDependencies.Insert(sDep);
        res = PL_FAILURE;
      }
    }
  }
  return res;
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

      sPath = pInfo->m_Path.GetAbsolutePath();
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
      plString sPath = pInfo->m_Path.GetAbsolutePath();
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

plResult plAssetCurator::ReadAssetDocumentInfo(const plDataDirPath& absFilePath, const plFileStatus& stat, plUniquePtr<plAssetInfo>& out_assetInfo)
{
  CURATOR_PROFILE(szAbsFilePath);
  plFileSystemModel* pFiles = plFileSystemModel::GetSingleton();

  out_assetInfo = PL_DEFAULT_NEW(plAssetInfo);
  out_assetInfo->m_Path = absFilePath;

  // figure out which manager should handle this asset type
  {
    const plDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (out_assetInfo->m_pDocumentTypeDescriptor == nullptr)
    {
      if (plDocumentManager::FindDocumentTypeFromPath(absFilePath, false, pTypeDesc).Failed())
      {
        PL_REPORT_FAILURE("Invalid asset setup");
      }

      out_assetInfo->m_pDocumentTypeDescriptor = static_cast<const plAssetDocumentTypeDescriptor*>(pTypeDesc);
    }
  }

  // Try cache first
  {
    plFileStatus cacheStat;
    plUniquePtr<plAssetDocumentInfo> docInfo;
    {
      PL_LOCK(m_CachedAssetsMutex);
      auto itFile = m_CachedFiles.Find(absFilePath);
      auto itAsset = m_CachedAssets.Find(absFilePath);
      if (itAsset.IsValid() && itFile.IsValid())
      {
        docInfo = std::move(itAsset.Value());
        cacheStat = itFile.Value();
        m_CachedAssets.Remove(itAsset);
        m_CachedFiles.Remove(itFile);
      }
    }

    if (docInfo && cacheStat.m_LastModified.Compare(stat.m_LastModified, plTimestamp::CompareMode::Identical))
    {
      out_assetInfo->m_Info = std::move(docInfo);
      return PL_SUCCESS;
    }
  }

  // try to read the asset file
  plStatus infoStatus;
  plResult res = pFiles->ReadDocument(absFilePath, [&out_assetInfo, &infoStatus](const plFileStatus& stat, plStreamReader& ref_reader) { infoStatus = out_assetInfo->GetManager()->ReadAssetDocumentInfo(out_assetInfo->m_Info, ref_reader); });

  if (infoStatus.Failed())
  {
    plLog::Error("Failed to read asset document info for asset file '{0}'", absFilePath);
    return PL_FAILURE;
  }

  PL_ASSERT_DEV(out_assetInfo->m_Info != nullptr, "Info should be valid on suceess.");
  return res;
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
      PL_ASSERT_DEV(bExisted == assetInfo.m_SubAssets.Contains(data.m_Guid), "Implementation error: m_KnownSubAssets and assetInfo.m_SubAssets are out of sync.");

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

void plAssetCurator::RemoveAssetTransformState(const plUuid& assetGuid)
{
  PL_LOCK(m_CuratorMutex);

  for (int i = 0; i < plAssetInfo::TransformState::COUNT; i++)
  {
    m_TransformState[i].Remove(assetGuid);
  }
  m_TransformStateStale.Remove(assetGuid);
}


void plAssetCurator::InvalidateAssetTransformState(const plUuid& assetGuid)
{
  PL_LOCK(m_CuratorMutex);

  plSet<plUuid> hull;
  {
    plAssetInfo* pAssetInfo = nullptr;
    if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    {
      GenerateInverseTransitiveHull(pAssetInfo, hull, true, true);
    }
  }

  for (const plUuid& guid : hull)
  {
    plAssetInfo* pAssetInfo = nullptr;
    if (m_KnownAssets.TryGetValue(guid, pAssetInfo))
    {
      // We do not set pAssetInfo->m_TransformState because that is user facing and
      // as after updating the state it might just be the same as before we instead add
      // it to the queue here to prevent flickering in the GUI.
      m_TransformStateStale.Insert(guid);
      // Increasing m_LastStateUpdate will ensure that asset hash/state computations
      // that are in flight will not be written back to the asset.
      pAssetInfo->m_LastStateUpdate++;
      pAssetInfo->m_AssetHash = 0;
      pAssetInfo->m_ThumbHash = 0;
    }
  }
}

void plAssetCurator::UpdateAssetTransformState(const plUuid& assetGuid, plAssetInfo::TransformState state)
{
  PL_LOCK(m_CuratorMutex);

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
        auto it = m_InverseTransformDeps.Find(pAssetInfo->m_Path);
        if (it.IsValid())
        {
          for (const plUuid& guid : it.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        auto it2 = m_InverseThumbnailDeps.Find(pAssetInfo->m_Path);
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
          plString sThumbPath = pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pAssetInfo->m_Path);
          plQtImageCache::GetSingleton()->InvalidateCache(sThumbPath);

          for (auto& subAssetUuid : pAssetInfo->m_SubAssets)
          {
            plSubAsset* pSubAsset;
            if (m_KnownSubAssets.TryGetValue(subAssetUuid, pSubAsset))
            {
              sThumbPath = pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pAssetInfo->m_Path, pSubAsset->m_Data.m_sName);
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
  PL_ASSERT_DEBUG(m_CuratorMutex.IsLocked(), "");

  // Only the main thread tick function is allowed to change from FileAdded / FileRenamed to FileModified to inform views.
  // A modified 'added' file is still added until the added state was addressed.
  auto IsModifiedAfterAddOrRename = [](plAssetExistanceState::Enum oldState, plAssetExistanceState::Enum newState) -> bool {
    return oldState == plAssetExistanceState::FileAdded && newState == plAssetExistanceState::FileModified ||
           oldState == plAssetExistanceState::FileMoved && newState == plAssetExistanceState::FileModified;
  };

  if (!IsModifiedAfterAddOrRename(assetInfo.m_ExistanceState, state))
    assetInfo.m_ExistanceState = state;

  for (plUuid subGuid : assetInfo.m_SubAssets)
  {
    auto& existanceState = GetSubAssetInternal(subGuid)->m_ExistanceState;
    if (!IsModifiedAfterAddOrRename(existanceState, state))
    {
      existanceState = state;
      m_SubAssetChanged.Insert(subGuid);
    }
  }

  auto& existanceState = GetSubAssetInternal(assetInfo.m_Info->m_DocumentID)->m_ExistanceState;
  if (!IsModifiedAfterAddOrRename(existanceState, state))
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
    PL_LOCK(plAssetCurator::GetSingleton()->m_CuratorMutex);
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
  plLogEntryDelegate logger([&](plLogEntry& ref_entry) -> void {}, plLogMsgType::All);
  plLogSystemScope logScope(&logger);

  plAssetCurator::GetSingleton()->IsAssetUpToDate(assetGuid, plAssetCurator::GetSingleton()->GetActiveAssetProfile(), static_cast<const plAssetDocumentTypeDescriptor*>(pTypeDescriptor), uiAssetHash, uiThumbHash);
}
