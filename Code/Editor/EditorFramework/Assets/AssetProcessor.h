#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/FileSystem/DataDirPath.h>
#include <atomic>

struct plAssetCuratorEvent;
class plTask;
struct plAssetInfo;

/// \brief Log for all background processing results
class plAssetProcessorLog : public plLogInterface
{
public:
  virtual void HandleLogMessage(const plLoggingEventData& le) override;
  void AddLogWriter(plLoggingEvent::Handler handler);
  void RemoveLogWriter(plLoggingEvent::Handler handler);

  plLoggingEvent m_LoggingEvent;
};

struct plAssetProcessorEvent
{
  enum class Type
  {
    ProcessTaskStateChanged
  };

  Type m_Type;
};


class plProcessThread : public plThread
{
public:
  plProcessThread()
    : plThread("plProcessThread")
  {
  }


  virtual plUInt32 Run() override;
};

class plProcessTask
{
public:
  plProcessTask();
  ~plProcessTask();

  plAtomicInteger32 m_bDidWork = true;
  plUInt32 m_uiProcessorID;

  bool BeginExecute();

  bool FinishExecute();

  void ShutdownProcess();

private:
  void StartProcess();
  void EventHandlerIPC(const plProcessCommunicationChannel::Event& e);

  bool GetNextAssetToProcess(plAssetInfo* pInfo, plUuid& out_guid, plDataDirPath& out_path);
  bool GetNextAssetToProcess(plUuid& out_guid, plDataDirPath& out_path);
  void OnProcessCrashed();


  plUuid m_AssetGuid;
  plUInt64 m_uiAssetHash = 0;
  plUInt64 m_uiThumbHash = 0;
  plDataDirPath m_AssetPath;
  plEditorProcessCommunicationChannel* m_pIPC;
  bool m_bProcessShouldBeRunning = false;
  bool m_bProcessCrashed = false;
  bool m_bWaiting = false;
  plTransformStatus m_Status;
  plDynamicArray<plLogEntry> m_LogEntries;
  plDynamicArray<plString> m_TransitiveHull;
};

/// \brief Background asset processing is handled by this class.
/// Creates EditorProcessor processes.
class PLASMA_EDITORFRAMEWORK_DLL plAssetProcessor
{
  PLASMA_DECLARE_SINGLETON(plAssetProcessor);

public:
  enum class ProcessTaskState : plUInt8
  {
    Stopped,  ///< No EditorProcessor or the process thread is running.
    Running,  ///< Everything is active.
    Stopping, ///< Everything is still running but no new tasks are put into the EditorProcessors.
  };

  plAssetProcessor();
  ~plAssetProcessor();

  void StartProcessTask();
  void StopProcessTask(bool bForce);
  ProcessTaskState GetProcessTaskState() const
  {
    return m_ProcessTaskState;
  }

  void AddLogWriter(plLoggingEvent::Handler handler);
  void RemoveLogWriter(plLoggingEvent::Handler handler);

public:
  // Can be called from worker threads!
  plEvent<const plAssetProcessorEvent&> m_Events;

private:
  friend class plProcessTask;
  friend class plProcessThread;
  friend class plAssetCurator;

  void Run();

private:
  plAssetProcessorLog m_CuratorLog;

  // Process thread and its state
  plUniquePtr<plProcessThread> m_pThread;
  std::atomic<bool> m_bForceStop = false; ///< If set, background processes will be killed when stopping without waiting for their current task to finish.

  // Locks writes to m_ProcessTaskState to make sure the state machine does not go from running to stopped before having fired stopping.
  mutable plMutex m_ProcessorMutex;
  std::atomic<ProcessTaskState> m_ProcessTaskState = ProcessTaskState::Stopped;

  // Data owned by the process thread.
  plDynamicArray<bool> m_ProcessRunning;
  plDynamicArray<plProcessTask> m_ProcessTasks;
};
