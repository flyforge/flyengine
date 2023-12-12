#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/TaskSystem.h>

struct plAssetCuratorEvent;
class plTask;
struct plAssetInfo;

/// \brief Creates a file system watcher for the given filesystem config and informs the plAssetCurator
/// of any changes.
class PLASMA_EDITORFRAMEWORK_DLL plAssetWatcher
{
public:
  plAssetWatcher(const plApplicationFileSystemConfig& fileSystemConfig);
  ~plAssetWatcher();

  /// \brief Needs to be called every frame. Handles update delays to allow compacting multiple changes.
  void MainThreadTick();

private:
  friend class plDirectoryUpdateTask;
  friend class plAssetCurator;
  struct WatcherResult
  {
    plString sFile;
    plDirectoryWatcherAction action;
    plDirectoryWatcherType type;
  };

  static constexpr plUInt32 s_FrameDelay = 5;
  struct PendingUpdate
  {
    plString sAbsPath;
    plUInt32 m_uiFrameDelay = s_FrameDelay;
  };


  void HandleWatcherChange(const WatcherResult& res);
  void UpdateFile(const char* szAbsPath);
  void UpdateDirectory(const char* szAbsPath);

private:
  mutable plMutex m_WatcherMutex;
  plHybridArray<plDirectoryWatcher*, 6> m_Watchers;
  plSharedPtr<plTask> m_pWatcherTask;
  plTaskGroupID m_WatcherGroup;
  plAtomicBool m_bShutdown = false;

  plHybridArray<plTaskGroupID, 4> m_DirectoryUpdates;
  plHybridArray<PendingUpdate, 4> m_UpdateFile;
  plHybridArray<PendingUpdate, 4> m_UpdateDirectory;

  plApplicationFileSystemConfig m_FileSystemConfig;
};

/// \brief Task to scan a directory and inform the plAssetCurator of any changes.
class plDirectoryUpdateTask final : public plTask
{
public:
  plDirectoryUpdateTask(plAssetWatcher* pWatcher, const char* szFolder);
  ~plDirectoryUpdateTask();

  plAssetWatcher* m_pWatcher = nullptr;
  plString m_sFolder;

private:
  virtual void Execute() override;
};
