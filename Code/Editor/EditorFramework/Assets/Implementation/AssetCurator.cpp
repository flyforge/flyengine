#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetWatcher.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

#define PLASMA_CURATOR_CACHE_VERSION 2
#define PLASMA_CURATOR_CACHE_FILE_VERSION 6

PLASMA_IMPLEMENT_SINGLETON(plAssetCurator);

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetCurator)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation",
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

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plFileStatus, plNoBase, 3, plRTTIDefaultAllocator<plFileStatus>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Timestamp", m_Timestamp),
    PLASMA_MEMBER_PROPERTY("Hash", m_uiHash),
    PLASMA_MEMBER_PROPERTY("AssetGuid", m_AssetGuid),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

inline plStreamWriter& operator<<(plStreamWriter& Stream, const plFileStatus& uiValue)
{
  Stream.WriteBytes(&uiValue, sizeof(plFileStatus)).IgnoreResult();
  return Stream;
}

inline plStreamReader& operator>>(plStreamReader& Stream, plFileStatus& uiValue)
{
  Stream.ReadBytes(&uiValue, sizeof(plFileStatus));
  return Stream;
}

void plAssetInfo::Update(plUniquePtr<plAssetInfo>& rhs)
{
  m_ExistanceState = rhs->m_ExistanceState;
  m_TransformState = rhs->m_TransformState;
  m_pDocumentTypeDescriptor = rhs->m_pDocumentTypeDescriptor;
  m_sAbsolutePath = std::move(rhs->m_sAbsolutePath);
  m_sDataDirParentRelativePath = std::move(rhs->m_sDataDirParentRelativePath);
  m_sDataDirRelativePath = plStringView(m_sDataDirParentRelativePath.FindSubString("/") + 1); // skip the initial folder
  m_Info = std::move(rhs->m_Info);
  m_AssetHash = rhs->m_AssetHash;
  m_ThumbHash = rhs->m_ThumbHash;
  m_MissingDependencies = rhs->m_MissingDependencies;
  m_MissingReferences = rhs->m_MissingReferences;
  // Don't copy m_SubAssets, we want to update it independently.
  rhs = nullptr;
}

plStringView plSubAsset::GetName() const
{
  if (m_bMainAsset)
    return plPathUtils::GetFileName(m_pAssetInfo->m_sDataDirParentRelativePath);
  else
    return m_Data.m_sName;
}


void plSubAsset::GetSubAssetIdentifier(plStringBuilder& out_sPath) const
{
  out_sPath = m_pAssetInfo->m_sDataDirParentRelativePath;

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

  m_DirDescriptor = plAssetDocumentTypeDescriptor();
  m_DirDescriptor.m_sIcon = ":/AssetIcons/Directory.svg";

  m_pWatcher = PLASMA_DEFAULT_NEW(plAssetWatcher, m_FileSystemConfig);

  plSharedPtr<plDelegateTask<void>> pInitTask = PLASMA_DEFAULT_NEW(plDelegateTask<void>, "AssetCuratorUpdateCache", [this]() {
    PLASMA_LOCK(m_CuratorMutex);
LoadCaches();

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

    SaveCaches(); 
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
  m_pWatcher = nullptr;

  SaveCaches();

  {
    m_ReferencedFiles.Clear();

    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      PLASMA_DEFAULT_DELETE(it.Value());
    }
    for (auto it = m_KnownDirectories.GetIterator(); it.IsValid(); ++it)
    {
      PLASMA_DEFAULT_DELETE(it.Value());
    }

    m_RemovedFolders.Clear();
    m_KnownSubAssets.Clear();
    m_KnownAssets.Clear();
    m_TransformStateStale.Clear();
    m_KnownDirectories.Clear();

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

  if (m_pWatcher)
    m_pWatcher->MainThreadTick();

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
      else if (pInfo->m_ExistanceState == plAssetExistanceState::FileRemoved)
      {
        e.m_Type = plAssetCuratorEvent::Type::AssetRemoved;
        m_Events.Broadcast(e);
        if (!pInfo->m_bIsDir)
        {
          if (pInfo->m_bMainAsset)
          {
            deletedAssets.PushBack(pInfo->m_pAssetInfo);
          }
          m_KnownAssets.Remove(guid);
          m_KnownSubAssets.Remove(guid);
        }
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

  // TODO: Probably needs to be done in headless as well to make proper thumbnails
  if (!plQtEditorApp::GetSingleton()->IsInHeadlessMode())
  {
    if (bTopLevel && m_bNeedToReloadResources && plTime::Now() > m_NextReloadResources)
    {
      m_bNeedToReloadResources = false;
      WriteAssetTables().IgnoreResult();
    }
  }

  bReentry = false;
}

plDateTime plAssetCurator::GetLastFullTransformDate() const
{
  plStringBuilder path = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  path.AppendPath("LastFullTransform.date");

  plFileStats stat;
  if (plOSFile::GetFileStats(path, stat).Failed())
    return {};

  return stat.m_LastModificationTime;
}

void plAssetCurator::StoreFullTransformDate()
{
  plStringBuilder path = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  path.AppendPath("LastFullTransform.date");

  plOSFile file;
  if (file.Open(path, plFileOpenMode::Write).Succeeded())
  {
    plDateTime date;
    date.SetTimestamp(plTimestamp::CurrentTimestamp());

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

      range.BeginNextStep(plPathUtils::GetFileNameAndExtension(pAssetInfo->m_sDataDirParentRelativePath).GetStartPointer());
      --uiNumStepsLeft;
    }

    plTransformStatus res = ProcessAsset(pAssetInfo, pAssetProfile, transformFlags);
    if (res.Failed())
    {
      uiNumFailedSteps++;
      plLog::Error("{0} ({1})", res.m_sMessage, pAssetInfo->m_sDataDirParentRelativePath);
    }
  }

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
    for (const plString& dep : itAsset.Value()->m_Info->m_AssetTransformDependencies)
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
    range.BeginNextStep(plPathUtils::GetFileNameAndExtension(pAssetInfo->m_sDataDirParentRelativePath).GetStartPointer());

    auto res = ResaveAsset(pAssetInfo);
    if (res.m_Result.Failed())
    {
      plLog::Error("{0} ({1})", res.m_sMessage, pAssetInfo->m_sDataDirParentRelativePath);
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

    sAbsPath = pInfo->m_sAbsolutePath;
    res = ProcessAsset(pInfo, pAssetProfile, transformFlags);
  }
  if (pTypeDesc && transformFlags.IsAnySet(plTransformFlags::TriggeredManually))
  {
    // As this is triggered manually it is safe to save here as these are only run on the main thread.
    if (plDocument* pDoc = pTypeDesc->m_pManager->GetDocumentByPath(sAbsPath))
    {
      // some assets modify the document during transformation
      // make sure the state is saved, at least when the user actively executed the action
      pDoc->SaveDocument().IgnoreResult();
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

plResult plAssetCurator::WriteAssetTables(const plPlatformProfile* pAssetProfile /* = nullptr*/)
{
  CURATOR_PROFILE("WriteAssetTables");
  PLASMA_LOG_BLOCK("plAssetCurator::WriteAssetTables");

  // TODO: figure out a way to early out this function, if nothing can have changed

  plResult res = PLASMA_SUCCESS;

  plStringBuilder sd;

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    PLASMA_SUCCEED_OR_RETURN(plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sd));
    sd.Append("/");

    if (WriteAssetTable(sd, pAssetProfile).Failed())
      res = PLASMA_FAILURE;
  }

  if (pAssetProfile == nullptr || pAssetProfile == GetActiveAssetProfile())
  {
    plSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ReloadAssetLUT";
    msg.m_sPayload = GetActiveAssetProfile()->GetConfigName();
    PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);

    msg.m_sWhatToDo = "ReloadResources";
    PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  return res;
}


////////////////////////////////////////////////////////////////////////
// plAssetCurator Asset Access
////////////////////////////////////////////////////////////////////////

const plAssetCurator::plLockedSubAsset plAssetCurator::FindSubAsset(const char* szPathOrGuid, bool bExhaustiveSearch) const
{
  CURATOR_PROFILE("FindSubAsset");
  PLASMA_LOCK(m_CuratorMutex);

  if (plConversionUtils::IsStringUuid(szPathOrGuid))
  {
    return GetSubAsset(plConversionUtils::ConvertStringToUuid(szPathOrGuid));
  }

  // Split into mainAsset|subAsset
  plStringBuilder mainAsset;
  plStringView subAsset;
  const char* szSeparator = plStringUtils::FindSubString(szPathOrGuid, "|");
  if (szSeparator != nullptr)
  {
    mainAsset.SetSubString_FromTo(szPathOrGuid, szSeparator);
    subAsset = plStringView(szSeparator + 1);
  }
  else
  {
    mainAsset = szPathOrGuid;
  }
  mainAsset.MakeCleanPath();

  // Find mainAsset
  plMap<plString, plFileStatus, plCompareString_NoCase>::ConstIterator it;
  if (plPathUtils::IsAbsolutePath(mainAsset))
  {
    it = m_ReferencedFiles.Find(mainAsset);
  }
  else
  {
    // Data dir parent relative?
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      plStringBuilder sDataDir;
      plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();
      sDataDir.PathParentDirectory();
      sDataDir.AppendPath(mainAsset);
      it = m_ReferencedFiles.Find(sDataDir);
      if (it.IsValid())
        break;
    }

    if (!it.IsValid())
    {
      // Data dir relative?
      for (const auto& dd : m_FileSystemConfig.m_DataDirs)
      {
        plStringBuilder sDataDir;
        plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();
        sDataDir.AppendPath(mainAsset);
        it = m_ReferencedFiles.Find(sDataDir);
        if (it.IsValid())
          break;
      }
    }
  }

  // Did we find an asset?
  if (it.IsValid() && it.Value().m_AssetGuid.IsValid())
  {
    plAssetInfo* pAssetInfo = nullptr;
    m_KnownAssets.TryGetValue(it.Value().m_AssetGuid, pAssetInfo);
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

  auto FindAsset = [this](plStringView path) -> plAssetInfo* {
    // try to find the 'exact' relative path
    // otherwise find the shortest possible path
    plUInt32 uiMinLength = 0xFFFFFFFF;
    plAssetInfo* pBestInfo = nullptr;

    if (path.IsEmpty())
      return nullptr;

    const plStringBuilder sPath = path;
    const plStringBuilder sPathWithSlash("/", sPath);

    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->m_sDataDirParentRelativePath.EndsWith_NoCase(sPath))
      {
        // endswith -> could also be equal
        if (path.IsEqual_NoCase(it.Value()->m_sDataDirParentRelativePath.GetData()))
        {
          // if equal, just take it
          return it.Value();
        }

        // need to check again with a slash to make sure we don't return something that is of an invalid type
        // this can happen where the user is allowed to type random paths
        if (it.Value()->m_sDataDirParentRelativePath.EndsWith_NoCase(sPathWithSlash))
        {
          const plUInt32 uiLength = it.Value()->m_sDataDirParentRelativePath.GetElementCount();
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

  szSeparator = plStringUtils::FindSubString(szPathOrGuid, "|");
  if (szSeparator != nullptr)
  {
    plStringBuilder mainAsset2;
    mainAsset2.SetSubString_FromTo(szPathOrGuid, szSeparator);

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

  plStringBuilder sPath = szPathOrGuid;
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

void plAssetCurator::GenerateTransitiveHull(const plStringView assetOrPath, plSet<plString>* pDependencies, plSet<plString>* pReferences)
{
  if (plConversionUtils::IsStringUuid(assetOrPath))
  {
    auto it = m_KnownSubAssets.Find(plConversionUtils::ConvertStringToUuid(assetOrPath));
    plAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;
    const bool bInsertDep = pDependencies && !pDependencies->Contains(pAssetInfo->m_sAbsolutePath);
    const bool bInsertRef = pReferences && !pReferences->Contains(pAssetInfo->m_sAbsolutePath);

    if (bInsertDep)
    {
      pDependencies->Insert(pAssetInfo->m_sAbsolutePath);
    }
    if (bInsertRef)
    {
      pReferences->Insert(pAssetInfo->m_sAbsolutePath);
    }

    if (pDependencies)
    {
      for (const plString& dep : pAssetInfo->m_Info->m_AssetTransformDependencies)
      {
        GenerateTransitiveHull(dep, pDependencies, nullptr);
      }
    }

    if (pReferences)
    {
      for (const plString& ref : pAssetInfo->m_Info->m_RuntimeDependencies)
      {
        GenerateTransitiveHull(ref, nullptr, pReferences);
      }
    }
  }
  else
  {
    if (pDependencies && !pDependencies->Contains(assetOrPath))
    {
      pDependencies->Insert(assetOrPath);
    }
    if (pReferences && !pReferences->Contains(assetOrPath))
    {
      pReferences->Insert(assetOrPath);
    }
  }
}

plAssetInfo::TransformState plAssetCurator::IsAssetUpToDate(const plUuid& assetGuid, const plPlatformProfile*, const plAssetDocumentTypeDescriptor* pTypeDescriptor, plUInt64& out_AssetHash, plUInt64& out_ThumbHash, bool bForce)
{
  return plAssetCurator::UpdateAssetTransformState(assetGuid, out_AssetHash, out_ThumbHash, bForce);
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
  if (EnsureAssetInfoUpdated(assetGuid).Failed())
  {
    plStringBuilder tmp;
    plLog::Error("Asset with GUID {0} is unknown", plConversionUtils::ToString(assetGuid, tmp));
    return plAssetInfo::TransformState::Unknown;
  }

  // Data to pull from the asset under the lock that is needed for update computation.
  plAssetDocumentManager* pManager = nullptr;
  const plAssetDocumentTypeDescriptor* pTypeDescriptor = nullptr;
  plString sAssetFile;
  plUInt8 uiLastStateUpdate = 0;
  plUInt64 uiSettingsHash = 0;
  plHybridArray<plString, 16> assetTransformDependencies;
  plHybridArray<plString, 16> runtimeDependencies;
  plHybridArray<plString, 16> outputs;

  // Lock asset and get all data needed for update computation.
  {
    CURATOR_PROFILE("CopyAssetData");
    PLASMA_LOCK(m_CuratorMutex);
    plAssetInfo* pAssetInfo = GetAssetInfo(assetGuid);

    pManager = pAssetInfo->GetManager();
    pTypeDescriptor = pAssetInfo->m_pDocumentTypeDescriptor;
    sAssetFile = pAssetInfo->m_sAbsolutePath;
    uiLastStateUpdate = pAssetInfo->m_LastStateUpdate;
    // The settings has combines both the file settings and the global profile settings.
    uiSettingsHash = pAssetInfo->m_Info->m_uiSettingsHash + pManager->GetAssetProfileHash();
    for (const plString& dep : pAssetInfo->m_Info->m_AssetTransformDependencies)
    {
      assetTransformDependencies.PushBack(dep);
    }
    for (const plString& ref : pAssetInfo->m_Info->m_RuntimeDependencies)
    {
      runtimeDependencies.PushBack(ref);
    }
    for (const plString& output : pAssetInfo->m_Info->m_Outputs)
    {
      outputs.PushBack(output);
    }
  }

  plAssetInfo::TransformState state = plAssetInfo::TransformState::Unknown;
  plSet<plString> missingDependencies;
  plSet<plString> missingReferences;
  // Compute final state and hashes.
  {
    state = HashAsset(uiSettingsHash, assetTransformDependencies, runtimeDependencies, missingDependencies, missingReferences, out_AssetHash, out_ThumbHash, bForce);
    PLASMA_ASSERT_DEV(state == plAssetInfo::Unknown || state == plAssetInfo::MissingDependency || state == plAssetInfo::MissingReference, "Unhandled case of HashAsset return value.");

    if (state == plAssetInfo::Unknown)
    {
      if (pManager->IsOutputUpToDate(sAssetFile, outputs, out_AssetHash, pTypeDescriptor))
      {
        state = plAssetInfo::TransformState::UpToDate;
        if (pTypeDescriptor->m_AssetDocumentFlags.IsSet(plAssetDocumentFlags::SupportsThumbnail))
        {
          if (!pManager->IsThumbnailUpToDate(sAssetFile, "", out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
          {
            state = plAssetInfo::TransformState::NeedsThumbnail;
          }
        }
        else if (pTypeDescriptor->m_AssetDocumentFlags.IsSet(plAssetDocumentFlags::AutoThumbnailOnTransform))
        {
          if (!pManager->IsThumbnailUpToDate(sAssetFile, "", out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
          {
            state = plAssetInfo::TransformState::NeedsTransform;
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
        pAssetInfo->m_MissingDependencies = std::move(missingDependencies);
        pAssetInfo->m_MissingReferences = std::move(missingReferences);
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

plString plAssetCurator::FindDataDirectoryForAsset(const char* szAbsoluteAssetPath) const
{
  plStringBuilder sAssetPath(szAbsoluteAssetPath);

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    plStringBuilder sDataDir;
    plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    if (sAssetPath.IsPathBelowFolder(sDataDir))
      return sDataDir;
  }

  PLASMA_REPORT_FAILURE("Could not find data directory for asset '{0}", szAbsoluteAssetPath);
  return plFileSystem::GetSdkRootDirectory();
}

plResult plAssetCurator::FindBestMatchForFile(plStringBuilder& sFile, plArrayPtr<plString> AllowedFileExtensions) const
{
  // TODO: Merge with exhaustive search in FindSubAsset
  sFile.MakeCleanPath();

  plStringBuilder testName = sFile;

  for (const auto& ext : AllowedFileExtensions)
  {
    testName.ChangeFileExtension(ext);

    if (plFileSystem::ExistsFile(testName))
    {
      sFile = testName;
      goto found;
    }
  }

  testName = sFile.GetFileNameAndExtension();

  if (testName.IsEmpty())
  {
    sFile = "";
    return PLASMA_FAILURE;
  }

  if (plPathUtils::ContainsInvalidFilenameChars(testName))
  {
    // not much we can do here, if the filename is already invalid, we will probably not find it in out known files list

    plPathUtils::MakeValidFilename(testName, '_', sFile);
    return PLASMA_FAILURE;
  }

  {
    PLASMA_LOCK(m_CuratorMutex);

    auto SearchFile = [this](plStringBuilder& name) -> bool {
      for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value().m_Status != plFileStatus::Status::Valid)
          continue;

        const plString& key = it.Key();

        if (key.EndsWith_NoCase(name))
        {
          name = it.Key();
          return true;
        }
      }

      return false;
    };

    // search for the full name
    {
      testName.Prepend("/"); // make sure to not find partial names

      for (const auto& ext : AllowedFileExtensions)
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
    sFile = testName;
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

void plAssetCurator::FindAllUses(plUuid assetGuid, plSet<plUuid>& uses, bool transitive) const
{
  PLASMA_LOCK(m_CuratorMutex);

  plSet<plUuid> todoList;
  todoList.Insert(assetGuid);

  auto GatherReferences = [&](const plMap<plString, plHybridArray<plUuid, 1>>& inverseTracker, const plStringBuilder& sAsset) {
    auto it = inverseTracker.Find(sAsset);
    if (it.IsValid())
    {
      for (const plUuid& guid : it.Value())
      {
        if (!uses.Contains(guid))
          todoList.Insert(guid);

        uses.Insert(guid);
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
      sCurrentAsset = pInfo->m_sAbsolutePath;
      GatherReferences(m_InverseReferences, sCurrentAsset);
      GatherReferences(m_InverseDependency, sCurrentAsset);
    }
  } while (transitive && !todoList.IsEmpty());
}

////////////////////////////////////////////////////////////////////////
// plAssetCurator Manual and Automatic Change Notification
////////////////////////////////////////////////////////////////////////

void plAssetCurator::NotifyOfFileChange(const char* szAbsolutePath)
{
  plStringBuilder sPath(szAbsolutePath);
  sPath.MakeCleanPath();
  HandleSingleFile(sPath);
  // MainThreadTick();
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

  plProgressRange* range = nullptr;
  if (plThreadUtils::IsMainThread())
    range = PLASMA_DEFAULT_NEW(plProgressRange, "Check File-System for Assets", m_FileSystemConfig.m_DataDirs.GetCount(), false);

  // make sure the hashing task has finished
  ShutdownUpdateTask();

  PLASMA_LOCK(m_CuratorMutex);

  SetAllAssetStatusUnknown();

  // check every data directory
  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    plStringBuilder sTemp;
    plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).IgnoreResult();

    if (plThreadUtils::IsMainThread())
      range->BeginNextStep(dd.m_sDataDirSpecialPath);

    IterateDataDirectory(sTemp);
  }

  RemoveStaleFileInfos();

  if (plThreadUtils::IsMainThread())
  {
    PLASMA_DEFAULT_DELETE(range);
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

void plAssetCurator::NeedsReloadResources()
{
  if (m_bNeedToReloadResources)
    return;

  m_bNeedToReloadResources = true;
  m_NextReloadResources = plTime::Now() + plTime::Seconds(1.5);
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

  for (const auto& dep : pAssetInfo->m_Info->m_AssetTransformDependencies)
  {
    plBitflags<plTransformFlags> transformFlagsDeps = transformFlags;
    transformFlagsDeps.Remove(plTransformFlags::ForceTransform);
    if (plAssetInfo* pInfo = GetAssetInfo(dep))
    {
      PLASMA_SUCCEED_OR_RETURN(ProcessAsset(pInfo, pAssetProfile, transformFlagsDeps));
    }
  }

  plTransformStatus resReferences;
  for (const auto& ref : pAssetInfo->m_Info->m_RuntimeDependencies)
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


  PLASMA_ASSERT_DEV(pTypeDesc->m_pDocumentType->IsDerivedFrom<plAssetDocument>(), "Asset document does not derive from correct base class ('{0}')", pAssetInfo->m_sDataDirParentRelativePath);

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

  if (state == plAssetInfo::TransformState::MissingDependency)
  {
    return plTransformStatus(plFmt("Missing dependency for asset '{0}', can't transform.", pAssetInfo->m_sAbsolutePath));
  }

  // does the document already exist and is open ?
  bool bWasOpen = false;
  plDocument* pDoc = pTypeDesc->m_pManager->GetDocumentByPath(pAssetInfo->m_sAbsolutePath);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = plQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_sAbsolutePath, plDocumentFlags::None);

  if (pDoc == nullptr)
    return plTransformStatus(plFmt("Could not open asset document '{0}'", pAssetInfo->m_sDataDirParentRelativePath));

  PLASMA_SCOPE_EXIT(if (!pDoc->HasWindowBeenRequested() && !bWasOpen) pDoc->GetDocumentManager()->CloseDocument(pDoc););

  plTransformStatus ret;
  plAssetDocument* pAsset = static_cast<plAssetDocument*>(pDoc);
  if (state == plAssetInfo::TransformState::NeedsTransform || (state == plAssetInfo::TransformState::NeedsThumbnail && assetFlags.IsSet(plAssetDocumentFlags::AutoThumbnailOnTransform)) || (transformFlags.IsSet(plTransformFlags::TriggeredManually) && state == plAssetInfo::TransformState::NeedsImport))
  {
    ret = pAsset->TransformAsset(transformFlags, pAssetProfile);
  }

  if (state == plAssetInfo::TransformState::MissingReference)
  {
    return plTransformStatus(plFmt("Missing reference for asset '{0}', can't create thumbnail.", pAssetInfo->m_sAbsolutePath));
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
  plDocument* pDoc = pAssetInfo->GetManager()->GetDocumentByPath(pAssetInfo->m_sAbsolutePath);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = plQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_sAbsolutePath, plDocumentFlags::None);

  if (pDoc == nullptr)
    return plStatus(plFmt("Could not open asset document '{0}'", pAssetInfo->m_sDataDirParentRelativePath));

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

void plAssetCurator::HandleSingleFile(const plString& sAbsolutePath)
{
  CURATOR_PROFILE("HandleSingleFile");
  PLASMA_LOCK(m_CuratorMutex);

  plFileStats Stats;
  if (plOSFile::GetFileStats(sAbsolutePath, Stats).Failed())
  {
    // this is a bit tricky:
    // when the document is deleted on disk, it would be nicer not to close it (discarding modifications!)
    // instead we could set it as modified
    // but then when it was only moved or renamed that means we have another document with the same GUID
    // so once the user would save the now modified document, we would end up with two documents with the same GUID
    // so, for now, since this is probably a rare case anyway, we just close the document without asking
    plDocumentManager::EnsureDocumentIsClosedInAllManagers(sAbsolutePath);

    if (plFileStatus* pFileStatus = m_ReferencedFiles.GetValue(sAbsolutePath))
    {
      pFileStatus->m_Timestamp.Invalidate();
      pFileStatus->m_uiHash = 0;
      pFileStatus->m_Status = plFileStatus::Status::Unknown;

      plUuid guid0 = pFileStatus->m_AssetGuid;
      if (guid0.IsValid())
      {
        plAssetInfo* pAssetInfo = m_KnownAssets[guid0];
        UntrackDependencies(pAssetInfo);
        RemoveAssetTransformState(guid0);
        SetAssetExistanceState(*pAssetInfo, plAssetExistanceState::FileRemoved);
        pFileStatus->m_AssetGuid = plUuid();
      }

      auto it = m_InverseDependency.Find(sAbsolutePath);
      if (it.IsValid())
      {
        for (const plUuid& guid : it.Value())
        {
          InvalidateAssetTransformState(guid);
        }
      }

      auto it2 = m_InverseReferences.Find(sAbsolutePath);
      if (it2.IsValid())
      {
        for (const plUuid& guid : it2.Value())
        {
          InvalidateAssetTransformState(guid);
        }
      }
    }

    return;
  }
  if (Stats.m_bIsDirectory)
    HandleSingleDir(sAbsolutePath, Stats);
  else
    HandleSingleFile(sAbsolutePath, Stats);
}

void plAssetCurator::HandleSingleFile(const plString& sAbsolutePath, const plFileStats& FileStat)
{
  PLASMA_ASSERT_DEV(!FileStat.m_bIsDirectory, "Directories are handled by plAssetWatcher and should not pass into this function.");
  CURATOR_PROFILE("HandleSingleFile2");
  PLASMA_LOCK(m_CuratorMutex);

  plStringBuilder sExt = plPathUtils::GetFileExtension(sAbsolutePath);
  sExt.ToLower();

  // store information for every file, even when it is no asset, it might be a dependency for some asset
  auto& RefFile = m_ReferencedFiles[sAbsolutePath];

  // mark the file as valid (i.e. we saw it on disk, so it hasn't been deleted or such)
  RefFile.m_Status = plFileStatus::Status::Valid;

  bool fileChanged = !RefFile.m_Timestamp.Compare(FileStat.m_LastModificationTime, plTimestamp::CompareMode::Identical);
  if (fileChanged)
  {
    RefFile.m_Timestamp.Invalidate();
    RefFile.m_uiHash = 0;
    if (RefFile.m_AssetGuid.IsValid())
      InvalidateAssetTransformState(RefFile.m_AssetGuid);

    auto it = m_InverseDependency.Find(sAbsolutePath);
    if (it.IsValid())
    {
      for (const plUuid& guid : it.Value())
      {
        InvalidateAssetTransformState(guid);
      }
    }

    auto it2 = m_InverseReferences.Find(sAbsolutePath);
    if (it2.IsValid())
    {
      for (const plUuid& guid : it2.Value())
      {
        InvalidateAssetTransformState(guid);
      }
    }
  }

  // Assets should never be in an AssetCache folder.
  const char* szNeedle = sAbsolutePath.FindSubString("AssetCache/");
  if (szNeedle != nullptr && sAbsolutePath.GetData() != szNeedle && szNeedle[-1] == '/')
  {
    return;
  }

  // check that this is an asset type that we know
  if (!m_ValidAssetExtensions.Contains(sExt))
  {
    return;
  }

  // the file is a known asset type
  // so make sure it gets a valid GUID assigned

  // File hasn't change, early out.
  if (RefFile.m_AssetGuid.IsValid() && !fileChanged)
    return;

  // This will update the timestamp for assets.
  EnsureAssetInfoUpdated(sAbsolutePath).IgnoreResult();
}

void plAssetCurator::HandleSingleDir(const plString& sAbsolutePath)
{
  plFileStats Stats;
  if (plFileSystem::GetFileStats(sAbsolutePath, Stats).Failed())
  {
    if (plFileStatus* pFileStatus = m_AssetFolders.GetValue(sAbsolutePath))
    {
      pFileStatus->m_Timestamp.Invalidate();
      pFileStatus->m_uiHash = 0;
      pFileStatus->m_Status = plFileStatus::Status::Unknown;

      plUuid guid0 = pFileStatus->m_AssetGuid;
      if (guid0.IsValid())
      {
        plAssetInfo* pAssetInfo = m_KnownDirectories[guid0];

        m_KnownDirectories.Remove(guid0);
        m_KnownSubAssets.Remove(guid0);
        m_AssetFolders.Remove(sAbsolutePath);
        m_RemovedFolders.PushBack(pAssetInfo->m_sDataDirRelativePath);
        pFileStatus->m_AssetGuid = plUuid();
        PLASMA_DEFAULT_DELETE(pAssetInfo);
      }
    }
    return;
  }

  HandleSingleDir(sAbsolutePath, Stats);
}

void plAssetCurator::HandleSingleDir(const plString& sAbsolutePath, const plFileStats& FileStat)
{
  PLASMA_ASSERT_DEV(FileStat.m_bIsDirectory, "Files are handled by HandleSingleFile function.");

  // store the folder of the asset
  plStringBuilder sAssetFolder = sAbsolutePath;
  sAssetFolder.TrimWordEnd("/");

  if (m_AssetFolders.Contains(sAssetFolder) || !plPathUtils::IsAbsolutePath(sAssetFolder))
  {
    m_AssetFolders[sAssetFolder].m_Status = plFileStatus::Status::Valid;
    return;
  }

  if (plStringUtils::FindSubString_NoCase(sAssetFolder, "assetcache") != nullptr)
  {
    return;
  }

  plUuid id = plUuid();
  id.CreateNewUuid();

  plFileStatus newStatus = plFileStatus();
  newStatus.m_AssetGuid = id;

  m_AssetFolders.Insert(sAssetFolder, newStatus);

  plSubAsset folderAsset = plSubAsset();

  folderAsset.m_pAssetInfo = PLASMA_DEFAULT_NEW(plAssetInfo);

  folderAsset.m_pAssetInfo->m_sAbsolutePath = sAssetFolder;
  if (plFileSystem::MakePathRelative(sAssetFolder.GetData(), sAssetFolder).Failed())
  {
    plLog::Error("Failed to make path relative {}", sAssetFolder);
    return;
  }
  folderAsset.m_pAssetInfo->m_sDataDirRelativePath = plString(sAssetFolder);

  folderAsset.m_pAssetInfo->m_pDocumentTypeDescriptor = &m_DirDescriptor;
  //plSubAssetData
  folderAsset.m_Data = plSubAssetData();
  folderAsset.m_Data.m_sName = sAssetFolder.GetFileName();
  folderAsset.m_Data.m_Guid = id;

  sAssetFolder.PathParentDirectory();
  folderAsset.m_pAssetInfo->m_sDataDirParentRelativePath = plString(sAssetFolder);

  folderAsset.m_bMainAsset = false;
  folderAsset.m_bIsDir = true;
  folderAsset.m_pAssetInfo->m_TransformState = plAssetInfo::TransformState::Folder;
  m_KnownSubAssets.Insert(id, folderAsset);
  m_KnownDirectories.Insert(id, folderAsset.m_pAssetInfo);
}

plResult plAssetCurator::WriteAssetTable(const char* szDataDirectory, const plPlatformProfile* pAssetProfile0 /*= nullptr*/)
{
  const plPlatformProfile* pAssetProfile = pAssetProfile0;

  if (pAssetProfile == nullptr)
  {
    pAssetProfile = GetActiveAssetProfile();
  }

  plStringBuilder sDataDir = szDataDirectory;
  sDataDir.MakeCleanPath();

  plStringBuilder sFinalPath(sDataDir, "/AssetCache/", pAssetProfile->GetConfigName(), ".plAidlt");
  sFinalPath.MakeCleanPath();

  plStringBuilder sTemp, sTemp2;
  plString sResourcePath;

  plMap<plString, plString> GuidToPath;

  {
    for (auto& man : plAssetDocumentManager::GetAllDocumentManagers())
    {
      if (!man->GetDynamicRTTI()->IsDerivedFrom<plAssetDocumentManager>())
        continue;

      plAssetDocumentManager* pManager = static_cast<plAssetDocumentManager*>(man);

      // allow to add fully custom entries
      pManager->AddEntriesToAssetTable(sDataDir, pAssetProfile, GuidToPath);
    }
  }

  // TODO: Iterate over m_KnownSubAssets instead
  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    sTemp = it.Value()->m_sAbsolutePath;

    // ignore all assets that are not located in this data directory
    if (!sTemp.IsPathBelowFolder(sDataDir))
      continue;

    plAssetDocumentManager* pManager = it.Value()->GetManager();

    auto WriteEntry = [this, &sDataDir, &pAssetProfile, &GuidToPath, pManager, &sTemp, &sTemp2](const plUuid& guid) {
      plSubAsset* pSub = GetSubAssetInternal(guid);
      plString sEntry = pManager->GetAssetTableEntry(pSub, sDataDir, pAssetProfile);

      // it is valid to write no asset table entry, if no redirection is required
      // this is used by decal assets for instance
      if (!sEntry.IsEmpty())
      {
        plConversionUtils::ToString(guid, sTemp2);

        GuidToPath[sTemp2] = sEntry;
      }
    };

    WriteEntry(it.Key());
    for (const plUuid& subGuid : it.Value()->m_SubAssets)
    {
      WriteEntry(subGuid);
    }
  }

  plDeferredFileWriter file;
  file.SetOutput(sFinalPath);

  for (auto it = GuidToPath.GetIterator(); it.IsValid(); ++it)
  {
    const plString& guid = it.Key();
    const plString& path = it.Value();

    file.WriteBytes(guid.GetData(), guid.GetElementCount()).IgnoreResult();
    file.WriteBytes(";", 1).IgnoreResult();
    file.WriteBytes(path.GetData(), path.GetElementCount()).IgnoreResult();
    file.WriteBytes("\n", 1).IgnoreResult();
  }

  if (file.Close().Failed())
  {
    plLog::Error("Failed to open asset lookup table file ('{0}')", sFinalPath);
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
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
          for (const auto& ref : pSubAsset->m_pAssetInfo->m_Info->m_RuntimeDependencies)
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
      out_sAbsPath = pAssetInfo->m_sAbsolutePath;
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
  // tags all known files as unknown, such that we can later remove files
  // that can not be found anymore

  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Status = plFileStatus::Status::Unknown;
  }

  for (auto it = m_AssetFolders.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Status = plFileStatus::Status::Unknown;
  }

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    UpdateAssetTransformState(it.Key(), plAssetInfo::TransformState::Unknown);
  }
}

void plAssetCurator::RemoveStaleFileInfos()
{
  plSet<plString> unknownFiles;
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    // search for files that existed previously but have not been found anymore recently
    if (it.Value().m_Status == plFileStatus::Status::Unknown)
    {
      unknownFiles.Insert(it.Key());
    }
  }

  for (const plString& sFile : unknownFiles)
  {
    if (!sFile.IsEmpty())
    HandleSingleFile(sFile);
    m_ReferencedFiles.Remove(sFile);
  }

  unknownFiles.Clear();
  for (auto it = m_AssetFolders.GetIterator(); it.IsValid(); ++it)
  {
    // search for files that existed previously but have not been found anymore recently
    if (it.Value().m_Status == plFileStatus::Status::Unknown)
    {
      unknownFiles.Insert(it.Key());
    }
  }

  for (const plString& sFile : unknownFiles)
  {
    if (!sFile.IsEmpty())
    HandleSingleDir(sFile);
    m_ReferencedFiles.Remove(sFile);
  }
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

void plAssetCurator::IterateDataDirectory(const char* szDataDir, plSet<plString>* pFoundFiles)
{
  plStringBuilder sDataDir = szDataDir;
  sDataDir.MakeCleanPath();
  PLASMA_ASSERT_DEV(plPathUtils::IsAbsolutePath(szDataDir), "Only absolute paths are supported for directory iteration.");

  while (sDataDir.EndsWith("/"))
    sDataDir.Shrink(0, 1);

  if (sDataDir.IsEmpty())
    return;

  plFileSystemIterator iterator;
  iterator.StartSearch(sDataDir, plFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);

  if (!iterator.IsValid())
    return;

  plStringBuilder sPath;
  plFileStats Stats;
  for (; iterator.IsValid(); iterator.Next())
  {
    Stats = iterator.GetStats();
    sPath = iterator.GetCurrentPath();
    sPath.AppendPath(Stats.m_sName);
    sPath.MakeCleanPath();

    if (Stats.m_bIsDirectory)
    {
      HandleSingleDir(sPath, Stats);
    }
    else
    {
      HandleSingleFile(sPath, Stats);
      // TODO : Review, is it useful to return a list of files if we already Handle them here?
      if (pFoundFiles)
      {
        pFoundFiles->Insert(sPath);
      }
    }
  }

  if (plFileSystem::GetFileStats(sDataDir, Stats).Failed())
  {
    plLog::Error("Failed to get stats for directory : {}", sDataDir);
    return;
  }
  sPath = iterator.GetCurrentPath();
  sPath.AppendPath(Stats.m_sName);
  sPath.MakeCleanPath();

  if (plOSFile::ExistsDirectory(sPath))
    HandleSingleDir(sPath, Stats);
}

void plAssetCurator::LoadCaches()
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
      plUInt32 uiAssetCount = 0;
      plUInt32 uiFileCount = 0;
      reader >> uiCuratorCacheVersion;
      reader >> uiFileVersion;
      reader >> uiAssetCount;
      reader >> uiFileCount;

      m_KnownAssets.Reserve(m_CachedAssets.GetCount());
      m_KnownSubAssets.Reserve(m_CachedAssets.GetCount());

      m_TransformState[plAssetInfo::Unknown].Reserve(m_CachedAssets.GetCount());
      m_TransformState[plAssetInfo::UpToDate].Reserve(m_CachedAssets.GetCount());
      m_SubAssetChanged.Reserve(m_CachedAssets.GetCount());
      m_TransformStateStale.Reserve(m_CachedAssets.GetCount());
      m_Updating.Reserve(m_CachedAssets.GetCount());

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

      const plRTTI* pFileStatusType = plGetStaticRTTI<plFileStatus>();
      {
        PLASMA_PROFILE_SCOPE("Assets");
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
      }
      {
        PLASMA_PROFILE_SCOPE("Files");
        for (plUInt32 i = 0; i < uiFileCount; i++)
        {
          plString sPath;
          reader >> sPath;
          plFileStatus stat;
          reader >> stat;
          m_ReferencedFiles.Insert(std::move(sPath), stat);
        }
      }
    }
  }

  plLog::Debug("Asset Curator LoadCaches: {0} ms", plArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

void plAssetCurator::SaveCaches()
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
  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    plStringBuilder sDataDir;
    plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    plStringBuilder sCacheFile = sDataDir;
    sCacheFile.AppendPath("AssetCache", "AssetCurator.plCache");

    const plUInt32 uiFileVersion = PLASMA_CURATOR_CACHE_FILE_VERSION;
    plUInt32 uiAssetCount = 0;
    plUInt32 uiFileCount = 0;

    {
      PLASMA_PROFILE_SCOPE("Count");
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value()->m_ExistanceState == plAssetExistanceState::FileUnchanged && it.Value()->m_sAbsolutePath.StartsWith(sDataDir))
        {
          ++uiAssetCount;
        }
      }
      for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value().m_Status == plFileStatus::Status::Valid && !it.Value().m_AssetGuid.IsValid() && it.Key().StartsWith(sDataDir))
        {
          ++uiFileCount;
        }
      }
    }
    plDeferredFileWriter writer;
    writer.SetOutput(sCacheFile);

    writer << uiCuratorCacheVersion;
    writer << uiFileVersion;
    writer << uiAssetCount;
    writer << uiFileCount;

    {
      PLASMA_PROFILE_SCOPE("Assets");
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        const plAssetInfo* pAsset = it.Value();
        if (pAsset->m_ExistanceState == plAssetExistanceState::FileUnchanged && pAsset->m_sAbsolutePath.StartsWith(sDataDir))
        {
          writer << pAsset->m_sAbsolutePath;
          plReflectionSerializer::WriteObjectToBinary(writer, plGetStaticRTTI<plAssetDocumentInfo>(), pAsset->m_Info.Borrow());
          const plFileStatus* pStat = m_ReferencedFiles.GetValue(it.Value()->m_sAbsolutePath);
          PLASMA_ASSERT_DEBUG(pStat != nullptr, "");
          writer << *pStat;
        }
      }
    }
    {
      PLASMA_PROFILE_SCOPE("Files");
      for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        const plFileStatus& stat = it.Value();
        if (stat.m_Status == plFileStatus::Status::Valid && !stat.m_AssetGuid.IsValid() && it.Key().StartsWith(sDataDir))
        {
          writer << it.Key();
          writer << stat;
        }
      }
    }
    writer.Close().IgnoreResult();
  }

  plLog::Debug("Asset Curator SaveCaches: {0} ms", plArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}
