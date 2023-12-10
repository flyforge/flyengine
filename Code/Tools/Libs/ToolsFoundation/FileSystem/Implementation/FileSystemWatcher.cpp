#include <ToolsFoundation/ToolsFoundationDLL.h>

#if PLASMA_ENABLED(PLASMA_SUPPORTS_DIRECTORY_WATCHER)

#  include <ToolsFoundation/FileSystem/FileSystemWatcher.h>

#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Threading/DelegateTask.h>

////////////////////////////////////////////////////////////////////////
// plAssetWatcher
////////////////////////////////////////////////////////////////////////

plFileSystemWatcher::plFileSystemWatcher(const plApplicationFileSystemConfig& fileSystemConfig)
{
  m_FileSystemConfig = fileSystemConfig;
}


plFileSystemWatcher::~plFileSystemWatcher() = default;

void plFileSystemWatcher::Initialize()
{
  PLASMA_PROFILE_SCOPE("Initialize");

  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    plStringBuilder sTemp;
    if (plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
    {
      plLog::Error("Failed to init directory watcher for dir '{0}'", dd.m_sDataDirSpecialPath);
      continue;
    }

    plDirectoryWatcher* pWatcher = PLASMA_DEFAULT_NEW(plDirectoryWatcher);
    plResult res = pWatcher->OpenDirectory(sTemp, plDirectoryWatcher::Watch::Deletes | plDirectoryWatcher::Watch::Writes | plDirectoryWatcher::Watch::Creates | plDirectoryWatcher::Watch::Renames | plDirectoryWatcher::Watch::Subdirectories);

    if (res.Failed())
    {
      PLASMA_DEFAULT_DELETE(pWatcher);
      plLog::Error("Failed to init directory watcher for dir '{0}'", sTemp);
      continue;
    }

    m_Watchers.PushBack(pWatcher);
  }

  m_pWatcherTask = PLASMA_DEFAULT_NEW(plDelegateTask<void>, "Watcher Changes", plTaskNesting::Never, [this]() {
    plHybridArray<WatcherResult, 16> watcherResults;
    for (plDirectoryWatcher* pWatcher : m_Watchers)
    {
      pWatcher->EnumerateChanges([pWatcher, &watcherResults](plStringView sFilename, plDirectoryWatcherAction action, plDirectoryWatcherType type) { watcherResults.PushBack({sFilename, action, type}); });
    }
    for (const WatcherResult& res : watcherResults)
    {
      HandleWatcherChange(res);
    } //
  });
  // This is a separate task as these trigger callbacks which can potentially take a long time and we can't have the watcher changes task be blocked for so long or notifications might get lost.
  m_pNotifyTask = PLASMA_DEFAULT_NEW(plDelegateTask<void>, "Watcher Notify", plTaskNesting::Never, [this]() { NotifyChanges(); });
}

void plFileSystemWatcher::Deinitialize()
{
  m_bShutdown = true;
  plTaskGroupID watcherGroup;
  plTaskGroupID notifyGroup;
  {
    PLASMA_LOCK(m_WatcherMutex);
    watcherGroup = m_WatcherGroup;
    notifyGroup = m_NotifyGroup;
  }
  plTaskSystem::WaitForGroup(watcherGroup);
  plTaskSystem::WaitForGroup(notifyGroup);
  {
    PLASMA_LOCK(m_WatcherMutex);
    m_pWatcherTask.Clear();
    m_pNotifyTask.Clear();
    for (plDirectoryWatcher* pWatcher : m_Watchers)
    {
      PLASMA_DEFAULT_DELETE(pWatcher);
    }
    m_Watchers.Clear();
  }
}

void plFileSystemWatcher::MainThreadTick()
{
  PLASMA_PROFILE_SCOPE("plAssetWatcherTick");
  PLASMA_LOCK(m_WatcherMutex);
  if (!m_bShutdown && m_pWatcherTask && plTaskSystem::IsTaskGroupFinished(m_WatcherGroup))
  {
    m_WatcherGroup = plTaskSystem::StartSingleTask(m_pWatcherTask, plTaskPriority::LongRunningHighPriority);
  }
  if (!m_bShutdown && m_pNotifyTask && plTaskSystem::IsTaskGroupFinished(m_NotifyGroup))
  {
    m_NotifyGroup = plTaskSystem::StartSingleTask(m_pNotifyTask, plTaskPriority::LongRunningHighPriority);
  }
}


void plFileSystemWatcher::NotifyChanges()
{
  auto NotifyChange = [this](const plString& sAbsPath, plFileSystemWatcherEvent::Type type) {
    plFileSystemWatcherEvent e;
    e.m_sPath = sAbsPath;
    e.m_Type = type;
    m_Events.Broadcast(e);
  };

  // Files
  ConsumeEntry(m_FileAdded, plFileSystemWatcherEvent::Type::FileAdded, NotifyChange);
  ConsumeEntry(m_FileChanged, plFileSystemWatcherEvent::Type::FileChanged, NotifyChange);
  ConsumeEntry(m_FileRemoved, plFileSystemWatcherEvent::Type::FileRemoved, NotifyChange);

  // Directories
  ConsumeEntry(m_DirectoryAdded, plFileSystemWatcherEvent::Type::DirectoryAdded, NotifyChange);
  ConsumeEntry(m_DirectoryRemoved, plFileSystemWatcherEvent::Type::DirectoryRemoved, NotifyChange);
}

void plFileSystemWatcher::HandleWatcherChange(const WatcherResult& res)
{
  switch (res.m_Action)
  {
    case plDirectoryWatcherAction::None:
      PLASMA_ASSERT_DEV(false, "None event should never happen");
      break;
    case plDirectoryWatcherAction::RenamedNewName:
    case plDirectoryWatcherAction::Added:
    {
      if (res.m_Type == plDirectoryWatcherType::Directory)
      {
        AddEntry(m_DirectoryAdded, res.m_sFile, s_AddedFrameDelay);
      }
      else
      {
        AddEntry(m_FileAdded, res.m_sFile, s_AddedFrameDelay);
      }
    }
    break;
    case plDirectoryWatcherAction::RenamedOldName:
    case plDirectoryWatcherAction::Removed:
    {
      if (res.m_Type == plDirectoryWatcherType::Directory)
      {
        AddEntry(m_DirectoryRemoved, res.m_sFile, s_RemovedFrameDelay);
      }
      else
      {
        AddEntry(m_FileRemoved, res.m_sFile, s_RemovedFrameDelay);
      }
    }
    break;
    case plDirectoryWatcherAction::Modified:
    {
      if (res.m_Type == plDirectoryWatcherType::Directory)
      {
        // Can a directory even be modified? In any case, we ignore this change.
        // UpdateEntry(m_DirectoryRemoved, res.sFile, s_RemovedFrameDelay);
      }
      else
      {
        AddEntry(m_FileChanged, res.m_sFile, s_ModifiedFrameDelay);
      }
    }
    break;
  }
}

void plFileSystemWatcher::AddEntry(plDynamicArray<PendingUpdate>& container, const plStringView sAbsPath, plUInt32 uiFrameDelay)
{
  PLASMA_LOCK(m_WatcherMutex);
  for (PendingUpdate& update : container)
  {
    if (update.m_sAbsPath == sAbsPath)
    {
      update.m_uiFrameDelay = uiFrameDelay;
      return;
    }
  }
  PendingUpdate& update = container.ExpandAndGetRef();
  update.m_uiFrameDelay = uiFrameDelay;
  update.m_sAbsPath = sAbsPath;
}

void plFileSystemWatcher::ConsumeEntry(plDynamicArray<PendingUpdate>& container, plFileSystemWatcherEvent::Type type, const plDelegate<void(const plString& sAbsPath, plFileSystemWatcherEvent::Type type)>& consume)
{
  plHybridArray<PendingUpdate, 16> updates;
  {
    PLASMA_LOCK(m_WatcherMutex);
    for (plUInt32 i = container.GetCount(); i > 0; --i)
    {
      PendingUpdate& update = container[i - 1];
      --update.m_uiFrameDelay;
      if (update.m_uiFrameDelay == 0)
      {
        updates.PushBack(update);
        container.RemoveAtAndSwap(i - 1);
      }
    }
  }
  for (const PendingUpdate& update : updates)
  {
    consume(update.m_sAbsPath, type);
  }
}

#endif
