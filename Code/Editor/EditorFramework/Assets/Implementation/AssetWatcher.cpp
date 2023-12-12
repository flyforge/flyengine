#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetWatcher.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

////////////////////////////////////////////////////////////////////////
// plAssetWatcher
////////////////////////////////////////////////////////////////////////

plAssetWatcher::plAssetWatcher(const plApplicationFileSystemConfig& fileSystemConfig)
{
  PLASMA_PROFILE_SCOPE("plAssetWatcher");
  m_FileSystemConfig = fileSystemConfig;
  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    plStringBuilder sTemp;
    if (plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
    {
      plLog::Error("Failed to init directory watcher for dir '{0}'", dd.m_sDataDirSpecialPath);
      continue;
    }

    plDirectoryWatcher* pWatcher = PLASMA_DEFAULT_NEW(plDirectoryWatcher);
    plResult res =
      pWatcher->OpenDirectory(sTemp, plDirectoryWatcher::Watch::Deletes | plDirectoryWatcher::Watch::Writes | plDirectoryWatcher::Watch::Creates |
                                       plDirectoryWatcher::Watch::Renames | plDirectoryWatcher::Watch::Subdirectories);

    if (res.Failed())
    {
      PLASMA_DEFAULT_DELETE(pWatcher);
      plLog::Error("Failed to init directory watcher for dir '{0}'", sTemp);
      continue;
    }

    m_Watchers.PushBack(pWatcher);
  }

  m_pWatcherTask = PLASMA_DEFAULT_NEW(plDelegateTask<void>, "Watcher Update", [this]() {
    plHybridArray<WatcherResult, 16> watcherResults;
    for (plDirectoryWatcher* pWatcher : m_Watchers)
    {
      pWatcher->EnumerateChanges([pWatcher, &watcherResults](plStringView sFilename, plDirectoryWatcherAction action, plDirectoryWatcherType type) {
        watcherResults.PushBack({sFilename, action, type});
      });
    }
    for (const WatcherResult& res : watcherResults)
    {
      HandleWatcherChange(res);
    }
  });
}


plAssetWatcher::~plAssetWatcher()
{
  m_bShutdown = true;
  plTaskGroupID watcherGroup;
  {
    PLASMA_LOCK(m_WatcherMutex);
    watcherGroup = m_WatcherGroup;
  }
  plTaskSystem::WaitForGroup(watcherGroup);
  {
    PLASMA_LOCK(m_WatcherMutex);
    m_pWatcherTask.Clear();
    for (plDirectoryWatcher* pWatcher : m_Watchers)
    {
      PLASMA_DEFAULT_DELETE(pWatcher);
    }
    m_Watchers.Clear();
  }

  do
  {
    plTaskGroupID id;
    {
      PLASMA_LOCK(m_WatcherMutex);
      if (m_DirectoryUpdates.IsEmpty())
        break;
      id = m_DirectoryUpdates.PeekBack();
    }
    plTaskSystem::WaitForGroup(id);
  } while (true);

  PLASMA_LOCK(m_WatcherMutex);
  PLASMA_ASSERT_DEV(m_DirectoryUpdates.IsEmpty(), "All directory updates should have finished.");
}

void plAssetWatcher::MainThreadTick()
{
  PLASMA_PROFILE_SCOPE("plAssetWatcherTick");
  PLASMA_LOCK(m_WatcherMutex);
  if (!m_bShutdown && m_pWatcherTask && plTaskSystem::IsTaskGroupFinished(m_WatcherGroup))
  {
    m_WatcherGroup = plTaskSystem::StartSingleTask(m_pWatcherTask, plTaskPriority::LongRunningHighPriority);
  }

  // Files
  plAssetCurator* pCurator = plAssetCurator::GetSingleton();

  for (plUInt32 i = m_UpdateFile.GetCount(); i > 0; --i)
  {
    PendingUpdate& update = m_UpdateFile[i - 1];
    --update.m_uiFrameDelay;
    if (update.m_uiFrameDelay == 0)
    {
      pCurator->NotifyOfFileChange(update.sAbsPath);
      m_UpdateFile.RemoveAtAndSwap(i - 1);
    }
  }

  // Directories
  for (plUInt32 i = m_UpdateDirectory.GetCount(); i > 0; --i)
  {
    PendingUpdate& update = m_UpdateDirectory[i - 1];
    --update.m_uiFrameDelay;
    if (update.m_uiFrameDelay == 0 && !m_bShutdown)
    {
      plSharedPtr<plTask> pTask = PLASMA_DEFAULT_NEW(plDirectoryUpdateTask, this, update.sAbsPath);
      plTaskGroupID id = plTaskSystem::StartSingleTask(pTask, plTaskPriority::LongRunningHighPriority, [this](plTaskGroupID id) {
        PLASMA_LOCK(m_WatcherMutex);
        m_DirectoryUpdates.RemoveAndSwap(id);
      });
      m_DirectoryUpdates.PushBack(id);

      m_UpdateDirectory.RemoveAtAndSwap(i - 1);
    }
  }
}

void plAssetWatcher::HandleWatcherChange(const WatcherResult& res)
{
  plAssetCurator* pCurator = plAssetCurator::GetSingleton();
  plFileStats stat;
  plResult stats = plOSFile::GetFileStats(res.sFile, stat);
  bool isFileKnown = false;
  {
    PLASMA_LOCK(pCurator->m_CuratorMutex);
    isFileKnown = pCurator->m_ReferencedFiles.Find(res.sFile).IsValid();
  }

  switch (res.action)
  {
    case plDirectoryWatcherAction::None:
      PLASMA_ASSERT_DEV(false, "None event should never happen");
      break;
    case plDirectoryWatcherAction::Added:
    {
      if (stats == PLASMA_SUCCESS)
      {
        if (stat.m_bIsDirectory)
        {
          UpdateDirectory(res.sFile);
        }
        else
        {
          UpdateFile(res.sFile);
        }
      }
    }
    break;
    case plDirectoryWatcherAction::Removed:
    {
      if (isFileKnown)
      {
        UpdateFile(res.sFile);
      }
      else
      {
        UpdateDirectory(res.sFile);
      }
    }
    break;
    case plDirectoryWatcherAction::Modified:
      if (stats == PLASMA_SUCCESS)
      {
        if (stat.m_bIsDirectory)
        {
          // TODO: Can directories be modified?
        }
        else
        {
          UpdateFile(res.sFile);
        }
      }
      break;
    case plDirectoryWatcherAction::RenamedOldName:
      // Ignore, we scan entire parent dir in the following RenamedNewName event.
      break;
    case plDirectoryWatcherAction::RenamedNewName:
    {
      // Rescan parent directory.
      // TODO: Renames on root will rescan the entire data dir.
      // However, fixing this would require dynamic recursion if we detect that
      // a folder was actually renamed.
      plStringBuilder sParentFolder = res.sFile;
      sParentFolder.PathParentDirectory();
      UpdateDirectory(sParentFolder);
    }
    break;
  }
}

void plAssetWatcher::UpdateFile(const char* szAbsPath)
{
  PLASMA_LOCK(m_WatcherMutex);
  for (PendingUpdate& update : m_UpdateFile)
  {
    if (update.sAbsPath == szAbsPath)
    {
      update.m_uiFrameDelay = s_FrameDelay;
      return;
    }
  }
  PendingUpdate& update = m_UpdateFile.ExpandAndGetRef();
  update.m_uiFrameDelay = s_FrameDelay;
  update.sAbsPath = szAbsPath;
}

void plAssetWatcher::UpdateDirectory(const char* szAbsPath)
{
  plStringBuilder sAbsPath = szAbsPath;
  PLASMA_LOCK(m_WatcherMutex);
  for (PendingUpdate& update : m_UpdateDirectory)
  {
    // No need to add this directory if itself or a parent directory is already queued.
    if (sAbsPath.IsPathBelowFolder(update.sAbsPath))
    {
      // Reset delay as we are probably doing a bigger operation right now.
      update.m_uiFrameDelay = s_FrameDelay;
      return;
    }
  }
  PendingUpdate& update = m_UpdateDirectory.ExpandAndGetRef();
  update.m_uiFrameDelay = s_FrameDelay;
  update.sAbsPath = sAbsPath;
}

////////////////////////////////////////////////////////////////////////
// plDirectoryUpdateTask
////////////////////////////////////////////////////////////////////////

plDirectoryUpdateTask::plDirectoryUpdateTask(plAssetWatcher* pWatcher, const char* szFolder)
  : m_pWatcher(pWatcher)
  , m_sFolder(szFolder)
{
  ConfigureTask("plDirectoryUpdateTask", plTaskNesting::Never);
}

plDirectoryUpdateTask::~plDirectoryUpdateTask() {}

void plDirectoryUpdateTask::Execute()
{
  plAssetCurator* pCurator = plAssetCurator::GetSingleton();
  plSet<plString> previouslyKnownFiles;
  {
    CURATOR_PROFILE("FindReferencedFiles");
    // Find all currently known files that are under the given folder.
    // TODO: is m_InverseDependency / m_InverseReferences covered by this?
    // TODO: What about asset output files?
    PLASMA_LOCK(pCurator->m_CuratorMutex);
    auto itlowerBound = pCurator->m_ReferencedFiles.LowerBound(m_sFolder);
    while (itlowerBound.IsValid() && itlowerBound.Key().StartsWith_NoCase(m_sFolder))
    {
      previouslyKnownFiles.Insert(itlowerBound.Key());
      ++itlowerBound;
    }
  }

  {
    CURATOR_PROFILE("IterateDataDirectory");
    // Iterate folder to find all actually existing files on disk.
    plSet<plString> knownFiles;
    pCurator->IterateDataDirectory(m_sFolder, &knownFiles);

    // Not encountered files are now removed which the plAssetCurator must be informed about.
    previouslyKnownFiles.Difference(knownFiles);
  }

  CURATOR_PROFILE("HandleRemovedFiles");
  for (const plString& sFile : previouslyKnownFiles)
  {
    pCurator->HandleSingleFile(sFile);
  }
}
