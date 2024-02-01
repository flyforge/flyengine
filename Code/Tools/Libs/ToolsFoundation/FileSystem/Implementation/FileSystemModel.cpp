#include <ToolsFoundation/ToolsFoundationDLL.h>

#if PL_ENABLED(PL_SUPPORTS_DIRECTORY_WATCHER) && PL_ENABLED(PL_SUPPORTS_FILE_ITERATORS)

#  include <ToolsFoundation/FileSystem/FileSystemModel.h>
#  include <ToolsFoundation/FileSystem/FileSystemWatcher.h>

#  include <Foundation/Algorithm/HashStream.h>
#  include <Foundation/Configuration/SubSystem.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/IO/MemoryStream.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Time/Stopwatch.h>
#  include <Foundation/Utilities/Progress.h>

PL_IMPLEMENT_SINGLETON(plFileSystemModel);

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, FileSystemModel)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    PL_DEFAULT_NEW(plFileSystemModel);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plFileSystemModel* pDummy = plFileSystemModel::GetSingleton();
    PL_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  thread_local plHybridArray<plFileChangedEvent, 2, plStaticsAllocatorWrapper> g_PostponedFiles;
  thread_local bool g_bInFileBroadcast = false;
  thread_local plHybridArray<plFolderChangedEvent, 2, plStaticsAllocatorWrapper> g_PostponedFolders;
  thread_local bool g_bInFolderBroadcast = false;
} // namespace

plFolderChangedEvent::plFolderChangedEvent(const plDataDirPath& file, Type type)
  : m_Path(file)
  , m_Type(type)
{
}

plFileChangedEvent::plFileChangedEvent(const plDataDirPath& file, plFileStatus status, Type type)
  : m_Path(file)
  , m_Status(status)
  , m_Type(type)
{
}

bool plFileSystemModel::IsSameFile(const plStringView sAbsolutePathA, const plStringView sAbsolutePathB)
{
#  if (PL_ENABLED(PL_SUPPORTS_CASE_INSENSITIVE_PATHS))
  return sAbsolutePathA.IsEqual_NoCase(sAbsolutePathB);
#  else
  return sAbsolutePathA.IsEqual(sAbsolutePathB);
#  endif
}

////////////////////////////////////////////////////////////////////////
// plAssetFiles
////////////////////////////////////////////////////////////////////////

plFileSystemModel::plFileSystemModel()
  : m_SingletonRegistrar(this)
{
}

plFileSystemModel::~plFileSystemModel() = default;

void plFileSystemModel::Initialize(const plApplicationFileSystemConfig& fileSystemConfig, plFileSystemModel::FilesMap&& referencedFiles, plFileSystemModel::FoldersMap&& referencedFolders)
{
  {
    PL_PROFILE_SCOPE("Initialize");
    PL_LOCK(m_FilesMutex);
    m_FileSystemConfig = fileSystemConfig;

    m_ReferencedFiles = std::move(referencedFiles);
    m_ReferencedFolders = std::move(referencedFolders);

    plStringBuilder sDataDirPath;
    m_DataDirRoots.Reserve(m_FileSystemConfig.m_DataDirs.GetCount());
    for (plUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
    {
      if (plFileSystem::ResolveSpecialDirectory(m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath, sDataDirPath).Failed())
      {
        plLog::Error("Failed to resolve data directory named '{}' at '{}'", m_FileSystemConfig.m_DataDirs[i].m_sRootName, m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath);
        m_DataDirRoots.PushBack({});
      }
      else
      {
        sDataDirPath.MakeCleanPath();
        sDataDirPath.Trim(nullptr, "/");

        m_DataDirRoots.PushBack(sDataDirPath);

        // The root should always be in the model so that every file's parent folder is present in the model.
        m_ReferencedFolders.FindOrAdd(plDataDirPath(sDataDirPath, m_DataDirRoots, i)).Value() = plFileStatus::Status::Valid;
      }
    }

    // Update data dir index and remove files no longer inside a data dir.
    for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid();)
    {
      const bool bValid = it.Key().UpdateDataDirInfos(m_DataDirRoots, it.Key().GetDataDirIndex());
      if (!bValid)
      {
        it = m_ReferencedFiles.Remove(it);
      }
      else
      {
        ++it;
      }
    }
    for (auto it = m_ReferencedFolders.GetIterator(); it.IsValid();)
    {
      const bool bValid = it.Key().UpdateDataDirInfos(m_DataDirRoots, it.Key().GetDataDirIndex());
      if (!bValid)
      {
        it = m_ReferencedFolders.Remove(it);
      }
      else
      {
        ++it;
      }
    }

    m_pWatcher = PL_DEFAULT_NEW(plFileSystemWatcher, m_FileSystemConfig);
    m_WatcherSubscription = m_pWatcher->m_Events.AddEventHandler(plMakeDelegate(&plFileSystemModel::OnAssetWatcherEvent, this));
    m_pWatcher->Initialize();
    m_bInitialized = true;
  }
  FireFileChangedEvent({}, {}, plFileChangedEvent::Type::ModelReset);
  FireFolderChangedEvent({}, plFolderChangedEvent::Type::ModelReset);
}


void plFileSystemModel::Deinitialize(plFileSystemModel::FilesMap* out_pReferencedFiles, plFileSystemModel::FoldersMap* out_pReferencedFolders)
{
  {
    PL_LOCK(m_FilesMutex);
    PL_PROFILE_SCOPE("Deinitialize");
    m_pWatcher->m_Events.RemoveEventHandler(m_WatcherSubscription);
    m_pWatcher->Deinitialize();
    m_pWatcher.Clear();

    if (out_pReferencedFiles)
    {
      m_ReferencedFiles.Swap(*out_pReferencedFiles);
    }
    if (out_pReferencedFolders)
    {
      m_ReferencedFolders.Swap(*out_pReferencedFolders);
    }
    m_ReferencedFiles.Clear();
    m_ReferencedFolders.Clear();
    m_LockedFiles.Clear();
    m_FileSystemConfig = plApplicationFileSystemConfig();
    m_DataDirRoots.Clear();
    m_bInitialized = false;
  }
  FireFileChangedEvent({}, {}, plFileChangedEvent::Type::ModelReset);
  FireFolderChangedEvent({}, plFolderChangedEvent::Type::ModelReset);
}

void plFileSystemModel::MainThreadTick()
{
  if (m_pWatcher)
    m_pWatcher->MainThreadTick();
}

const plFileSystemModel::LockedFiles plFileSystemModel::GetFiles() const
{
  return LockedFiles(m_FilesMutex, &m_ReferencedFiles);
}


const plFileSystemModel::LockedFolders plFileSystemModel::GetFolders() const
{
  return LockedFolders(m_FilesMutex, &m_ReferencedFolders);
}

void plFileSystemModel::NotifyOfChange(plStringView sAbsolutePath)
{
  if (!m_bInitialized)
    return;

  PL_ASSERT_DEV(plPathUtils::IsAbsolutePath(sAbsolutePath), "Only absolute paths are supported for directory iteration.");

  plStringBuilder sPath(sAbsolutePath);
  sPath.MakeCleanPath();
  sPath.Trim(nullptr, "/");
  if (sPath.IsEmpty())
    return;
  plDataDirPath folder(sPath, m_DataDirRoots);

  // We ignore any changes outside the model's data dirs.
  if (!folder.IsValid())
    return;

  HandleSingleFile(std::move(folder), true);
}

void plFileSystemModel::CheckFileSystem()
{
  if (!m_bInitialized)
    return;

  PL_PROFILE_SCOPE("CheckFileSystem");

  plUniquePtr<plProgressRange> range = nullptr;
  if (plThreadUtils::IsMainThread())
    range = PL_DEFAULT_NEW(plProgressRange, "Check File-System for Assets", m_FileSystemConfig.m_DataDirs.GetCount(), false);

  {
    SetAllStatusUnknown();

    // check every data directory
    for (plUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); i++)
    {
      auto& dd = m_FileSystemConfig.m_DataDirs[i];
      if (plThreadUtils::IsMainThread())
        range->BeginNextStep(dd.m_sDataDirSpecialPath);
      if (!m_DataDirRoots[i].IsEmpty())
      {
        CheckFolder(m_DataDirRoots[i]);
      }
    }

    RemoveStaleFileInfos();
  }

  if (plThreadUtils::IsMainThread())
  {
    range = nullptr;
  }

  FireFileChangedEvent({}, {}, plFileChangedEvent::Type::ModelReset);
  FireFolderChangedEvent({}, plFolderChangedEvent::Type::ModelReset);
}


plResult plFileSystemModel::FindFile(plStringView sPath, plFileStatus& out_stat) const
{
  if (!m_bInitialized)
    return PL_FAILURE;

  PL_LOCK(m_FilesMutex);
  plFileSystemModel::FilesMap::ConstIterator it;
  if (plPathUtils::IsAbsolutePath(sPath))
  {
    it = m_ReferencedFiles.Find(sPath);
  }
  else
  {
    // Data dir parent relative?
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      plStringBuilder sDataDir;
      plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).AssertSuccess();
      sDataDir.PathParentDirectory();
      sDataDir.AppendPath(sPath);
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
        plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).AssertSuccess();
        sDataDir.AppendPath(sPath);
        it = m_ReferencedFiles.Find(sDataDir);
        if (it.IsValid())
          break;
      }
    }
  }

  if (it.IsValid())
  {
    out_stat = it.Value();
    return PL_SUCCESS;
  }
  return PL_FAILURE;
}


plResult plFileSystemModel::FindFile(plDelegate<bool(const plDataDirPath&, const plFileStatus&)> visitor) const
{
  if (!m_bInitialized)
    return PL_FAILURE;

  PL_LOCK(m_FilesMutex);
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    if (visitor(it.Key(), it.Value()))
      return PL_SUCCESS;
  }
  return PL_FAILURE;
}


plResult plFileSystemModel::LinkDocument(plStringView sAbsolutePath, const plUuid& documentId)
{
  if (!m_bInitialized || !documentId.IsValid())
    return PL_FAILURE;

  plDataDirPath filePath;
  plFileStatus fileStatus;
  {
    PL_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (it.IsValid())
    {
      // Store status before updates so we can fire the unlink if a guid was already set.
      fileStatus = it.Value();
      it.Value().m_DocumentID = documentId;
      filePath = it.Key();
    }
    else
    {
      return PL_FAILURE;
    }
  }

  if (fileStatus.m_DocumentID != documentId)
  {
    if (fileStatus.m_DocumentID.IsValid())
    {
      FireFileChangedEvent(filePath, fileStatus, plFileChangedEvent::Type::DocumentUnlinked);
    }
    fileStatus.m_DocumentID = documentId;
    FireFileChangedEvent(std::move(filePath), fileStatus, plFileChangedEvent::Type::DocumentLinked);
  }
  return PL_SUCCESS;
}

plResult plFileSystemModel::UnlinkDocument(plStringView sAbsolutePath)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  plDataDirPath filePath;
  plFileStatus fileStatus;
  bool bDocumentLinkChanged = false;
  {
    PL_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath);
    if (it.IsValid())
    {
      bDocumentLinkChanged = it.Value().m_DocumentID.IsValid();
      fileStatus = it.Value();
      it.Value().m_DocumentID = plUuid::MakeInvalid();
      filePath = it.Key();
    }
    else
    {
      return PL_FAILURE;
    }
  }

  if (bDocumentLinkChanged)
  {
    FireFileChangedEvent(std::move(filePath), fileStatus, plFileChangedEvent::Type::DocumentUnlinked);
  }
  return PL_SUCCESS;
}

plResult plFileSystemModel::HashFile(plStringView sAbsolutePath, plFileStatus& out_stat)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  PL_ASSERT_DEV(plPathUtils::IsAbsolutePath(sAbsolutePath), "Only absolute paths are supported for hashing.");

  plStringBuilder sAbsolutePath2(sAbsolutePath);
  sAbsolutePath2.MakeCleanPath();
  sAbsolutePath2.Trim("", "/");
  if (sAbsolutePath2.IsEmpty())
    return PL_FAILURE;

  plDataDirPath file(sAbsolutePath2, m_DataDirRoots);

  plFileStats statDep;
  if (plOSFile::GetFileStats(sAbsolutePath2, statDep).Failed())
  {
    plLog::Error("Failed to hash file '{0}', retrieve stats failed", sAbsolutePath2);
    return PL_FAILURE;
  }

  // We ignore any changes outside the model's data dirs.
  if (file.IsValid())
  {
    {
      PL_LOCK(m_FilesMutex);
      auto it = m_ReferencedFiles.Find(sAbsolutePath2);
      if (it.IsValid())
      {
        out_stat = it.Value();
      }
    }

    // We can only hash files that are tracked.
    if (out_stat.m_Status == plFileStatus::Status::Unknown)
    {
      out_stat = HandleSingleFile(file, statDep, false);
      if (out_stat.m_Status == plFileStatus::Status::Unknown)
      {
        plLog::Error("Failed to hash file '{0}', update failed", sAbsolutePath2);
        return PL_FAILURE;
      }
    }

    // if the file has been modified, make sure to get updated data
    if (!out_stat.m_LastModified.Compare(statDep.m_LastModificationTime, plTimestamp::CompareMode::Identical) || out_stat.m_uiHash == 0)
    {
      FILESYSTEM_PROFILE(sAbsolutePath2);
      plFileReader fileReader;
      if (fileReader.Open(sAbsolutePath2).Failed())
      {
        MarkFileLocked(sAbsolutePath2);
        plLog::Error("Failed to hash file '{0}', open failed", sAbsolutePath2);
        return PL_FAILURE;
      }

      // We need to request the stats again wile while we have shared read access or we might trigger a race condition of writes to the file between the last stat call and the current file open.
      if (plOSFile::GetFileStats(sAbsolutePath2, statDep).Failed())
      {
        plLog::Error("Failed to hash file '{0}', retrieve stats failed", sAbsolutePath2);
        return PL_FAILURE;
      }
      out_stat.m_LastModified = statDep.m_LastModificationTime;
      out_stat.m_uiHash = plFileSystemModel::HashFile(fileReader, nullptr);
      out_stat.m_Status = plFileStatus::Status::Valid;

      // Update state. No need to compare timestamps we hold a lock on the file via the reader.
      PL_LOCK(m_FilesMutex);
      m_ReferencedFiles.Insert(file, out_stat);
    }
    return PL_SUCCESS;
  }
  else
  {
    {
      PL_LOCK(m_FilesMutex);
      auto it = m_TransiendFiles.Find(sAbsolutePath2);
      if (it.IsValid())
      {
        out_stat = it.Value();
      }
    }

    // if the file has been modified, make sure to get updated data
    if (!out_stat.m_LastModified.Compare(statDep.m_LastModificationTime, plTimestamp::CompareMode::Identical) || out_stat.m_uiHash == 0)
    {
      FILESYSTEM_PROFILE(sAbsolutePath2);
      plFileReader file;
      if (file.Open(sAbsolutePath2).Failed())
      {
        plLog::Error("Failed to hash file '{0}', open failed", sAbsolutePath2);
        return PL_FAILURE;
      }

      // We need to request the stats again wile while we have shared read access or we might trigger a race condition of writes to the file between the last stat call and the current file open.
      if (plOSFile::GetFileStats(sAbsolutePath2, statDep).Failed())
      {
        plLog::Error("Failed to hash file '{0}', retrieve stats failed", sAbsolutePath2);
        return PL_FAILURE;
      }
      out_stat.m_LastModified = statDep.m_LastModificationTime;
      out_stat.m_uiHash = plFileSystemModel::HashFile(file, nullptr);
      out_stat.m_Status = plFileStatus::Status::Valid;

      // Update state. No need to compare timestamps we hold a lock on the file via the reader.
      PL_LOCK(m_FilesMutex);
      m_TransiendFiles.Insert(sAbsolutePath2, out_stat);
    }
    return PL_SUCCESS;
  }
}


plUInt64 plFileSystemModel::HashFile(plStreamReader& ref_inputStream, plStreamWriter* pPassThroughStream)
{
  plHashStreamWriter64 hsw;

  FILESYSTEM_PROFILE("HashFile");
  plUInt8 cachedBytes[1024 * 10];

  while (true)
  {
    const plUInt64 uiRead = ref_inputStream.ReadBytes(cachedBytes, PL_ARRAY_SIZE(cachedBytes));

    if (uiRead == 0)
      break;

    hsw.WriteBytes(cachedBytes, uiRead).AssertSuccess();

    if (pPassThroughStream != nullptr)
      pPassThroughStream->WriteBytes(cachedBytes, uiRead).AssertSuccess();
  }

  return hsw.GetHashValue();
}

plResult plFileSystemModel::ReadDocument(plStringView sAbsolutePath, const plDelegate<void(const plFileStatus&, plStreamReader&)>& callback)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  plStringBuilder sAbsolutePath2(sAbsolutePath);
  sAbsolutePath2.MakeCleanPath();
  sAbsolutePath2.Trim(nullptr, "/");

  // try to read the asset file
  plFileReader file;
  if (file.Open(sAbsolutePath2) == PL_FAILURE)
  {
    MarkFileLocked(sAbsolutePath2);
    plLog::Error("Failed to open file '{0}'", sAbsolutePath2);
    return PL_FAILURE;
  }

  // Get model state.
  plFileStatus stat;
  {
    PL_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath2);
    if (!it.IsValid())
      return PL_FAILURE;

    stat = it.Value();
  }

  // Get current state.
  plFileStats statDep;
  if (plOSFile::GetFileStats(sAbsolutePath, statDep).Failed())
  {
    plLog::Error("Failed to retrieve file stats '{0}'", sAbsolutePath);
    return PL_FAILURE;
  }

  plDefaultMemoryStreamStorage storage;
  plMemoryStreamReader MemReader(&storage);
  MemReader.SetDebugSourceInformation(sAbsolutePath);

  plMemoryStreamWriter MemWriter(&storage);
  stat.m_LastModified = statDep.m_LastModificationTime;
  stat.m_Status = plFileStatus::Status::Valid;
  stat.m_uiHash = plFileSystemModel::HashFile(file, &MemWriter);

  if (callback.IsValid())
  {
    callback(stat, MemReader);
  }

  bool bFileChanged = false;
  {
    // Update state. No need to compare timestamps we hold a lock on the file via the reader.
    PL_LOCK(m_FilesMutex);
    auto it = m_ReferencedFiles.Find(sAbsolutePath2);
    if (it.IsValid())
    {
      bFileChanged = !it.Value().m_LastModified.Compare(stat.m_LastModified, plTimestamp::CompareMode::Identical);
      it.Value() = stat;
    }
    else
    {
      PL_REPORT_FAILURE("A file was removed from the model while we had a lock on it.");
    }

    if (bFileChanged)
    {
      FireFileChangedEvent(it.Key(), stat, plFileChangedEvent::Type::FileChanged);
    }
  }

  return PL_SUCCESS;
}

void plFileSystemModel::SetAllStatusUnknown()
{
  PL_LOCK(m_FilesMutex);
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Status = plFileStatus::Status::Unknown;
  }

  for (auto it = m_ReferencedFolders.GetIterator(); it.IsValid(); ++it)
  {
    it.Value() = plFileStatus::Status::Unknown;
  }
}


void plFileSystemModel::RemoveStaleFileInfos()
{
  plSet<plDataDirPath> unknownFiles;
  plSet<plDataDirPath> unknownFolders;
  {
    PL_LOCK(m_FilesMutex);
    for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
    {
      // search for files that existed previously but have not been found anymore recently
      if (it.Value().m_Status == plFileStatus::Status::Unknown)
      {
        unknownFiles.Insert(it.Key());
      }
    }
    for (auto it = m_ReferencedFolders.GetIterator(); it.IsValid(); ++it)
    {
      // search for folders that existed previously but have not been found anymore recently
      if (it.Value() == plFileStatus::Status::Unknown)
      {
        unknownFolders.Insert(it.Key());
      }
    }
  }

  for (const plDataDirPath& file : unknownFiles)
  {
    HandleSingleFile(file, false);
  }
  for (const plDataDirPath& folders : unknownFolders)
  {
    HandleSingleFile(folders, false);
  }
}


void plFileSystemModel::CheckFolder(plStringView sAbsolutePath)
{
  plStringBuilder sAbsolutePath2 = sAbsolutePath;
  sAbsolutePath2.MakeCleanPath();
  PL_ASSERT_DEV(plPathUtils::IsAbsolutePath(sAbsolutePath2), "Only absolute paths are supported for directory iteration.");
  sAbsolutePath2.Trim(nullptr, "/");

  if (sAbsolutePath2.IsEmpty())
    return;

  plDataDirPath folder(sAbsolutePath2, m_DataDirRoots);

  // We ignore any changes outside the model's data dirs.
  if (!folder.IsValid())
    return;

  bool bExists = false;
  {
    PL_LOCK(m_FilesMutex);
    bExists = m_ReferencedFolders.Contains(folder);
  }
  if (!bExists)
  {
    // If the folder does not exist yet we call NotifyOfChange which handles add / removal recursively as well.
    NotifyOfChange(folder);
    return;
  }

  plFileSystemIterator iterator;
  iterator.StartSearch(sAbsolutePath2, plFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);

  if (!iterator.IsValid())
    return;

  plStringBuilder sPath;

  plSet<plString> visitedFiles;
  plSet<plString> visitedFolders;
  visitedFolders.Insert(sAbsolutePath2);

  for (; iterator.IsValid(); iterator.Next())
  {
    sPath = iterator.GetCurrentPath();
    sPath.AppendPath(iterator.GetStats().m_sName);
    sPath.MakeCleanPath();
    if (iterator.GetStats().m_bIsDirectory)
      visitedFolders.Insert(sPath);
    else
      visitedFiles.Insert(sPath);

    plDataDirPath path(sPath, m_DataDirRoots, folder.GetDataDirIndex());
    HandleSingleFile(std::move(path), iterator.GetStats(), false);
  }

  plDynamicArray<plString> missingFiles;
  plDynamicArray<plString> missingFolders;

  {
    PL_LOCK(m_FilesMutex);

    // As we are using plCompareDataDirPath, entries of different casing interleave but we are only interested in the ones with matching casing so we skip the rest.
    for (auto it = m_ReferencedFiles.LowerBound(sAbsolutePath2.GetView()); it.IsValid(); ++it)
    {
      if (plPathUtils::IsSubPath(sAbsolutePath2, it.Key().GetAbsolutePath()) && !visitedFiles.Contains(it.Key().GetAbsolutePath()))
        missingFiles.PushBack(it.Key().GetAbsolutePath());
      if (!it.Key().GetAbsolutePath().StartsWith_NoCase(sAbsolutePath2))
        break;
    }

    for (auto it = m_ReferencedFolders.LowerBound(sAbsolutePath2.GetView()); it.IsValid(); ++it)
    {
      if (plPathUtils::IsSubPath(sAbsolutePath2, it.Key().GetAbsolutePath()) && !visitedFolders.Contains(it.Key().GetAbsolutePath()))
        missingFolders.PushBack(it.Key().GetAbsolutePath());
      if (!it.Key().GetAbsolutePath().StartsWith_NoCase(sAbsolutePath2))
        break;
    }
  }

  for (plString& sFile : missingFiles)
  {
    plDataDirPath path(std::move(sFile), m_DataDirRoots, folder.GetDataDirIndex());
    HandleSingleFile(std::move(path), false);
  }

  // Delete sub-folders before parent folders.
  missingFolders.Sort([](const plString& lhs, const plString& rhs) -> bool { return plStringUtils::Compare(lhs, rhs) > 0; });
  for (plString& sFolder : missingFolders)
  {
    plDataDirPath path(std::move(sFolder), m_DataDirRoots, folder.GetDataDirIndex());
    HandleSingleFile(std::move(path), false);
  }
}

void plFileSystemModel::OnAssetWatcherEvent(const plFileSystemWatcherEvent& e)
{
  switch (e.m_Type)
  {
    case plFileSystemWatcherEvent::Type::FileAdded:
    case plFileSystemWatcherEvent::Type::FileRemoved:
    case plFileSystemWatcherEvent::Type::FileChanged:
    case plFileSystemWatcherEvent::Type::DirectoryAdded:
    case plFileSystemWatcherEvent::Type::DirectoryRemoved:
      NotifyOfChange(e.m_sPath);
      break;
  }
}

plFileStatus plFileSystemModel::HandleSingleFile(plDataDirPath absolutePath, bool bRecurseIntoFolders)
{
  FILESYSTEM_PROFILE("HandleSingleFile");

  plFileStats Stats;
  const plResult statCheck = plOSFile::GetFileStats(absolutePath, Stats);

#  if PL_ENABLED(PL_PLATFORM_WINDOWS)
  if (statCheck.Succeeded() && Stats.m_sName != plPathUtils::GetFileNameAndExtension(absolutePath))
  {
    // Casing has changed.
    plStringBuilder sCorrectCasingPath = absolutePath.GetAbsolutePath();
    sCorrectCasingPath.ChangeFileNameAndExtension(Stats.m_sName);
    plDataDirPath correctCasingPath(sCorrectCasingPath.GetView(), m_DataDirRoots, absolutePath.GetDataDirIndex());
    // Add new casing
    plFileStatus res = HandleSingleFile(std::move(correctCasingPath), Stats, bRecurseIntoFolders);
    // Remove old casing
    RemoveFileOrFolder(absolutePath, bRecurseIntoFolders);
    return res;
  }
#  endif

  if (statCheck.Failed())
  {
    RemoveFileOrFolder(absolutePath, bRecurseIntoFolders);
    return {};
  }

  return HandleSingleFile(std::move(absolutePath), Stats, bRecurseIntoFolders);
}


plFileStatus plFileSystemModel::HandleSingleFile(plDataDirPath absolutePath, const plFileStats& FileStat, bool bRecurseIntoFolders)
{
  FILESYSTEM_PROFILE("HandleSingleFile2");

  if (FileStat.m_bIsDirectory)
  {
    plFileStatus status;
    status.m_Status = plFileStatus::Status::Valid;

    bool bExisted = false;
    {
      PL_LOCK(m_FilesMutex);
      auto it = m_ReferencedFolders.FindOrAdd(absolutePath, &bExisted);
      it.Value() = plFileStatus::Status::Valid;
    }

    if (!bExisted)
    {
      FireFolderChangedEvent(absolutePath, plFolderChangedEvent::Type::FolderAdded);
      if (bRecurseIntoFolders)
        CheckFolder(absolutePath);
    }

    return status;
  }
  else
  {
    plFileStatus status;
    bool bExisted = false;
    bool bFileChanged = false;
    {
      PL_LOCK(m_FilesMutex);
      auto it = m_ReferencedFiles.FindOrAdd(absolutePath, &bExisted);
      plFileStatus& value = it.Value();
      bFileChanged = !value.m_LastModified.Compare(FileStat.m_LastModificationTime, plTimestamp::CompareMode::Identical);
      if (bFileChanged)
      {
        value.m_uiHash = 0;
      }

      // If the state is unknown, we loaded it from the cache and need to fire FileChanged to update dependent systems.
      // #TODO_ASSET This behaviors should be changed once the asset cache is stored less lossy.
      bFileChanged |= value.m_Status == plFileStatus::Status::Unknown;
      // mark the file as valid (i.e. we saw it on disk, so it hasn't been deleted or such)
      value.m_Status = plFileStatus::Status::Valid;
      value.m_LastModified = FileStat.m_LastModificationTime;
      status = value;
    }

    if (!bExisted)
    {
      FireFileChangedEvent(absolutePath, status, plFileChangedEvent::Type::FileAdded);
    }
    else if (bFileChanged)
    {
      FireFileChangedEvent(absolutePath, status, plFileChangedEvent::Type::FileChanged);
    }
    return status;
  }
}

void plFileSystemModel::RemoveFileOrFolder(const plDataDirPath& absolutePath, bool bRecurseIntoFolders)
{
  plFileStatus fileStatus;
  bool bFileExisted = false;
  bool bFolderExisted = false;
  {
    PL_LOCK(m_FilesMutex);
    if (auto it = m_ReferencedFiles.Find(absolutePath); it.IsValid())
    {
      bFileExisted = true;
      fileStatus = it.Value();
      m_ReferencedFiles.Remove(it);
    }
    if (auto it = m_ReferencedFolders.Find(absolutePath); it.IsValid())
    {
      bFolderExisted = true;
      m_ReferencedFolders.Remove(it);
    }
  }

  if (bFileExisted)
  {
    FireFileChangedEvent(absolutePath, fileStatus, plFileChangedEvent::Type::FileRemoved);
  }

  if (bFolderExisted)
  {
    if (bRecurseIntoFolders)
    {
      plSet<plDataDirPath> previouslyKnownFiles;
      {
        FILESYSTEM_PROFILE("FindReferencedFiles");
        PL_LOCK(m_FilesMutex);
        auto itlowerBound = m_ReferencedFiles.LowerBound(absolutePath);
        while (itlowerBound.IsValid())
        {
          if (plPathUtils::IsSubPath(absolutePath, itlowerBound.Key().GetAbsolutePath()))
          {
            previouslyKnownFiles.Insert(itlowerBound.Key());
          }
          // As we are using plCompareDataDirPath, entries of different casing interleave but we are only interested in the ones with matching casing so we skip the rest.
          if (!itlowerBound.Key().GetAbsolutePath().StartsWith_NoCase(absolutePath.GetAbsolutePath()))
          {
            break;
          }
          ++itlowerBound;
        }
      }
      {
        FILESYSTEM_PROFILE("HandleRemovedFiles");
        for (const plDataDirPath& file : previouslyKnownFiles)
        {
          RemoveFileOrFolder(file, false);
        }
      }
    }
    FireFolderChangedEvent(absolutePath, plFolderChangedEvent::Type::FolderRemoved);
  }
}

void plFileSystemModel::MarkFileLocked(plStringView sAbsolutePath)
{
  PL_LOCK(m_FilesMutex);
  auto it = m_ReferencedFiles.Find(sAbsolutePath);
  if (it.IsValid())
  {
    it.Value().m_Status = plFileStatus::Status::FileLocked;
    m_LockedFiles.Insert(sAbsolutePath);
  }
}

void plFileSystemModel::FireFileChangedEvent(const plDataDirPath& file, plFileStatus fileStatus, plFileChangedEvent::Type type)
{
  // We queue up all requests on a thread and only return once the list is empty. The reason for this is that:
  // A: We don't want to allow recursive event calling as it creates limbo states in the model and hard to debug bugs.
  // B: If a user calls NotifyOfChange, the function should only return if the event and any indirect events that were triggered by the event handlers have been processed.

  plFileChangedEvent& e = g_PostponedFiles.ExpandAndGetRef();
  e.m_Path = file;
  e.m_Status = fileStatus;
  e.m_Type = type;

  if (g_bInFileBroadcast)
  {
    return;
  }

  g_bInFileBroadcast = true;
  PL_SCOPE_EXIT(g_bInFileBroadcast = false);

  for (plUInt32 i = 0; i < g_PostponedFiles.GetCount(); i++)
  {
    // Need to make a copy as new elements can be added and the array resized during broadcast.
    plFileChangedEvent tempEvent = std::move(g_PostponedFiles[i]);
    m_FileChangedEvents.Broadcast(tempEvent);
  }
  g_PostponedFiles.Clear();
}

void plFileSystemModel::FireFolderChangedEvent(const plDataDirPath& file, plFolderChangedEvent::Type type)
{
  // See comment in FireFileChangedEvent.
  plFolderChangedEvent& e = g_PostponedFolders.ExpandAndGetRef();
  e.m_Path = file;
  e.m_Type = type;

  if (g_bInFolderBroadcast)
  {
    return;
  }

  g_bInFolderBroadcast = true;
  PL_SCOPE_EXIT(g_bInFolderBroadcast = false);

  for (plUInt32 i = 0; i < g_PostponedFolders.GetCount(); i++)
  {
    // Need to make a copy as new elements can be added and the array resized during broadcast.
    plFolderChangedEvent tempEvent = std::move(g_PostponedFolders[i]);
    m_FolderChangedEvents.Broadcast(tempEvent);
  }
  g_PostponedFolders.Clear();
}

#endif
