#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

thread_local plTaskWorkerInfo tl_TaskWorkerInfo;

static const char* GenerateThreadName(plWorkerThreadType::Enum threadType, plUInt32 uiThreadNumber)
{
  static plStringBuilder sTemp;
  sTemp.Format("{} {}", plWorkerThreadType::GetThreadTypeName(threadType), uiThreadNumber);
  return sTemp.GetData();
}

plTaskWorkerThread::plTaskWorkerThread(plWorkerThreadType::Enum threadType, plUInt32 uiThreadNumber)
  // We need at least 256 kb of stack size, otherwise the shader compilation tasks will run out of stack space.
  : plThread(GenerateThreadName(threadType, uiThreadNumber), 256 * 1024)
{
  m_WorkerType = threadType;
  m_uiWorkerThreadNumber = uiThreadNumber & 0xFFFF;
}

plTaskWorkerThread::~plTaskWorkerThread() = default;

plResult plTaskWorkerThread::DeactivateWorker()
{
  m_bActive = false;

  if (GetThreadStatus() != plThread::Finished)
  {
    // if necessary, wake this thread up
    WakeUpIfIdle();

    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plUInt32 plTaskWorkerThread::Run()
{
  PLASMA_ASSERT_DEBUG(
    m_WorkerType != plWorkerThreadType::Unknown && m_WorkerType != plWorkerThreadType::MainThread, "Worker threads cannot use this type");
  PLASMA_ASSERT_DEBUG(m_WorkerType < plWorkerThreadType::ENUM_COUNT, "Worker Thread Type is invalid: {0}", m_WorkerType);

  // once this thread is running, store the worker type in the thread_local variable
  // such that the plTaskSystem is able to look this up (e.g. in WaitForGroup) to know which types of tasks to help with
  tl_TaskWorkerInfo.m_WorkerType = m_WorkerType;
  tl_TaskWorkerInfo.m_iWorkerIndex = m_uiWorkerThreadNumber;
  tl_TaskWorkerInfo.m_pWorkerState = &m_iWorkerState;

  const bool bIsReserve = m_uiWorkerThreadNumber >= plTaskSystem::s_pThreadState->m_uiMaxWorkersToUse[m_WorkerType];

  plTaskPriority::Enum FirstPriority;
  plTaskPriority::Enum LastPriority;
  plTaskSystem::DetermineTasksToExecuteOnThread(FirstPriority, LastPriority);

  m_bExecutingTask = false;

  while (m_bActive)
  {
    if (!m_bExecutingTask)
    {
      m_bExecutingTask = true;
      m_StartedWorkingTime = plTime::Now();
    }

    if (!plTaskSystem::ExecuteTask(FirstPriority, LastPriority, false, plTaskGroupID(), &m_iWorkerState))
    {
      WaitForWork();
    }
    else
    {
      ++m_uiNumTasksExecuted;

      if (bIsReserve)
      {
        PLASMA_VERIFY(m_iWorkerState.Set((int)plTaskWorkerState::Idle) == (int)plTaskWorkerState::Active, "Corrupt worker state");

        // if this thread is part of the reserve, then don't continue to process tasks indefinitely
        // instead, put this thread to sleep and wake up someone else
        // that someone else may be a thread at the front of the queue, it may also turn out to be this thread again
        // either way, if at some point we woke up more threads than the maximum desired, this will move the active threads
        // to the front of the list, because of the way plTaskSystem::WakeUpThreads() works
        plTaskSystem::WakeUpThreads(m_WorkerType, 1);

        WaitForWork();
      }
    }
  }

  return 0;
}

void plTaskWorkerThread::WaitForWork()
{
  // m_bIsIdle usually will be true here, but may also already have been reset to false
  // in that case m_WakeUpSignal will be raised already and the code below will just run through and continue

  m_ThreadActiveTime += plTime::Now() - m_StartedWorkingTime;
  m_bExecutingTask = false;
  m_WakeUpSignal.WaitForSignal();
  PLASMA_ASSERT_DEBUG(m_iWorkerState == (int)plTaskWorkerState::Active, "Worker state should have been reset to 'active'");
}

plTaskWorkerState plTaskWorkerThread::WakeUpIfIdle()
{
  plTaskWorkerState prev = (plTaskWorkerState)m_iWorkerState.CompareAndSwap((int)plTaskWorkerState::Idle, (int)plTaskWorkerState::Active);
  if (prev == plTaskWorkerState::Idle) // was idle before
  {
    m_WakeUpSignal.RaiseSignal();
  }

  return static_cast<plTaskWorkerState>(prev);
}

void plTaskWorkerThread::UpdateThreadUtilization(plTime timePassed)
{
  plTime tActive = m_ThreadActiveTime;

  // The thread keeps track of how much time it spends executing tasks.
  // Here we retrieve that time and resets it to zero.
  {
    m_ThreadActiveTime = plTime::MakeZero();

    if (m_bExecutingTask)
    {
      const plTime tNow = plTime::Now();
      tActive += tNow - m_StartedWorkingTime;
      m_StartedWorkingTime = tNow;
    }
  }

  m_fLastThreadUtilization = tActive.GetSeconds() / timePassed.GetSeconds();
  m_uiLastNumTasksExecuted = m_uiNumTasksExecuted;
  m_uiNumTasksExecuted = 0;
}

double plTaskWorkerThread::GetThreadUtilization(plUInt32* pNumTasksExecuted /*= nullptr*/)
{
  if (pNumTasksExecuted)
  {
    *pNumTasksExecuted = m_uiLastNumTasksExecuted;
  }

  return m_fLastThreadUtilization;
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskWorkerThread);
