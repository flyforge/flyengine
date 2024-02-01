#pragma once

#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>

#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>

/// \internal Internal task worker thread class.
class plTaskWorkerThread final : public plThread
{
  PL_DISALLOW_COPY_AND_ASSIGN(plTaskWorkerThread);

  /// \name Execution
  ///@{

public:
  /// \brief Tells the worker thread what tasks to execute and which thread index it has.
  plTaskWorkerThread(plWorkerThreadType::Enum threadType, plUInt32 uiThreadNumber);
  ~plTaskWorkerThread();

  /// \brief Deactivates the thread. Returns failure, if the thread is currently still running.
  plResult DeactivateWorker();

private:
  // Which types of tasks this thread should work on.
  plWorkerThreadType::Enum m_WorkerType;

  // Whether the thread is supposed to continue running.
  volatile bool m_bActive = true;

  // For display purposes.
  plUInt16 m_uiWorkerThreadNumber = 0xFFFF;

  ///@}

  /// \name Thread Utilization
  ///@{

public:
  /// \brief Returns the last utilization value (0 - 1 range). Optionally returns how many tasks it executed recently.
  double GetThreadUtilization(plUInt32* pNumTasksExecuted = nullptr);

  /// \brief Computes the thread utilization by dividing the thread active time by the time that has passed since the last update.
  void UpdateThreadUtilization(plTime timePassed);

private:
  bool m_bExecutingTask = false;
  plUInt16 m_uiLastNumTasksExecuted = 0;
  plUInt16 m_uiNumTasksExecuted = 0;
  plTime m_StartedWorkingTime;
  plTime m_ThreadActiveTime;
  double m_fLastThreadUtilization = 0.0;

  ///@}

  /// \name Idle State
  ///@{

public:
  /// \brief If the thread is currently idle, this will wake it up and return PL_SUCCESS.
  plTaskWorkerState WakeUpIfIdle();

private:
  // Puts the thread to sleep (idle state)
  void WaitForWork();

  virtual plUInt32 Run() override;

  // used to wake up idle threads, see m_WorkerState
  plThreadSignal m_WakeUpSignal;

  // used to indicate whether this thread is currently idle
  // if so, it can be woken up using m_WakeUpSignal
  // plAtomicBool m_bIsIdle = false;
  plAtomicInteger32 m_iWorkerState; // plTaskWorkerState

  ///@}
};

/// \internal Thread local state used by the task system (and for better debugging)
struct plTaskWorkerInfo
{
  plWorkerThreadType::Enum m_WorkerType = plWorkerThreadType::Unknown;
  bool m_bAllowNestedTasks = true;
  plInt32 m_iWorkerIndex = -1;
  const char* m_szTaskName = nullptr;
  plAtomicInteger32* m_pWorkerState = nullptr;
};

extern thread_local plTaskWorkerInfo tl_TaskWorkerInfo;
