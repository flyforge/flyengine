#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetTableWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

#define PLASMA_CURATOR_CACHE_VERSION 2      // Change this to delete and re-gen all asset caches.
#define PLASMA_CURATOR_CACHE_FILE_VERSION 8 // Change this if for cache format changes.

PLASMA_IMPLEMENT_SINGLETON(plAssetCurator);

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetCurator)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation",
  "FileSystemModel",
  "DocumentManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    PLASMA_DEFAULT_NEW(plAssetCurator);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plAssetCurator* pDummy = plAssetCurator::GetSingleton();
    PLASMA_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

void plAssetInfo::Update(plUniquePtr<plAssetInfo>& rhs)
{
  // Don't update the existance state, it is handled via plAssetCurator::SetAssetExistanceState
  // m_ExistanceState = rhs->m_ExistanceState;
  m_TransformState = rhs->m_TransformState;
  m_pDocumentTypeDescriptor = rhs->m_pDocumentTypeDescriptor;
  m_Path = std::move(rhs->m_Path);
  m_Info = std::move(rhs->m_Info);

  m_AssetHash = rhs->m_AssetHash;
  m_ThumbHash = rhs->m_ThumbHash;
  m_MissingTransformDeps = std::move(rhs->m_MissingTransformDeps);
  m_MissingThumbnailDeps = std::move(rhs->m_MissingThumbnailDeps);
  m_CircularDependencies = std::move(rhs->m_CircularDependencies);
  // Don't copy m_SubAssets, we want to update it independently.
  rhs = nullptr;
}

plStringView plSubAsset::GetName() const
{
  if (m_bMainAsset)
    return plPathUtils::GetFileName(m_pAssetInfo->m_Path.GetDataDirParentRelativePath());
  else
    return m_Data.m_sName;
}


void plSubAsset::GetSubAssetIdentifier(plStringBuilder& out_sPath) const
{
  out_sPath = m_pAssetInfo->m_Path.GetDataDirParentRelativePath();

  if (!m_bMainAsset)
  {
    out_sPath.Append("|", m_Data.m_sName);
  }
}

////////////////////////////////////////////////////////////////////////
// plAssetCurator Setup
////////////////////////////////////////////////////////////////////////

plAssetCurator::plAssetCurator()
  : m_SingletonRegistrar(this)
{
}

plAssetCurator::~plAssetCurator()
{
  PLASMA_ASSERT_DEBUG(m_KnownAssets.IsEmpty(), "Need to call Deinitialize before curator is deleted.");
}

void plAssetCurator::StartInitialize(const plApplicationFileSystemConfig& cfg)
{
  PLASMA_PROFILE_SCOPE("StartInitialize");

  {
    PLASMA_LOG_BLOCK("SetupAssetProfiles");

    SetupDefaultAssetProfiles();
    if (LoadAssetProfiles().Failed())
    {
      plLog::Warning("Asset profiles file does not exist or contains invalid data. Setting up default profiles.");
      SaveAssetProfiles().IgnoreResult();
      SaveRuntimeProfiles();
    }
  }

  ComputeAllDocumentManagerAssetProfileHashes();
  BuildFileExtensionSet(m_ValidAssetExtensions);

  m_bRunUpdateTask = true;
  m_FileSystemConfig = cfg;

  plFileSystemModel::GetSingleton()->m_FileChangedEvents.AddEventHandler(plMakeDelegate(&plAssetCurator::OnFileChangedEvent, this));
  plFileSystemModel::FilesMap referencedFiles;
  plFileSystemModel::FoldersMap referencedFolders;
  LoadCaches(referencedFiles, referencedFolders);
  // We postpone the plAssetFiles initialize to after we have loaded the cache. No events will be fired before initialize is called.
  plFileSystemModel::GetSingleton()->Initialize(m_FileSystemConfig, std::move(referencedFiles), std::move(referencedFolders));

  m_pAssetTableWriter = PLASMA_DEFAULT_NEW(plAssetTableWriter, m_FileSystemConfig);

  plSharedPtr<plDelegateTask<void>> pInitTask = PLASMA_DEFAULT_NEW(plDelegateTask<void>, "AssetCuratorUpdateCache", plTaskNesting::Never, [this]()
    {
      PLASMA_LOCK(m_CuratorMutex);

      m_CuratorMutex.Unlock();
      CheckFileSystem();
      m_CuratorMutex.Lock();

      // As we fired a AssetListReset in CheckFileSystem, set everything new to FileUnchanged or
      // we would fire an added call for every asset.
      for (auto it = m_KnownSubAssets.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value().m_ExistanceState == plAssetExistanceState::FileAdded)
        {
          it.Value().m_ExistanceState = plAssetExistanceState::FileUnchanged;
        }
      }
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value()->m_ExistanceState == plAssetExistanceState::FileAdded)
        {
          it.Value()->m_ExistanceState = plAssetExistanceState::FileUnchanged;
        }
      }

      // Re-save caches after we made a full CheckFileSystem pass.
      plFileSystemModel::FilesMap referencedFiles;
      plFileSystemModel::FoldersMap referencedFolders;
      plFileSystemModel* pFiles = plFileSystemModel::GetSingleton();
      {
        referencedFiles = *pFiles->GetFiles();
        referencedFolders = *pFiles->GetFolders();
      }
      SaveCaches(referencedFiles, referencedFolders); //
    });
  pInitTask->ConfigureTask("Initialize Curator", plTaskNesting::Never);
  m_InitializeCuratorTaskID = plTaskSystem::StartSingleTask(pInitTask, plTaskPriority::FileAccessHighPriority);

  {
    plAssetCuratorEvent e;
    e.m_Type = plAssetCuratorEvent::Type::ActivePlatformChanged;
    m_Events.Broadcast(e);
  }
}

void plAssetCurator::WaitForInitialize()
{
  PLASMA_PROFILE_SCOPE("WaitForInitialize");
  plTaskSystem::WaitForGroup(m_InitializeCuratorTaskID);
  m_InitializeCuratorTaskID.Invalidate();

  PLASMA_LOCK(m_CuratorMutex);
  ProcessAllCoreAssets();
  // Broadcast reset.
  {
    plAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = plAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }
}

void plAssetCurator::Deinitialize()
{
  PLASMA_PROFILE_SCOPE("Deinitialize");

  SaveAssetProfiles().IgnoreResult();

  ShutdownUpdateTask();
  plAssetProcessor::GetSingleton()->StopProcessTask(true);
  plFileSystemModel* pFiles = plFileSystemModel::GetSingleton();
  plFileSystemModel::FilesMap referencedFiles;
  plFileSystemModel::FoldersMap referencedFolders;
  pFiles->Deinitialize(&referencedFiles, &referencedFolders);
  SaveCaches(referencedFiles, referencedFolders);

  pFiles->m_FileChangedEvents.RemoveEventHandler(plMakeDelegate(&plAssetCurator::OnFileChangedEvent, this));
  pFiles = nullptr;
  m_pAssetTableWriter = nullptr;

  {
    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      PLASMA_DEFAULT_DELETE(it.Value());
    }
    m_KnownSubAssets.Clear();
    m_KnownAssets.Clear();
    m_TransformStateStale.Clear();

    for (int i = 0; i < plAssetInfo::TransformState::COUNT; i++)
    {
      m_TransformState[i].Clear();
    }
  }

  // Broadcast reset.
  {
    plAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = plAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }

  ClearAssetProfiles();
}

void plAssetCurator::MainThreadTick(bool bTopLevel)
{
  CURATOR_PROFILE("MainThreadTick");

  static std::atomic<bool> bReentry = false;
  if (bReentry)
    return;

  if (plQtEditorApp::GetSingleton()->IsProgressBarProcessingEvents())
    return;

  bReentry = true;

  plFileSystemModel::GetSingleton()->MainThreadTick();

  PLASMA_LOCK(m_CuratorMutex);
  plHybridArray<plAssetInfo*, 32> deletedAssets;
  for (const plUuid& guid : m_SubAssetChanged)
  {
    plSubAsset* pInfo = GetSubAssetInternal(guid);
    plAssetCuratorEvent e;
    e.m_AssetGuid = guid;
    e.m_pInfo = pInfo;
    e.m_Type = plAssetCuratorEvent::Type::AssetUpdated;

    if (pInfo != nullptr)
    {
      if (pInfo->m_ExistanceState == plAssetExistanceState::FileAdded)
      {
        pInfo->m_ExistanceState = plAssetExistanceState::FileUnchanged;
        if (pInfo->m_bMainAsset)
          pInfo->m_pAssetInfo->m_ExistanceState = plAssetExistanceState::FileUnchanged;
        e.m_Type = plAssetCuratorEvent::Type::AssetAdded;
        m_Events.Broadcast(e);
      }
      else if (pInfo->m_ExistanceState == plAssetExistanceState::FileMoved)
      {
        if (pInfo->m_bMainAsset)
        {
          // Make sure the document knows that its underlying file was renamed.
          if (plDocument* pDoc = plDocumentManager::GetDocumentByGuid(guid))
            pDoc->DocumentRenamed(pInfo->m_pAssetInfo->m_Path);
        }

        pInfo->m_ExistanceState = plAssetExistanceState::FileUnchanged;
        if (pInfo->m_bMainAsset)
          pInfo->m_pAssetInfo->m_ExistanceState = plAssetExistanceState::FileUnchanged;
        e.m_Type = plAssetCuratorEvent::Type::AssetMoved;
        m_Events.Broadcast(e);
      }
      else if (pInfo->m_ExistanceState == plAssetExistanceState::FileRemoved)
      {
        // this is a bit tricky:
        // when the document is deleted on disk, it would be nicer not to close it (discarding modifications!)
        // instead we could set it as modified
        // but then when it was only moved or renamed that means we have another document with the same GUID
        // so once the user would save the now modified document, we would end up with two documents with the same GUID
        // so, for now, since this is probably a rare case anyway, we just close the document without asking
        plDocumentManager::EnsureDocumentIsClosedInAllManagers(pInfo->m_pAssetInfo->m_Path);
        e.m_Type = plAssetCuratorEvent::Type::AssetRemoved;
        m_Events.Broadcast(e);

        if (pInfo->m_bMainAsset)
        {
          deletedAssets.PushBack(pInfo->m_pAssetInfo);
        }
        m_KnownAssets.Remove(guid);
        m_KnownSubAssets.Remove(guid);
      }
      else // Either plAssetInfo::ExistanceState::FileModified or tranform changed
      {
        pInfo->m_ExistanceState = plAssetExistanceState::FileUnchanged;
        if (pInfo->m_bMainAsset)
          pInfo->m_pAssetInfo->m_ExistanceState = plAssetExistanceState::FileUnchanged;
        e.m_Type = plAssetCuratorEvent::Type::AssetUpdated;
        m_Events.Broadcast(e);
      }
    }
  }
  m_SubAssetChanged.Clear();

  // Delete file asset info after all the sub-assets have been handled (so no ref exist to it anymore).
  for (plAssetInfo* pInfo : deletedAssets)
  {
    PLASMA_DEFAULT_DELETE(pInfo);
  }

  RunNextUpdateTask();

  if (bTopLevel && !m_TransformState[plAssetInfo::TransformState::NeedsImport].IsEmpty())
  {
    const plUuid assetToImport = *m_TransformState[plAssetInfo::TransformState::NeedsImport].GetIterator();

    plAssetInfo* pInfo = GetAssetInfo(assetToImport);

    ProcessAsset(pInfo, nullptr, plTransformFlags::TriggeredManually);
    UpdateAssetTransformState(assetToImport, plAssetInfo::TransformState::Unknown);
  }

  if (bTopLevel && m_pAssetTableWriter)
    m_pAssetTableWriter->MainThreadTick();

  bReentry = false;
}

plDateTime plAssetCurator::GetLastFullTransformDate() const
{
  plStringBuilder path = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  path.AppendPath("LastFullTransform.date");

  plFileStats stat;
  if (plOSFile::GetFileStats(path, stat).Failed())
    return {};

  return plDateTime::MakeFromTimestamp(stat.m_LastModificationTime);
}

void plAssetCurator::StoreFullTransformDate()
{
  plStringBuilder path = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  path.AppendPath("LastFullTransform.date");

  plOSFile file;
  if (file.Open(path, plFileOpenMode::Write).Succeeded())
  {
    plDateTime date;
    date.SetFromTimestamp(plTimestamp::CurrentTimestamp()).AssertSuccess();

    path.Format("{}", date);
    file.Write(path.GetData(), path.GetElementCount()).AssertSuccess();
  }
}

////////////////////////////////////////////////////////////////////////
// plAssetCurator High Level Functions
////////////////////////////////////////////////////////////////////////

plStatus plAssetCurator::TransformAllAssets(plBitflags<plTransformFlags> transformFlags, const plPlatformProfile* pAssetProfile)
{
  PLASMA_PROFILE_SCOPE("TransformAllAssets");

  plDynamicArray<plUuid> assets;
  {
    PLASMA_LOCK(m_CuratorMutex);
    assets.Reserve(m_KnownAssets.GetCount());
    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      assets.PushBack(it.Key());
    }
  }
  plUInt32 uiNumStepsLeft = assets.GetCount();

  plUInt32 uiNumFailedSteps = 0;
  plProgressRange range("Transforming Assets", 1 + uiNumStepsLeft, true);
  for (const plUuid& assetGuid : assets)
  {
    if (range.WasCanceled())
      break;

    PLASMA_LOCK(m_CuratorMutex);

    plAssetInfo* pAssetInfo = nullptr;
    if (!m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
      continue;

    if (uiNumStepsLeft > 0)
    {
      // it can happen that the number of known assets changes while we are processing them
      // in this case the progress bar may assert that the number of steps completed is larger than
      // what was specified before
      // since this is a valid case, we just stop updating the progress bar, in case more assets are detected

      range.BeginNextStep(plPathUtils::GetFileNameAndExtension(pAssetInfo->m_Path.GetDataDirParentRelativePath()));
      --uiNumStepsLeft;
    }

    plTransformStatus res = ProcessAsset(pAssetInfo, pAssetProfile, transformFlags);
    if (res.Failed())
    {
      uiNumFailedSteps++;
      plLog::Error("{0} ({1})", res.m_sMessage, pAssetInfo->m_Path.GetDataDirParentRelativePath());
    }
  }

  TransformAssetsForSceneExport(pAssetProfile);

  range.BeginNextStep("Writing Lookup Tables");

  WriteAssetTables(pAssetProfile).IgnoreResult();

  StoreFullTransformDate();

  if (uiNumFailedSteps > 0)
    return plStatus(plFmt("Transform all assets failed on {0} assets.", uiNumFailedSteps));

  return plStatus(PLASMA_SUCCESS);
}

void plAssetCurator::ResaveAllAssets()
{
  plProgressRange range("Re-saving all Assets", 1 + m_KnownAssets.GetCount(), true);

  PLASMA_LOCK(m_CuratorMutex);

  plDynamicArray<plUuid> sortedAssets;
  sortedAssets.Reserve(m_KnownAssets.GetCount());

  plMap<plUuid, plSet<plUuid>> dependencies;

  plSet<plUuid> accu;

  for (auto itAsset = m_KnownAssets.GetIterator(); itAsset.IsValid(); ++itAsset)
  {
    auto it2 = dependencies.Insert(itAsset.Key(), plSet<plUuid>());
    for (const plString& dep : itAsset.Value()->m_Info->m_TransformDependencies)
    {
      if (plConversionUtils::IsStringUuid(dep))
      {
        it2.Value().Insert(plConversionUtils::ConvertStringToUuid(dep));
      }
    }
  }

  while (!dependencies.IsEmpty())
  {
    bool bDeadEnd = true;
    for (auto it = dependencies.GetIterator(); it.IsValid(); ++it)
    {
      // Are the types dependencies met?
      if (accu.ContainsSet(it.Value()))
      {
        sortedAssets.PushBack(it.Key());
        accu.Insert(it.Key());
        dependencies.Remove(it);
        bDeadEnd = false;
        break;
      }
    }

    if (bDeadEnd)
    {
      // Just take the next one in and hope for the best.
      auto it = dependencies.GetIterator();
      sortedAssets.PushBack(it.Key());
      accu.Insert(it.Key());
      dependencies.Remove(it);
    }
  }

  for (plUInt32 i = 0; i < sortedAssets.GetCount(); i++)
  {
    if (range.WasCanceled())
      break;

    plAssetInfo* pAssetInfo = GetAssetInfo(sortedAssets[i]);
    PLASMA_ASSERT_DEBUG(pAssetInfo, "Should not happen as data was derived from known assets list.");
    range.BeginNextStep(plPathUtils::GetFileNameAndExtension(pAssetInfo->m_Path.GetDataDirParentRelativePath()));

    auto res = ResaveAsset(pAssetInfo);
    if (res.m_Result.Failed())
    {
      plLog::Error("{0} ({1})", res.m_sMessage, pAssetInfo->m_Path.GetDataDirParentRelativePath());
    }
  }
}

plTransformStatus plAssetCurator::TransformAsset(const plUuid& assetGuid, plBitflags<plTransformFlags> transformFlags, const plPlatformProfile* pAssetProfile)
{
  plTransformStatus res;
  plStringBuilder sAbsPath;
  plStopwatch timer;
  const plAssetDocumentTypeDescriptor* pTypeDesc = nullptr;
  {
    PLASMA_LOCK(m_CuratorMutex);

    plAssetInfo* pInfo = nullptr;
    if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
      return plTransformStatus("Transform failed, unknown asset.");

    sAbsPath = pInfo->m_Path;
    res = ProcessAsset(pInfo, pAssetProfile, transformFlags);
  }
  if (pTypeDesc && transformFlags.IsAnySet(plTransformFlags::TriggeredManually))
  {
    // As this is triggered manually it is safe to save here as these are only run on the main thread.
    if (plDocument* pDoc = pTypeDesc->m_pManager->GetDocumentByPath(sAbsPath))
    {
      // some assets modify the document during transformation
      // make sure the state is saved, at least when the user actively executed the action
      pDoc->SaveDocument().LogFailure();
    }
  }
  plLog::Info("Transform asset time: {0}s", plArgF(timer.GetRunningTotal().GetSeconds(), 2));
  return res;
}

plTransformStatus plAssetCurator::CreateThumbnail(const plUuid& assetGuid)
{
  PLASMA_LOCK(m_CuratorMutex);

  plAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
    return plStatus("Create thumbnail failed, unknown asset.");

  return ProcessAsset(pInfo, nullptr, plTransformFlags::None);
}

void plAssetCurator::TransformAssetsForSceneExport(const plPlatformProfile* pAssetProfile /*= nullptr*/)
{
  PLASMA_PROFILE_SCOPE("Transform Special Assets");

  plSet<plTempHashedString> types;

  {
    auto& allDMs = plDocumentManager::GetAllDocumentManagers();
    for (auto& dm : allDMs)
    {
      if (plAssetDocumentManager* pADM = plDynamicCast<plAssetDocumentManager*>(dm))
      {
        pADM->GetAssetTypesRequiringTransformForSceneExport(types);
      }
    }
  }

  plSet<plUuid> assets;
  {
    plAssetCurator::plLockedAssetTable allAssets = GetKnownAssets();

    for (auto it : *allAssets)
    {
      if (types.Contains(it.Value()->m_Info->m_sAssetsDocumentTypeName))
      {
        assets.Insert(it.Value()->m_Info->m_DocumentID);
      }
    }
  }

  for (const auto& guid : assets)
  {
    // Ignore result
    TransformAsset(guid, plTransformFlags::TriggeredManually | plTransformFlags::ForceTransform, pAssetProfile);
  }
}

plResult plAssetCurator::WriteAssetTables(const plPlatformProfile* pAssetProfile, bool bForce)
{
  CURATOR_PROFILE("WriteAssetTables");
  PLASMA_LOG_BLOCK("plAssetCurator::WriteAssetTables");

  if (pAssetProfile == nullptr)
  {
    pAssetProfile = GetActiveAssetProfile();
  }

  return m_pAssetTableWriter->WriteAssetTables(pAssetProfile, bForce);
}


////////////////////////////////////////////////////////////////////////
// plAssetCurator Asset Access
////////////////////////////////////////////////////////////////////////

const plAssetCurator::plLockedSubAsset plAssetCurator::FindSubAsset(plStringView sPathOrGuid, bool bExhaustiveSearch) const
{
  CURATOR_PROFILE("FindSubAsset");
  PLASMA_LOCK(m_CuratorMutex);

  if (plConversionUtils::IsStringUuid(sPathOrGuid))
  {
    return GetSubAsset(plConversionUtils::ConvertStringToUuid(sPathOrGuid));
  }

  // Split into mainAsset|subAsset
  plStringBuilder mainAsset;
  plStringView subAsset;
  const char* szSeparator = sPathOrGuid.FindSubString("|");
  if (szSeparator != nullptr)
  {
    mainAsset.SetSubString_FromTo(sPathOrGuid.GetStartPointer(), szSeparator);
    subAsset = plStringView(szSeparator + 1);
  }
  else
  {
    mainAsset = sPathOrGuid;
  }
  mainAsset.MakeCleanPath();

  // Find mainAsset
  plFileStatus stat;
  plResult res = plFileSystemModel::GetSingleton()->FindFile(mainAsset, stat);

  // Did we find an asset?
  if (res == PLASMA_SUCCESS && stat.m_DocumentID.IsValid())
  {
    plAssetInfo* pAssetInfo = nullptr;
    m_KnownAssets.TryGetValue(stat.m_DocumentID, pAssetInfo);
    PLASMA_ASSERT_DEV(pAssetInfo != nullptr, "Files reference non-existant assset!");

    if (subAsset.IsValid())
    {
      for (const plUuid& sub : pAssetInfo->m_SubAssets)
      {
        auto itSub = m_KnownSubAssets.Find(sub);
        if (itSub.IsValid() && subAsset.IsEqual_NoCase(itSub.Value().GetName()))
        {
          return plLockedSubAsset(m_CuratorMutex, &itSub.Value());
        }
      }
    }
    else
    {
      auto itSub = m_KnownSubAssets.Find(pAssetInfo->m_Info->m_DocumentID);
      return plLockedSubAsset(m_CuratorMutex, &itSub.Value());
    }
  }

  if (!bExhaustiveSearch)
    return plLockedSubAsset();

  // TODO: This is the old slow code path that will find the longest substring match.
  // Should be removed or folded into FindBestMatchForFile once it's surely not needed anymore.

  auto FindAsset = [this](plStringView sPathView) -> plAssetInfo*
  {
    // try to find the 'exact' relative path
    // otherwise find the shortest possible path
    plUInt32 uiMinLength = 0xFFFFFFFF;
    plAssetInfo* pBestInfo = nullptr;

    if (sPathView.IsEmpty())
      return nullptr;

    const plStringBuilder sPath = sPathView;
    const plStringBuilder sPathWithSlash("/", sPath);

    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->m_Path.GetDataDirParentRelativePath().EndsWith_NoCase(sPath))
      {
        // endswith -> could also be equal
        if (sPathView.IsEqual_NoCase(it.Value()->m_Path.GetDataDirParentRelativePath()))
        {
          // if equal, just take it
          return it.Value();
        }

        // need to check again with a slash to make sure we don't return something that is of an invalid type
        // this can happen where the user is allowed to type random paths
        if (it.Value()->m_Path.GetDataDirParentRelativePath().EndsWith_NoCase(sPathWithSlash))
        {
          const plUInt32 uiLength = it.Value()->m_Path.GetDataDirParentRelativePath().GetElementCount();
          if (uiLength < uiMinLength)
          {
            uiMinLength = uiLength;
            pBestInfo = it.Value();
          }
        }
      }
    }

    return pBestInfo;
  };

  szSeparator = sPathOrGuid.FindSubString("|");
  if (szSeparator != nullptr)
  {
    plStringBuilder mainAsset2;
    mainAsset2.SetSubString_FromTo(sPathOrGuid.GetStartPointer(), szSeparator);

    plStringView subAsset2(szSeparator + 1);
    if (plAssetInfo* pAssetInfo = FindAsset(mainAsset2))
    {
      for (const plUuid& sub : pAssetInfo->m_SubAssets)
      {
        auto subIt = m_KnownSubAssets.Find(sub);
        if (subIt.IsValid() && subAsset2.IsEqual_NoCase(subIt.Value().GetName()))
        {
          return plLockedSubAsset(m_CuratorMutex, &subIt.Value());
        }
      }
    }
  }

  plStringBuilder sPath = sPathOrGuid;
  sPath.MakeCleanPath();
  if (sPath.IsAbsolutePath())
  {
    if (!plQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sPath))
      return plLockedSubAsset();
  }

  if (plAssetInfo* pAssetInfo = FindAsset(sPath))
  {
    auto itSub = m_KnownSubAssets.Find(pAssetInfo->m_Info->m_DocumentID);
    return plLockedSubAsset(m_CuratorMutex, &itSub.Value());
  }
  return plLockedSubAsset();
}

const plAssetCurator::plLockedSubAsset plAssetCurator::GetSubAsset(const plUuid& assetGuid) const
{
  PLASMA_LOCK(m_CuratorMutex);

  auto it = m_KnownSubAssets.Find(assetGuid);
  if (it.IsValid())
  {
    const plSubAsset* pAssetInfo = &(it.Value());
    return plLockedSubAsset(m_CuratorMutex, pAssetInfo);
  }
  return plLockedSubAsset();
}

const plAssetCurator::plLockedSubAssetTable plAssetCurator::GetKnownSubAssets() const
{
  return plLockedSubAssetTable(m_CuratorMutex, &m_KnownSubAssets);
}

const plAssetCurator::plLockedAssetTable plAssetCurator::GetKnownAssets() const
{
  return plLockedAssetTable(m_CuratorMutex, &m_KnownAssets);
}

plUInt64 plAssetCurator::GetAssetDependencyHash(plUuid assetGuid)
{
  plUInt64 assetHash = 0;
  plUInt64 thumbHash = 0;
  plAssetCurator::UpdateAssetTransformState(assetGuid, assetHash, thumbHash, false);
  return assetHash;
}

plUInt64 plAssetCurator::GetAssetReferenceHash(plUuid assetGuid)
{
  plUInt64 assetHash = 0;
  plUInt64 thumbHash = 0;
  plAssetCurator::UpdateAssetTransformState(assetGuid, assetHash, thumbHash, false);
  return thumbHash;
}

plAssetInfo::TransformState plAssetCurator::IsAssetUpToDate(const plUuid& assetGuid, const plPlatformProfile*, const plAssetDocumentTypeDescriptor* pTypeDescriptor, plUInt64& out_uiAssetHash, plUInt64& out_uiThumbHash, bool bForce)
{
  return plAssetCurator::UpdateAssetTransformState(assetGuid, out_uiAssetHash, out_uiThumbHash, bForce);
}

void plAssetCurator::InvalidateAssetsWithTransformState(plAssetInfo::TransformState state)
{
  PLASMA_LOCK(m_CuratorMutex);

  plHashSet<plUuid> allWithState = m_TransformState[state];

  for (const auto& asset : allWithState)
  {
    InvalidateAssetTransformState(asset);
  }
}

plAssetInfo::TransformState plAssetCurator::UpdateAssetTransformState(plUuid assetGuid, plUInt64& out_AssetHash, plUInt64& out_ThumbHash, bool bForce)
{
  CURATOR_PROFILE("UpdateAssetTransformState");
  plStringBuilder sAbsAssetPath;
  {
    PLASMA_LOCK(m_CuratorMutex);
    // If assetGuid is a sub-asset, redirect to main asset.
    auto it = m_KnownSubAssets.Find(assetGuid);
    if (!it.IsValid())
    {
      return plAssetInfo::Unknown;
    }
    plAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;
    assetGuid = pAssetInfo->m_Info->m_DocumentID;
    sAbsAssetPath = pAssetInfo->m_Path;

    // Circular dependencies can change if any asset in the circle has changed (and potentially broken the circle). Thus, we need to call CheckForCircularDependencies again for every asset.
    if (!pAssetInfo->m_CircularDependencies.IsEmpty() && m_TransformStateStale.Contains(assetGuid))
    {
      pAssetInfo->m_CircularDependencies.Clear();
      if (CheckForCircularDependencies(pAssetInfo).Failed())
      {
        UpdateAssetTransformState(assetGuid, plAssetInfo::CircularDependency);
        out_AssetHash = 0;
        out_ThumbHash = 0;
        return plAssetInfo::CircularDependency;
      }
    }

    // Setting an asset to unknown actually does not change the m_TransformState but merely adds it to the m_TransformStateStale list.
    // This is to prevent the user facing state to constantly fluctuate if something is tagged as modified but not actually changed (E.g. saving a
    // file without modifying the content). Thus we need to check for m_TransformStateStale as well as for the set state.
    if (!bForce && pAssetInfo->m_TransformState != plAssetInfo::Unknown && !m_TransformStateStale.Contains(assetGuid))
    {
      out_AssetHash = pAssetInfo->m_AssetHash;
      out_ThumbHash = pAssetInfo->m_ThumbHash;
      return pAssetInfo->m_TransformState;
    }
  }

  plFileSystemModel::GetSingleton()->NotifyOfChange(sAbsAssetPath);

  // Data to pull from the asset under the lock that is needed for update computation.
  plAssetDocumentManager* pManager = nullptr;
  const plAssetDocumentTypeDescriptor* pTypeDescriptor = nullptr;
  plString sAssetFile;
  plUInt8 uiLastStateUpdate = 0;
  plUInt64 uiSettingsHash = 0;
  plHybridArray<plString, 16> transformDeps;
  plHybridArray<plString, 16> thumbnailDeps;
  plHybridArray<plString, 16> outputs;
  plHybridArray<plString, 16> subAssetNames;

  // Lock asset and get all data needed for update computation.
  {
    CURATOR_PROFILE("CopyAssetData");
    PLASMA_LOCK(m_CuratorMutex);
    plAssetInfo* pAssetInfo = GetAssetInfo(assetGuid);
    if (!pAssetInfo)
    {
      plStringBuilder tmp;
      plLog::Error("Asset with GUID {0} is unknown", plConversionUtils::ToString(assetGuid, tmp));
      return plAssetInfo::TransformState::Unknown;
    }
    pManager = pAssetInfo->GetManager();
    pTypeDescriptor = pAssetInfo->m_pDocumentTypeDescriptor;
    sAssetFile = pAssetInfo->m_Path;
    uiLastStateUpdate = pAssetInfo->m_LastStateUpdate;
    // The settings has combines both the file settings and the global profile settings.
    uiSettingsHash = pAssetInfo->m_Info->m_uiSettingsHash + pManager->GetAssetProfileHash();
    for (const plString& dep : pAssetInfo->m_Info->m_TransformDependencies)
    {
      transformDeps.PushBack(dep);
    }
    for (const plString& ref : pAssetInfo->m_Info->m_ThumbnailDependencies)
    {
      thumbnailDeps.PushBack(ref);
    }
    for (const plString& output : pAssetInfo->m_Info->m_Outputs)
    {
      outputs.PushBack(output);
    }
    for (auto& subAssetUuid : pAssetInfo->m_SubAssets)
    {
      if (plSubAsset* pSubAsset = GetSubAssetInternal(subAssetUuid))
      {
        subAssetNames.PushBack(pSubAsset->m_Data.m_sName);
      }
    }
  }

  plAssetInfo::TransformState state = plAssetInfo::TransformState::Unknown;
  plSet<plString> missingTransformDeps;
  plSet<plString> missingThumbnailDeps;
  // Compute final state and hashes.
  {
    state = HashAsset(uiSettingsHash, transformDeps, thumbnailDeps, missingTransformDeps, missingThumbnailDeps, out_AssetHash, out_ThumbHash, bForce);
    PLASMA_ASSERT_DEV(state == plAssetInfo::Unknown || state == plAssetInfo::MissingTransformDependency || state == plAssetInfo::MissingThumbnailDependency, "Unhandled case of HashAsset return value.");

    if (state == plAssetInfo::Unknown)
    {
      if (pManager->IsOutputUpToDate(sAssetFile, outputs, out_AssetHash, pTypeDescriptor))
      {
        state = plAssetInfo::TransformState::UpToDate;
        if (pTypeDescriptor->m_AssetDocumentFlags.IsAnySet(plAssetDocumentFlags::SupportsThumbnail | plAssetDocumentFlags::AutoThumbnailOnTransform))
        {
          if (!pManager->IsThumbnailUpToDate(sAssetFile, "", out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
          {
            state = pTypeDescriptor->m_AssetDocumentFlags.IsSet(plAssetDocumentFlags::AutoThumbnailOnTransform) ? plAssetInfo::TransformState::NeedsTransform : plAssetInfo::TransformState::NeedsThumbnail;
          }
        }
        else if (pTypeDescriptor->m_AssetDocumentFlags.IsAnySet(plAssetDocumentFlags::SubAssetsSupportThumbnail | plAssetDocumentFlags::SubAssetsAutoThumbnailOnTransform))
        {
          for (const plString& subAssetName : subAssetNames)
          {
            if (!pManager->IsThumbnailUpToDate(sAssetFile, subAssetName, out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
            {
              state = pTypeDescriptor->m_AssetDocumentFlags.IsSet(plAssetDocumentFlags::SubAssetsAutoThumbnailOnTransform) ? plAssetInfo::TransformState::NeedsTransform : plAssetInfo::TransformState::NeedsThumbnail;
              break;
            }
          }
        }
      }
      else
      {
        state = plAssetInfo::TransformState::NeedsTransform;
      }
    }
  }

  {
    PLASMA_LOCK(m_CuratorMutex);
    plAssetInfo* pAssetInfo = GetAssetInfo(assetGuid);
    if (pAssetInfo)
    {
      // Only update the state if the asset state remains unchanged since we gathered its data.
      // Otherwise the state we computed would already be stale. Return the data regardless
      // instead of waiting for a new computation as the case in which the value has actually changed
      // is very rare (asset modified between the two locks) in which case we will just create
      // an already stale transform / thumbnail which will be immediately replaced again.
      if (pAssetInfo->m_LastStateUpdate == uiLastStateUpdate)
      {
        UpdateAssetTransformState(assetGuid, state);
        pAssetInfo->m_AssetHash = out_AssetHash;
        pAssetInfo->m_ThumbHash = out_ThumbHash;
        pAssetInfo->m_MissingTransformDeps = std::move(missingTransformDeps);
        pAssetInfo->m_MissingThumbnailDeps = std::move(missingThumbnailDeps);
        if (state == plAssetInfo::TransformState::UpToDate)
        {
          UpdateSubAssets(*pAssetInfo);
        }
      }
    }
    else
    {
      plStringBuilder tmp;
      plLog::Error("Asset with GUID {0} is unknown", plConversionUtils::ToString(assetGuid, tmp));
      return plAssetInfo::TransformState::Unknown;
    }
    return state;
  }
}

void plAssetCurator::GetAssetTransformStats(plUInt32& out_uiNumAssets, plHybridArray<plUInt32, plAssetInfo::TransformState::COUNT>& out_count)
{
  PLASMA_LOCK(m_CuratorMutex);
  out_count.SetCountUninitialized(plAssetInfo::TransformState::COUNT);
  for (int i = 0; i < plAssetInfo::TransformState::COUNT; i++)
  {
    out_count[i] = m_TransformState[i].GetCount();
  }

  out_uiNumAssets = m_KnownAssets.GetCount();
}

plString plAssetCurator::FindDataDirectoryForAsset(plStringView sAbsoluteAssetPath) const
{
  plStringBuilder sAssetPath(sAbsoluteAssetPath);

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    plStringBuilder sDataDir;
    plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    if (sAssetPath.IsPathBelowFolder(sDataDir))
      return sDataDir;
  }

  PLASMA_REPORT_FAILURE("Could not find data directory for asset '{0}", sAbsoluteAssetPath);
  return plFileSystem::GetSdkRootDirectory();
}

plResult plAssetCurator::FindBestMatchForFile(plStringBuilder& ref_sFile, plArrayPtr<plString> allowedFileExtensions) const
{
  // TODO: Merge with exhaustive search in FindSubAsset
  ref_sFile.MakeCleanPath();

  plStringBuilder testName = ref_sFile;

  for (const auto& ext : allowedFileExtensions)
  {
    testName.ChangeFileExtension(ext);

    if (plFileSystem::ExistsFile(testName))
    {
      ref_sFile = testName;
      goto found;
    }
  }

  testName = ref_sFile.GetFileNameAndExtension();

  if (testName.IsEmpty())
  {
    ref_sFile = "";
    return PLASMA_FAILURE;
  }

  if (plPathUtils::ContainsInvalidFilenameChars(testName))
  {
    // not much we can do here, if the filename is already invalid, we will probably not find it in out known files list

    plPathUtils::MakeValidFilename(testName, '_', ref_sFile);
    return PLASMA_FAILURE;
  }

  {
    PLASMA_LOCK(m_CuratorMutex);

    auto SearchFile = [this](plStringBuilder& ref_sName) -> bool
    {
      return plFileSystemModel::GetSingleton()->FindFile([&ref_sName](const plDataDirPath& file, const plFileStatus& stat)
                                                {
                                                  if (stat.m_Status != plFileStatus::Status::Valid)
                                                    return false;

                                                  if (file.GetAbsolutePath().EndsWith_NoCase(ref_sName))
                                                  {
                                                    ref_sName = file.GetAbsolutePath();
                                                    return true;
                                                  }
                                                  return false; //
                                                })
        .Succeeded();
    };

    // search for the full name
    {
      testName.Prepend("/"); // make sure to not find partial names

      for (const auto& ext : allowedFileExtensions)
      {
        testName.ChangeFileExtension(ext);

        if (SearchFile(testName))
          goto found;
      }
    }

    return PLASMA_FAILURE;
  }

found:
  if (plQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(testName))
  {
    ref_sFile = testName;
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

void plAssetCurator::FindAllUses(plUuid assetGuid, plSet<plUuid>& ref_uses, bool bTransitive) const
{
  PLASMA_LOCK(m_CuratorMutex);

  plSet<plUuid> todoList;
  todoList.Insert(assetGuid);

  auto GatherReferences = [&](const plMap<plString, plHybridArray<plUuid, 1>>& inverseTracker, const plStringBuilder& sAsset)
  {
    auto it = inverseTracker.Find(sAsset);
    if (it.IsValid())
    {
      for (const plUuid& guid : it.Value())
      {
        if (!ref_uses.Contains(guid))
          todoList.Insert(guid);

        ref_uses.Insert(guid);
      }
    }
  };

  plStringBuilder sCurrentAsset;
  do
  {
    auto itFirst = todoList.GetIterator();
    const plAssetInfo* pInfo = GetAssetInfo(itFirst.Key());
    todoList.Remove(itFirst);

    if (pInfo)
    {
      sCurrentAsset = pInfo->m_Path;
      GatherReferences(m_InverseThumbnailDeps, sCurrentAsset);
      GatherReferences(m_InverseTransformDeps, sCurrentAsset);
    }
  } while (bTransitive && !todoList.IsEmpty());
}

void plAssetCurator::FindAllUses(plStringView sAbsolutePath, plSet<plUuid>& ref_uses) const
{
  PLASMA_LOCK(m_CuratorMutex);
  if (auto it = m_InverseTransformDeps.Find(sAbsolutePath); it.IsValid())
  {
    for (const plUuid& guid : it.Value())
    {
      ref_uses.Insert(guid);
    }
  }
}

bool plAssetCurator::IsReferenced(plStringView sAbsolutePath) const
{
  PLASMA_LOCK(m_CuratorMutex);
  auto it = m_InverseTransformDeps.Find(sAbsolutePath);
  return it.IsValid() && !it.Value().IsEmpty();
}

////////////////////////////////////////////////////////////////////////
// plAssetCurator Manual and Automatic Change Notification
////////////////////////////////////////////////////////////////////////

void plAssetCurator::NotifyOfFileChange(plStringView sAbsolutePath)
{
  plStringBuilder sPath(sAbsolutePath);
  sPath.MakeCleanPath();
  plFileSystemModel::GetSingleton()->NotifyOfChange(sPath);
}

void plAssetCurator::NotifyOfAssetChange(const plUuid& assetGuid)
{
  InvalidateAssetTransformState(assetGuid);
}

void plAssetCurator::UpdateAssetLastAccessTime(const plUuid& assetGuid)
{
  auto it = m_KnownSubAssets.Find(assetGuid);

  if (!it.IsValid())
    return;

  it.Value().m_LastAccess = plTime::Now();
}

void plAssetCurator::CheckFileSystem()
{
  PLASMA_PROFILE_SCOPE("CheckFileSystem");
  plStopwatch sw;

  // make sure the hashing task has finished
  ShutdownUpdateTask();

  {
    PLASMA_LOCK(m_CuratorMutex);
    SetAllAssetStatusUnknown();
  }
  plFileSystemModel::GetSingleton()->CheckFileSystem();

  if (plThreadUtils::IsMainThread())
  {
    // Broadcast reset only if we are on the main thread.
    // Otherwise we are on the init task thread and the reset will be called on the main thread by WaitForInitialize.
    plAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type = plAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }

  RestartUpdateTask();

  plLog::Debug("Asset Curator Refresh Time: {0} ms", plArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

void plAssetCurator::NeedsReloadResources(const plUuid& assetGuid)
{
  if (m_pAssetTableWriter)
  {
    m_pAssetTableWriter->NeedsReloadResource(assetGuid);

    plAssetInfo* pAssetInfo = nullptr;
    if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    {
      for (auto& subAssetUuid : pAssetInfo->m_SubAssets)
      {
        m_pAssetTableWriter->NeedsReloadResource(subAssetUuid);
      }
    }
  }
}

void plAssetCurator::GenerateTransitiveHull(const plStringView sAssetOrPath, plSet<plString>& inout_deps, bool bIncludeTransformDeps, bool bIncludeThumbnailDeps, bool bIncludePackageDeps) const
{
  PLASMA_LOCK(m_CuratorMutex);

  plHybridArray<plString, 6> toDoList;
  inout_deps.Insert(sAssetOrPath);
  toDoList.PushBack(sAssetOrPath);

  while (!toDoList.IsEmpty())
  {
    plString currentAsset = toDoList.PeekBack();
    toDoList.PopBack();

    if (plConversionUtils::IsStringUuid(currentAsset))
    {
      auto it = m_KnownSubAssets.Find(plConversionUtils::ConvertStringToUuid(currentAsset));
      plAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;

      if (bIncludeTransformDeps)
      {
        for (const plString& dep : pAssetInfo->m_Info->m_TransformDependencies)
        {
          if (!inout_deps.Contains(dep))
          {
            inout_deps.Insert(dep);
            toDoList.PushBack(dep);
          }
        }
      }
      if (bIncludeThumbnailDeps)
      {
        for (const plString& dep : pAssetInfo->m_Info->m_ThumbnailDependencies)
        {
          if (!inout_deps.Contains(dep))
          {
            inout_deps.Insert(dep);
            toDoList.PushBack(dep);
          }
        }
      }
      if (bIncludePackageDeps)
      {
        for (const plString& dep : pAssetInfo->m_Info->m_PackageDependencies)
        {
          if (!inout_deps.Contains(dep))
          {
            inout_deps.Insert(dep);
            toDoList.PushBack(dep);
          }
        }
      }
    }
  }
}

void plAssetCurator::GenerateInverseTransitiveHull(const plAssetInfo* pAssetInfo, plSet<plUuid>& inout_inverseDeps, bool bIncludeTransformDebs, bool bIncludeThumbnailDebs) const
{
  PLASMA_LOCK(m_CuratorMutex);

  plHybridArray<const plAssetInfo*, 6> toDoList;
  toDoList.PushBack(pAssetInfo);
  inout_inverseDeps.Insert(pAssetInfo->m_Info->m_DocumentID);

  while (!toDoList.IsEmpty())
  {
    const plAssetInfo* currentAsset = toDoList.PeekBack();
    toDoList.PopBack();

    if (bIncludeTransformDebs)
    {
      if (auto it = m_InverseTransformDeps.Find(currentAsset->m_Path.GetAbsolutePath()); it.IsValid())
      {
        for (const plUuid& asset : it.Value())
        {
          if (!inout_inverseDeps.Contains(asset))
          {
            plAssetInfo* pAssetInfo = nullptr;
            if (m_KnownAssets.TryGetValue(asset, pAssetInfo))
            {
              toDoList.PushBack(pAssetInfo);
              inout_inverseDeps.Insert(asset);
            }
          }
        }
      }
    }

    if (bIncludeThumbnailDebs)
    {
      if (auto it = m_InverseThumbnailDeps.Find(currentAsset->m_Path.GetAbsolutePath()); it.IsValid())
      {
        for (const plUuid& asset : it.Value())
        {
          if (!inout_inverseDeps.Contains(asset))
          {
            plAssetInfo* pAssetInfo = nullptr;
            if (m_KnownAssets.TryGetValue(asset, pAssetInfo))
            {
              toDoList.PushBack(pAssetInfo);
              inout_inverseDeps.Insert(asset);
            }
          }
        }
      }
    }
  }
}

void plAssetCurator::WriteDependencyDGML(const plUuid& guid, plStringView sOutputFile) const
{
  PLASMA_LOCK(m_CuratorMutex);

  plDGMLGraph graph;

  plSet<plString> deps;
  plStringBuilder sTemp;
  GenerateTransitiveHull(plConversionUtils::ToString(guid, sTemp), deps, true, true);

  plHashTable<plString, plUInt32> nodeMap;
  nodeMap.Reserve(deps.GetCount());
  for (auto& dep : deps)
  {
    plDGMLGraph::NodeDesc nd;
    if (plConversionUtils::IsStringUuid(dep))
    {
      auto it = m_KnownSubAssets.Find(plConversionUtils::ConvertStringToUuid(dep));
      const plSubAsset& subAsset = it.Value();
      const plAssetInfo* pAssetInfo = subAsset.m_pAssetInfo;
      if (subAsset.m_bMainAsset)
      {
        nd.m_Color = plColor::Blue;
        sTemp.Format("{}", pAssetInfo->m_Path.GetDataDirParentRelativePath());
      }
      else
      {
        nd.m_Color = plColor::AliceBlue;
        sTemp.Format("{} | {}", pAssetInfo->m_Path.GetDataDirParentRelativePath(), subAsset.GetName());
      }
      nd.m_Shape = plDGMLGraph::NodeShape::Rectangle;
    }
    else
    {
      sTemp = dep;
      nd.m_Color = plColor::Orange;
      nd.m_Shape = plDGMLGraph::NodeShape::Rectangle;
    }
    plUInt32 uiGraphNode = graph.AddNode(sTemp, &nd);
    nodeMap.Insert(dep, uiGraphNode);
  }

  for (auto& node : deps)
  {
    plDGMLGraph::NodeDesc nd;
    if (plConversionUtils::IsStringUuid(node))
    {
      plUInt32 uiInputNode = *nodeMap.GetValue(node);

      auto it = m_KnownSubAssets.Find(plConversionUtils::ConvertStringToUuid(node));
      plAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;

      plMap<plUInt32, plString> connection;

      auto ExtendConnection = [&](const plString& sRef, plStringView sLabel)
      {
        plUInt32 uiOutputNode = *nodeMap.GetValue(sRef);
        sTemp = connection[uiOutputNode];
        if (sTemp.IsEmpty())
          sTemp = sLabel;
        else
          sTemp.AppendFormat(" | {}", sLabel);
        connection[uiOutputNode] = sTemp;
      };

      for (const plString& sRef : pAssetInfo->m_Info->m_TransformDependencies)
      {
        ExtendConnection(sRef, "Transform");
      }

      for (const plString& sRef : pAssetInfo->m_Info->m_ThumbnailDependencies)
      {
        ExtendConnection(sRef, "Thumbnail");
      }

      // This will make the graph very big, not recommended.
      /* for (const plString& ref : pAssetInfo->m_Info->m_PackageDependencies)
       {
         ExtendConnection(ref, "Package");
       }*/

      for (auto it : connection)
      {
        graph.AddConnection(uiInputNode, it.Key(), it.Value());
      }
    }
  }

  plDGMLGraphWriter::WriteGraphToFile(sOutputFile, graph).IgnoreResult();
}

////////////////////////////////////////////////////////////////////////
// plAssetCurator Processing
////////////////////////////////////////////////////////////////////////

plCommandLineOptionEnum opt_AssetThumbnails("_Editor", "-AssetThumbnails", "Whether to generate thumbnails for transformed assets.", "default = 0 | never = 1", 0);

plTransformStatus plAssetCurator::ProcessAsset(plAssetInfo* pAssetInfo, const plPlatformProfile* pAssetProfile, plBitflags<plTransformFlags> transformFlags)
{
  if (transformFlags.IsSet(plTransformFlags::ForceTransform))
    plLog::Dev("Asset transform forced.");

  const plAssetDocumentTypeDescriptor* pTypeDesc = pAssetInfo->m_pDocumentTypeDescriptor;
  plUInt64 uiHash = 0;
  plUInt64 uiThumbHash = 0;
  plAssetInfo::TransformState state = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash, uiThumbHash);

  if (state == plAssetInfo::TransformState::CircularDependency)
  {
    return plTransformStatus(plFmt("Circular dependency for asset '{0}', can't transform.", pAssetInfo->m_Path.GetAbsolutePath()));
  }

  for (const auto& dep : pAssetInfo->m_Info->m_TransformDependencies)
  {
    plBitflags<plTransformFlags> transformFlagsDeps = transformFlags;
    transformFlagsDeps.Remove(plTransformFlags::ForceTransform);
    if (plAssetInfo* pInfo = GetAssetInfo(dep))
    {
      PLASMA_SUCCEED_OR_RETURN(ProcessAsset(pInfo, pAssetProfile, transformFlagsDeps));
    }
  }

  plTransformStatus resReferences;
  for (const auto& ref : pAssetInfo->m_Info->m_ThumbnailDependencies)
  {
    plBitflags<plTransformFlags> transformFlagsRefs = transformFlags;
    transformFlagsRefs.Remove(plTransformFlags::ForceTransform);
    if (plAssetInfo* pInfo = GetAssetInfo(ref))
    {
      resReferences = ProcessAsset(pInfo, pAssetProfile, transformFlagsRefs);
      if (resReferences.Failed())
        break;
    }
  }


  PLASMA_ASSERT_DEV(pTypeDesc->m_pDocumentType->IsDerivedFrom<plAssetDocument>(), "Asset document does not derive from correct base class ('{0}')", pAssetInfo->m_Path.GetDataDirParentRelativePath());

  auto assetFlags = pTypeDesc->m_AssetDocumentFlags;

  // Skip assets that cannot be auto-transformed.
  {
    if (assetFlags.IsAnySet(plAssetDocumentFlags::DisableTransform))
      return plStatus(PLASMA_SUCCESS);

    if (!transformFlags.IsSet(plTransformFlags::TriggeredManually) && assetFlags.IsAnySet(plAssetDocumentFlags::OnlyTransformManually))
      return plStatus(PLASMA_SUCCESS);
  }

  // If references are not complete and we generate thumbnails on transform we can cancel right away.
  if (assetFlags.IsSet(plAssetDocumentFlags::AutoThumbnailOnTransform) && resReferences.Failed())
  {
    return resReferences;
  }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  {
    // Sanity check that transforming the dependencies did not change the asset's transform state.
    // In theory this can happen if an asset is transformed by multiple processes at the same time or changes to the file system are being made in the middle of the transform.
    // If this can be reproduced consistently, it is usually a bug in the dependency tracking or other part of the asset curator.
    plUInt64 uiHash2 = 0;
    plUInt64 uiThumbHash2 = 0;
    plAssetInfo::TransformState state2 = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash2, uiThumbHash2);

    if (uiHash != uiHash2)
      return plTransformStatus(plFmt("Asset hash changed while prosessing dependencies from {} to {}", uiHash, uiHash2));
    if (uiThumbHash != uiThumbHash2)
      return plTransformStatus(plFmt("Asset thumbnail hash changed while prosessing dependencies from {} to {}", uiThumbHash, uiThumbHash2));
    if (state != state2)
      return plTransformStatus(plFmt("Asset state changed while prosessing dependencies from {} to {}", state, state2));
  }
#endif

  if (transformFlags.IsSet(plTransformFlags::ForceTransform))
  {
    state = plAssetInfo::NeedsTransform;
  }

  if (state == plAssetInfo::TransformState::UpToDate)
    return plStatus(PLASMA_SUCCESS);

  if (state == plAssetInfo::TransformState::MissingTransformDependency)
  {
    return plTransformStatus(plFmt("Missing dependency for asset '{0}', can't transform.", pAssetInfo->m_Path.GetAbsolutePath()));
  }

  // does the document already exist and is open ?
  bool bWasOpen = false;
  plDocument* pDoc = pTypeDesc->m_pManager->GetDocumentByPath(pAssetInfo->m_Path);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = plQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_Path.GetAbsolutePath(), plDocumentFlags::None);

  if (pDoc == nullptr)
    return plTransformStatus(plFmt("Could not open asset document '{0}'", pAssetInfo->m_Path.GetDataDirParentRelativePath()));

  PLASMA_SCOPE_EXIT(if (!pDoc->HasWindowBeenRequested() && !bWasOpen) pDoc->GetDocumentManager()->CloseDocument(pDoc););

  plTransformStatus ret;
  plAssetDocument* pAsset = static_cast<plAssetDocument*>(pDoc);
  if (state == plAssetInfo::TransformState::NeedsTransform || (state == plAssetInfo::TransformState::NeedsThumbnail && assetFlags.IsSet(plAssetDocumentFlags::AutoThumbnailOnTransform)) || (transformFlags.IsSet(plTransformFlags::TriggeredManually) && state == plAssetInfo::TransformState::NeedsImport))
  {
    ret = pAsset->TransformAsset(transformFlags, pAssetProfile);
    if (ret.Succeeded())
    {
      m_pAssetTableWriter->NeedsReloadResource(pAsset->GetGuid());

      for (auto& subAssetUuid : pAssetInfo->m_SubAssets)
      {
        m_pAssetTableWriter->NeedsReloadResource(subAssetUuid);
      }
    }
  }

  if (state == plAssetInfo::TransformState::MissingThumbnailDependency)
  {
    return plTransformStatus(plFmt("Missing reference for asset '{0}', can't create thumbnail.", pAssetInfo->m_Path.GetAbsolutePath()));
  }

  if (opt_AssetThumbnails.GetOptionValue(plCommandLineOption::LogMode::FirstTimeIfSpecified) != 1)
  {
    // skip thumbnail generation, if disabled globally

    if (ret.Succeeded() && assetFlags.IsSet(plAssetDocumentFlags::SupportsThumbnail) && !assetFlags.IsSet(plAssetDocumentFlags::AutoThumbnailOnTransform) && !resReferences.Failed())
    {
      // If the transformed succeeded, the asset should now be in the NeedsThumbnail state unless the thumbnail already exists in which case we are done or the transform made changes to the asset, e.g. a mesh imported new materials in which case we will revert to transform needed as our dependencies need transform. We simply skip the thumbnail generation in this case.
      plAssetInfo::TransformState state3 = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash, uiThumbHash);
      if (state3 == plAssetInfo::TransformState::NeedsThumbnail)
      {
        ret = pAsset->CreateThumbnail();
      }
    }
  }

  return ret;
}


plStatus plAssetCurator::ResaveAsset(plAssetInfo* pAssetInfo)
{
  bool bWasOpen = false;
  plDocument* pDoc = pAssetInfo->GetManager()->GetDocumentByPath(pAssetInfo->m_Path);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = plQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_Path.GetAbsolutePath(), plDocumentFlags::None);

  if (pDoc == nullptr)
    return plStatus(plFmt("Could not open asset document '{0}'", pAssetInfo->m_Path.GetDataDirParentRelativePath()));

  plStatus ret = pDoc->SaveDocument(true);

  if (!pDoc->HasWindowBeenRequested() && !bWasOpen)
    pDoc->GetDocumentManager()->CloseDocument(pDoc);

  return ret;
}

plAssetInfo* plAssetCurator::GetAssetInfo(const plUuid& assetGuid)
{
  plAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return pAssetInfo;
  return nullptr;
}

const plAssetInfo* plAssetCurator::GetAssetInfo(const plUuid& assetGuid) const
{
  plAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return pAssetInfo;
  return nullptr;
}

plAssetInfo* plAssetCurator::GetAssetInfo(const plString& sAssetGuid)
{
  if (sAssetGuid.IsEmpty())
    return nullptr;

  if (plConversionUtils::IsStringUuid(sAssetGuid))
  {
    const plUuid guid = plConversionUtils::ConvertStringToUuid(sAssetGuid);

    plAssetInfo* pInfo = nullptr;
    if (m_KnownAssets.TryGetValue(guid, pInfo))
      return pInfo;
  }

  return nullptr;
}

plSubAsset* plAssetCurator::GetSubAssetInternal(const plUuid& assetGuid)
{
  auto it = m_KnownSubAssets.Find(assetGuid);

  if (it.IsValid())
    return &it.Value();

  return nullptr;
}

void plAssetCurator::BuildFileExtensionSet(plSet<plString>& AllExtensions)
{
  plStringBuilder sTemp;
  AllExtensions.Clear();

  const auto& assetTypes = plAssetDocumentManager::GetAllDocumentDescriptors();

  // use translated strings
  plMap<plString, const plDocumentTypeDescriptor*> allDesc;
  for (auto it : assetTypes)
  {
    allDesc[plTranslate(it.Key())] = it.Value();
  }

  for (auto it : allDesc)
  {
    const auto desc = it.Value();

    if (desc->m_pManager->GetDynamicRTTI()->IsDerivedFrom<plAssetDocumentManager>())
    {
      sTemp = desc->m_sFileExtension;
      sTemp.ToLower();

      AllExtensions.Insert(sTemp);
    }
  }
}

void plAssetCurator::OnFileChangedEvent(const plFileChangedEvent& e)
{
  switch (e.m_Type)
  {
    case plFileChangedEvent::Type::DocumentLinked:
    case plFileChangedEvent::Type::DocumentUnlinked:
      break;
    case plFileChangedEvent::Type::FileAdded:
    case plFileChangedEvent::Type::FileChanged:
    {
      // If the asset was just added it is not tracked and thus no need to invalidate anything.
      if (e.m_Type == plFileChangedEvent::Type::FileChanged)
      {
        PLASMA_LOCK(m_CuratorMutex);
        plUuid guid0 = e.m_Status.m_DocumentID;
        if (guid0.IsValid())
          InvalidateAssetTransformState(guid0);

        auto it = m_InverseTransformDeps.Find(e.m_Path);
        if (it.IsValid())
        {
          for (const plUuid& guid : it.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        auto it2 = m_InverseThumbnailDeps.Find(e.m_Path);
        if (it2.IsValid())
        {
          for (const plUuid& guid : it2.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }
      }

      // Assets should never be in an AssetCache folder.
      if (e.m_Path.GetAbsolutePath().FindSubString("/AssetCache/") != nullptr)
      {
        return;
      }

      // check that this is an asset type that we know
      plStringBuilder sExt = plPathUtils::GetFileExtension(e.m_Path);
      sExt.ToLower();
      if (!m_ValidAssetExtensions.Contains(sExt))
      {
        return;
      }

      EnsureAssetInfoUpdated(e.m_Path, e.m_Status).IgnoreResult();
    }
    break;
    case plFileChangedEvent::Type::FileRemoved:
    {
      PLASMA_LOCK(m_CuratorMutex);
      plUuid guid0 = e.m_Status.m_DocumentID;
      if (guid0.IsValid())
      {
        if (auto it = m_KnownAssets.Find(guid0); it.IsValid())
        {
          plAssetInfo* pAssetInfo = it.Value();
          PLASMA_ASSERT_DEBUG(plFileSystemModel::IsSameFile(e.m_Path, pAssetInfo->m_Path), "");
          UntrackDependencies(pAssetInfo);
          RemoveAssetTransformState(guid0);
          SetAssetExistanceState(*pAssetInfo, plAssetExistanceState::FileRemoved);
        }
      }
      auto it = m_InverseTransformDeps.Find(e.m_Path);
      if (it.IsValid())
      {
        for (const plUuid& guid : it.Value())
        {
          InvalidateAssetTransformState(guid);
        }
      }

      auto it2 = m_InverseThumbnailDeps.Find(e.m_Path);
      if (it2.IsValid())
      {
        for (const plUuid& guid : it2.Value())
        {
          InvalidateAssetTransformState(guid);
        }
      }
    }
    break;
    case plFileChangedEvent::Type::ModelReset:
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }
}

void plAssetCurator::ProcessAllCoreAssets()
{
  PLASMA_PROFILE_SCOPE("ProcessAllCoreAssets");
  if (plQtUiServices::IsHeadless())
    return;

  // The 'Core Assets' are always transformed for the PC platform,
  // as they are needed to run the editor properly
  const plPlatformProfile* pAssetProfile = GetDevelopmentAssetProfile();

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    plStringBuilder sCoreCollectionPath;
    plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sCoreCollectionPath).IgnoreResult();

    plStringBuilder sName = sCoreCollectionPath.GetFileName();
    sName.Append(".plCollectionAsset");
    sCoreCollectionPath.AppendPath(sName);

    QFile coreCollection(sCoreCollectionPath.GetData());
    if (coreCollection.exists())
    {
      auto pSubAsset = FindSubAsset(sCoreCollectionPath);
      if (pSubAsset)
      {
        // prefer certain asset types over others, to ensure that thumbnail generation works
        plHybridArray<plTempHashedString, 4> transformOrder;
        transformOrder.PushBack(plTempHashedString("RenderPipeline"));
        transformOrder.PushBack(plTempHashedString(""));

        plTransformStatus resReferences(PLASMA_SUCCESS);

        for (const plTempHashedString& name : transformOrder)
        {
          for (const auto& ref : pSubAsset->m_pAssetInfo->m_Info->m_PackageDependencies)
          {
            if (plAssetInfo* pInfo = GetAssetInfo(ref))
            {
              if (name.GetHash() == 0ull || pInfo->m_Info->m_sAssetsDocumentTypeName == name)
              {
                resReferences = ProcessAsset(pInfo, pAssetProfile, plTransformFlags::TriggeredManually);
                if (resReferences.Failed())
                  break;
              }
            }
          }
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
// plAssetCurator Update Task
////////////////////////////////////////////////////////////////////////

void plAssetCurator::RestartUpdateTask()
{
  PLASMA_LOCK(m_CuratorMutex);
  m_bRunUpdateTask = true;

  RunNextUpdateTask();
}

void plAssetCurator::ShutdownUpdateTask()
{
  {
    PLASMA_LOCK(m_CuratorMutex);
    m_bRunUpdateTask = false;
  }

  if (m_pUpdateTask)
  {
    plTaskSystem::WaitForGroup(m_UpdateTaskGroup);

    PLASMA_LOCK(m_CuratorMutex);
    m_pUpdateTask.Clear();
  }
}

bool plAssetCurator::GetNextAssetToUpdate(plUuid& guid, plStringBuilder& out_sAbsPath)
{
  PLASMA_LOCK(m_CuratorMutex);

  while (!m_TransformStateStale.IsEmpty())
  {
    auto it = m_TransformStateStale.GetIterator();
    guid = it.Key();

    auto pAssetInfo = GetAssetInfo(guid);

    // PLASMA_ASSERT_DEBUG(pAssetInfo != nullptr, "Non-existent assets should not have a tracked transform state.");

    if (pAssetInfo != nullptr)
    {
      out_sAbsPath = pAssetInfo->m_Path;
      return true;
    }
    else
    {
      plLog::Error("Non-existent assets ('{0}') should not have a tracked transform state.", guid);
      m_TransformStateStale.Remove(it);
    }
  }

  return false;
}

void plAssetCurator::OnUpdateTaskFinished(const plSharedPtr<plTask>& pTask)
{
  PLASMA_LOCK(m_CuratorMutex);

  RunNextUpdateTask();
}

void plAssetCurator::RunNextUpdateTask()
{
  PLASMA_LOCK(m_CuratorMutex);

  if (!m_bRunUpdateTask || (m_TransformStateStale.IsEmpty() && m_TransformState[plAssetInfo::TransformState::Unknown].IsEmpty()))
    return;

  if (m_pUpdateTask == nullptr)
  {
    m_pUpdateTask = PLASMA_DEFAULT_NEW(plUpdateTask, plMakeDelegate(&plAssetCurator::OnUpdateTaskFinished, this));
  }

  if (m_pUpdateTask->IsTaskFinished())
  {
    m_UpdateTaskGroup = plTaskSystem::StartSingleTask(m_pUpdateTask, plTaskPriority::FileAccess);
  }
}

////////////////////////////////////////////////////////////////////////
// plAssetCurator Check File System Helper
////////////////////////////////////////////////////////////////////////

void plAssetCurator::SetAllAssetStatusUnknown()
{
  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    UpdateAssetTransformState(it.Key(), plAssetInfo::TransformState::Unknown);
  }
}

void plAssetCurator::LoadCaches(plFileSystemModel::FilesMap& out_referencedFiles, plFileSystemModel::FoldersMap& out_referencedFolders)
{
  PLASMA_PROFILE_SCOPE("LoadCaches");
  PLASMA_LOCK(m_CuratorMutex);

  plStopwatch sw;
  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    plStringBuilder sDataDir;
    plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    plStringBuilder sCacheFile = sDataDir;
    sCacheFile.AppendPath("AssetCache", "AssetCurator.plCache");

    plFileReader reader;
    if (reader.Open(sCacheFile).Succeeded())
    {
      plUInt32 uiCuratorCacheVersion = 0;
      plUInt32 uiFileVersion = 0;
      reader >> uiCuratorCacheVersion;
      reader >> uiFileVersion;

      if (uiCuratorCacheVersion != PLASMA_CURATOR_CACHE_VERSION)
      {
        // Do not purge cache on processors.
        if (!plQtUiServices::IsHeadless())
        {
          plStringBuilder sCacheDir = sDataDir;
          sCacheDir.AppendPath("AssetCache");

          QDir dir(sCacheDir.GetData());
          if (dir.exists())
          {
            dir.removeRecursively();
          }
        }
        continue;
      }

      if (uiFileVersion != PLASMA_CURATOR_CACHE_FILE_VERSION)
        continue;

      {
        PLASMA_PROFILE_SCOPE("Assets");
        plUInt32 uiAssetCount = 0;
        reader >> uiAssetCount;
        for (plUInt32 i = 0; i < uiAssetCount; i++)
        {
          plString sPath;
          reader >> sPath;

          const plRTTI* pType = nullptr;
          plAssetDocumentInfo* pEntry = static_cast<plAssetDocumentInfo*>(plReflectionSerializer::ReadObjectFromBinary(reader, pType));
          PLASMA_ASSERT_DEBUG(pEntry != nullptr && pType == plGetStaticRTTI<plAssetDocumentInfo>(), "Failed to deserialize plAssetDocumentInfo!");
          m_CachedAssets.Insert(sPath, plUniquePtr<plAssetDocumentInfo>(pEntry, plFoundation::GetDefaultAllocator()));

          plFileStatus stat;
          reader >> stat;
          m_CachedFiles.Insert(std::move(sPath), stat);
        }

        m_KnownAssets.Reserve(m_CachedAssets.GetCount());
        m_KnownSubAssets.Reserve(m_CachedAssets.GetCount());

        m_TransformState[plAssetInfo::Unknown].Reserve(m_CachedAssets.GetCount());
        m_TransformState[plAssetInfo::UpToDate].Reserve(m_CachedAssets.GetCount());
        m_SubAssetChanged.Reserve(m_CachedAssets.GetCount());
        m_TransformStateStale.Reserve(m_CachedAssets.GetCount());
        m_Updating.Reserve(m_CachedAssets.GetCount());
      }
      {
        PLASMA_PROFILE_SCOPE("Files");
        plUInt32 uiFileCount = 0;
        reader >> uiFileCount;
        for (plUInt32 i = 0; i < uiFileCount; i++)
        {
          plDataDirPath path;
          reader >> path;
          plFileStatus stat;
          reader >> stat;
          // We invalidate all asset guids as the current cache as stored on disk is missing various bits in the curator that requires the code to go through the found new asset init code on load again.
          stat.m_DocumentID = plUuid::MakeInvalid();
          out_referencedFiles.Insert(std::move(path), stat);
        }
      }

      {
        PLASMA_PROFILE_SCOPE("Folders");
        plUInt32 uiFolderCount = 0;
        reader >> uiFolderCount;
        for (plUInt32 i = 0; i < uiFolderCount; i++)
        {
          plDataDirPath path;
          reader >> path;
          plFileStatus::Status stat;
          reader >> (plUInt8&)stat;
          out_referencedFolders.Insert(std::move(path), stat);
        }
      }
    }
  }

  plLog::Debug("Asset Curator LoadCaches: {0} ms", plArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

void plAssetCurator::SaveCaches(const plFileSystemModel::FilesMap& referencedFiles, const plFileSystemModel::FoldersMap& referencedFolders)
{
  PLASMA_PROFILE_SCOPE("SaveCaches");
  m_CachedAssets.Clear();
  m_CachedFiles.Clear();

  // Do not save cache on processors.
  if (plQtUiServices::IsHeadless())
    return;

  PLASMA_LOCK(m_CuratorMutex);
  const plUInt32 uiCuratorCacheVersion = PLASMA_CURATOR_CACHE_VERSION;

  plStopwatch sw;
  for (plUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); i++)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i];

    plStringBuilder sDataDir;
    plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    plStringBuilder sCacheFile = sDataDir;
    sCacheFile.AppendPath("AssetCache", "AssetCurator.plCache");

    const plUInt32 uiFileVersion = PLASMA_CURATOR_CACHE_FILE_VERSION;
    plUInt32 uiAssetCount = 0;
    plUInt32 uiFileCount = 0;
    plUInt32 uiFolderCount = 0;

    {
      PLASMA_PROFILE_SCOPE("Count");
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value()->m_ExistanceState == plAssetExistanceState::FileUnchanged && it.Value()->m_Path.GetDataDirIndex() == i)
        {
          ++uiAssetCount;
        }
      }
      for (auto it = referencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value().m_Status == plFileStatus::Status::Valid && it.Key().GetDataDirIndex() == i)
        {
          ++uiFileCount;
        }
      }
      for (auto it = referencedFolders.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value() == plFileStatus::Status::Valid && it.Key().GetDataDirIndex() == i)
        {
          ++uiFolderCount;
        }
      }
    }
    plDeferredFileWriter writer;
    writer.SetOutput(sCacheFile);

    writer << uiCuratorCacheVersion;
    writer << uiFileVersion;

    {
      PLASMA_PROFILE_SCOPE("Assets");
      writer << uiAssetCount;
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        const plAssetInfo* pAsset = it.Value();
        if (pAsset->m_ExistanceState == plAssetExistanceState::FileUnchanged && pAsset->m_Path.GetDataDirIndex() == i)
        {
          writer << pAsset->m_Path.GetAbsolutePath();
          plReflectionSerializer::WriteObjectToBinary(writer, plGetStaticRTTI<plAssetDocumentInfo>(), pAsset->m_Info.Borrow());
          const plFileStatus* pStat = referencedFiles.GetValue(it.Value()->m_Path);
          PLASMA_ASSERT_DEBUG(pStat != nullptr, "");
          writer << *pStat;
        }
      }
    }
    {
      PLASMA_PROFILE_SCOPE("Files");
      writer << uiFileCount;
      for (auto it = referencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        const plFileStatus& stat = it.Value();
        if (stat.m_Status == plFileStatus::Status::Valid && it.Key().GetDataDirIndex() == i)
        {
          writer << it.Key();
          writer << stat;
        }
      }
    }
    {
      PLASMA_PROFILE_SCOPE("Folders");
      writer << uiFolderCount;
      for (auto it = referencedFolders.GetIterator(); it.IsValid(); ++it)
      {
        const plFileStatus::Status stat = it.Value();
        if (stat == plFileStatus::Status::Valid && it.Key().GetDataDirIndex() == i)
        {
          writer << it.Key();
          writer << (plUInt8)stat;
        }
      }
    }

    writer.Close().IgnoreResult();
  }

  plLog::Debug("Asset Curator SaveCaches: {0} ms", plArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

void plAssetCurator::ClearAssetCaches(plAssetDocumentManager::OutputReliability threshold)
{
  const bool bWasRunning = plAssetProcessor::GetSingleton()->GetProcessTaskState() == plAssetProcessor::ProcessTaskState::Running;

  if (bWasRunning)
  {
    // pause background asset processing while we delete files
    plAssetProcessor::GetSingleton()->StopProcessTask(true);
  }

  {
    PLASMA_LOCK(m_CuratorMutex);

    plStringBuilder filePath;

    plSet<plString> keepAssets;
    plSet<plString> filesToDelete;

    // for all assets, gather their outputs and check which ones we want to keep
    // e.g. textures are perfectly reliable, and even when clearing the cache we can keep them, also because they cost a lot of time to regenerate
    for (auto it : m_KnownSubAssets)
    {
      const auto& subAsset = it.Value();
      auto pManager = subAsset.m_pAssetInfo->GetManager();
      if (pManager->GetAssetTypeOutputReliability() > threshold)
      {
        auto pDocumentTypeDescriptor = subAsset.m_pAssetInfo->m_pDocumentTypeDescriptor;
        const auto& path = subAsset.m_pAssetInfo->m_Path;

        // check additional outputs
        for (const auto& output : subAsset.m_pAssetInfo->m_Info->m_Outputs)
        {
          filePath = pManager->GetAbsoluteOutputFileName(pDocumentTypeDescriptor, path, output);
          filePath.MakeCleanPath();
          keepAssets.Insert(filePath);
        }

        filePath = pManager->GetAbsoluteOutputFileName(pDocumentTypeDescriptor, path, nullptr);
        filePath.MakeCleanPath();
        keepAssets.Insert(filePath);

        // and also keep the thumbnail
        filePath = pManager->GenerateResourceThumbnailPath(path, subAsset.m_Data.m_sName);
        filePath.MakeCleanPath();
        keepAssets.Insert(filePath);
      }
    }

    // iterate over all AssetCache folders in all data directories and gather the list of files for deletion
    plFileSystemIterator iter;
    for (plFileSystem::StartSearch(iter, "AssetCache/", plFileSystemIteratorFlags::ReportFilesRecursive); iter.IsValid(); iter.Next())
    {
      iter.GetStats().GetFullPath(filePath);
      filePath.MakeCleanPath();

      if (keepAssets.Contains(filePath))
        continue;

      filesToDelete.Insert(filePath);
    }

    for (const plString& file : filesToDelete)
    {
      plOSFile::DeleteFile(file).IgnoreResult();
    }
  }

  plAssetCurator::CheckFileSystem();

  plAssetCurator::ProcessAllCoreAssets();

  if (bWasRunning)
  {
    // restart background asset processing
    plAssetProcessor::GetSingleton()->StartProcessTask();
  }
}
