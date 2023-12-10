#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#if PLASMA_ENABLED(PLASMA_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/Application/Config/FileSystemConfig.h>
#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/Threading/TaskSystem.h>

class plTask;

/// \brief Event fired by plFileSystemWatcher::m_Events.
struct plFileSystemWatcherEvent
{
  enum class Type
  {
    FileAdded,
    FileRemoved,
    FileChanged,
    DirectoryAdded,
    DirectoryRemoved,
  };

  plStringView m_sPath;
  Type m_Type;
};

/// \brief Creates a file system watcher for the given filesystem config and fires any changes on a worker task via an event.
class PLASMA_TOOLSFOUNDATION_DLL plFileSystemWatcher
{
public:
  plFileSystemWatcher(const plApplicationFileSystemConfig& fileSystemConfig);
  ~plFileSystemWatcher();

  /// \brief Once called, file system watchers are created for each data directory and changes are observed.
  void Initialize();

  /// \brief Waits for all pending tasks to complete and then stops observing changes and destroys file system watchers.
  void Deinitialize();

  /// \brief Needs to be called at regular intervals (e.g. each frame) to restart background tasks.
  void MainThreadTick();

public:
  plEvent<const plFileSystemWatcherEvent&, plMutex> m_Events;

private:
  // On file move / rename operations we want the new file to be seen first before the old file delete event so that we can correctly detect this as a move instead of a delete operation. We achieve this by delaying each event by a fixed number of frames.
  static constexpr plUInt32 s_AddedFrameDelay = 5;
  static constexpr plUInt32 s_RemovedFrameDelay = 10;
  // Sometimes moving a file triggers a modified event on the old file. To prevent this from triggering the removal to be seen before the addition, we also delay modified events by the same amount as remove events.
  static constexpr plUInt32 s_ModifiedFrameDelay = 10;

  struct WatcherResult
  {
    plString m_sFile;
    plDirectoryWatcherAction m_Action;
    plDirectoryWatcherType m_Type;
  };

  struct PendingUpdate
  {
    plString m_sAbsPath;
    plUInt32 m_uiFrameDelay = 0;
  };

  /// \brief Handles a single change notification by a directory watcher.
  void HandleWatcherChange(const WatcherResult& res);
  /// \brief Handles update delays to allow compacting multiple changes.
  void NotifyChanges();
  /// \brief Adds a change with the given delay to the container. If the entry is already present, only its delay is increased.
  void AddEntry(plDynamicArray<PendingUpdate>& container, const plStringView sAbsPath, plUInt32 uiFrameDelay);
  /// \brief Reduces the delay counter of every item in the container. If a delay reaches zero, it is removed and the callback is fired.
  void ConsumeEntry(plDynamicArray<PendingUpdate>& container, plFileSystemWatcherEvent::Type type, const plDelegate<void(const plString& sAbsPath, plFileSystemWatcherEvent::Type type)>& consume);

private:
  // Immutable data after StartInitialize
  plApplicationFileSystemConfig m_FileSystemConfig;

  // Watchers
  mutable plMutex m_WatcherMutex;
  plHybridArray<plDirectoryWatcher*, 6> m_Watchers;
  plSharedPtr<plTask> m_pWatcherTask;
  plSharedPtr<plTask> m_pNotifyTask;
  plTaskGroupID m_WatcherGroup;
  plTaskGroupID m_NotifyGroup;
  plAtomicBool m_bShutdown = false;

  // Pending ops
  plHybridArray<PendingUpdate, 4> m_FileAdded;
  plHybridArray<PendingUpdate, 4> m_FileRemoved;
  plHybridArray<PendingUpdate, 4> m_FileChanged;
  plHybridArray<PendingUpdate, 4> m_DirectoryAdded;
  plHybridArray<PendingUpdate, 4> m_DirectoryRemoved;
};

#endif
