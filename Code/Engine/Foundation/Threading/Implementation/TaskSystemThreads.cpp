#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

plUInt32 plTaskSystem::GetWorkerThreadCount(plWorkerThreadType::Enum type)
{
  return s_pThreadState->m_uiMaxWorkersToUse[type];
}

plUInt32 plTaskSystem::GetNumAllocatedWorkerThreads(plWorkerThreadType::Enum type)
{
  return s_pThreadState->m_iAllocatedWorkers[type];
}

void plTaskSystem::SetWorkerThreadCount(plInt32 iShortTasks, plInt32 iLongTasks)
{
  plSystemInformation info = plSystemInformation::Get();

  // these settings are supposed to be a sensible default for most applications
  // an app can of course change that to optimize for its own usage
  //
  const plInt32 iCpuCores = info.GetCPUCoreCount();

  // at least 2 threads, 4 on six cores, 6 on eight cores and up
  if (iShortTasks <= 0)
    iShortTasks = plMath::Clamp<plInt32>(iCpuCores - 2, 2, 8);

  // at least 2 threads, 4 on six cores, 6 on eight cores and up
  if (iLongTasks <= 0)
    iLongTasks = plMath::Clamp<plInt32>(iCpuCores - 2, 2, 8);

  // plus there is always one additional 'file access' thread
  // and the main thread, of course

  plUInt32 uiShortTasks = static_cast<plUInt32>(plMath::Max<plInt32>(iShortTasks, 1));
  plUInt32 uiLongTasks = static_cast<plUInt32>(plMath::Max<plInt32>(iLongTasks, 1));

  // if nothing has changed, do nothing
  if (s_pThreadState->m_uiMaxWorkersToUse[plWorkerThreadType::ShortTasks] == uiShortTasks &&
      s_pThreadState->m_uiMaxWorkersToUse[plWorkerThreadType::LongTasks] == uiLongTasks)
    return;

  StopWorkerThreads();

  // this only allocates pointers, i.e. the maximum possible number of threads that we may be able to realloc at runtime
  s_pThreadState->m_Workers[plWorkerThreadType::ShortTasks].SetCount(1024);
  s_pThreadState->m_Workers[plWorkerThreadType::LongTasks].SetCount(1024);
  s_pThreadState->m_Workers[plWorkerThreadType::FileAccess].SetCount(128);

  s_pThreadState->m_uiMaxWorkersToUse[plWorkerThreadType::ShortTasks] = uiShortTasks;
  s_pThreadState->m_uiMaxWorkersToUse[plWorkerThreadType::LongTasks] = uiLongTasks;
  s_pThreadState->m_uiMaxWorkersToUse[plWorkerThreadType::FileAccess] = 1;

  AllocateThreads(plWorkerThreadType::ShortTasks, s_pThreadState->m_uiMaxWorkersToUse[plWorkerThreadType::ShortTasks]);
  AllocateThreads(plWorkerThreadType::LongTasks, s_pThreadState->m_uiMaxWorkersToUse[plWorkerThreadType::LongTasks]);
  AllocateThreads(plWorkerThreadType::FileAccess, s_pThreadState->m_uiMaxWorkersToUse[plWorkerThreadType::FileAccess]);
}

void plTaskSystem::StopWorkerThreads()
{
  bool bWorkersStillRunning = true;

  // as long as any worker thread is still active, send the wake up signal
  while (bWorkersStillRunning)
  {
    bWorkersStillRunning = false;

    for (plUInt32 type = 0; type < plWorkerThreadType::ENUM_COUNT; ++type)
    {
      const plUInt32 uiNumThreads = s_pThreadState->m_iAllocatedWorkers[type];

      for (plUInt32 i = 0; i < uiNumThreads; ++i)
      {
        if (s_pThreadState->m_Workers[type][i]->DeactivateWorker().Failed())
        {
          bWorkersStillRunning = true;
        }
      }
    }

    // waste some time
    plThreadUtils::YieldTimeSlice();
  }

  for (plUInt32 type = 0; type < plWorkerThreadType::ENUM_COUNT; ++type)
  {
    const plUInt32 uiNumWorkers = s_pThreadState->m_iAllocatedWorkers[type];

    for (plUInt32 i = 0; i < uiNumWorkers; ++i)
    {
      s_pThreadState->m_Workers[type][i]->Join();
      PL_DEFAULT_DELETE(s_pThreadState->m_Workers[type][i]);
    }

    s_pThreadState->m_iAllocatedWorkers[type] = 0;
    s_pThreadState->m_uiMaxWorkersToUse[type] = 0;
    s_pThreadState->m_Workers[type].Clear();
  }
}

void plTaskSystem::AllocateThreads(plWorkerThreadType::Enum type, plUInt32 uiAddThreads)
{
  PL_ASSERT_DEBUG(uiAddThreads > 0, "Invalid number of threads to allocate");

  {
    // prevent concurrent thread allocation
    PL_LOCK(s_TaskSystemMutex);

    plUInt32 uiNextThreadIdx = s_pThreadState->m_iAllocatedWorkers[type];

    PL_ASSERT_ALWAYS(uiNextThreadIdx + uiAddThreads <= s_pThreadState->m_Workers[type].GetCount(), "Max number of worker threads ({}) exceeded.",
      s_pThreadState->m_Workers[type].GetCount());

    for (plUInt32 i = 0; i < uiAddThreads; ++i)
    {
      s_pThreadState->m_Workers[type][uiNextThreadIdx] = PL_DEFAULT_NEW(plTaskWorkerThread, (plWorkerThreadType::Enum)type, uiNextThreadIdx);
      s_pThreadState->m_Workers[type][uiNextThreadIdx]->Start();

      ++uiNextThreadIdx;
    }

    // let others access the new threads now
    s_pThreadState->m_iAllocatedWorkers[type] = uiNextThreadIdx;
  }

  plLog::Dev("Allocated {} additional '{}' worker threads ({} total)", uiAddThreads, plWorkerThreadType::GetThreadTypeName(type),
    s_pThreadState->m_iAllocatedWorkers[type]);
}

void plTaskSystem::WakeUpThreads(plWorkerThreadType::Enum type, plUInt32 uiNumThreadsToWakeUp)
{
  // together with plTaskWorkerThread::Run() this function will make sure to keep the number
  // of active threads close to m_uiMaxWorkersToUse
  //
  // threads that go into the 'blocked' state will raise the number of threads that get activated
  // and when they are unblocked, together they may exceed the 'maximum' number of active threads
  // but over time the threads at the end of the list will put themselves to sleep again

  auto* s = plTaskSystem::s_pThreadState.Borrow();

  const plUInt32 uiTotalThreads = s_pThreadState->m_iAllocatedWorkers[type];
  plUInt32 uiAllowedActiveThreads = s_pThreadState->m_uiMaxWorkersToUse[type];

  for (plUInt32 threadIdx = 0; threadIdx < uiTotalThreads; ++threadIdx)
  {
    switch (s->m_Workers[type][threadIdx]->WakeUpIfIdle())
    {
      case plTaskWorkerState::Idle:
      {
        // was idle before -> now it is active
        if (--uiNumThreadsToWakeUp == 0)
          return;

        [[fallthrough]];
      }

      case plTaskWorkerState::Active:
      {
        // already active
        if (--uiAllowedActiveThreads == 0)
          return;

        break;
      }

      default:
        break;
    }
  }

  // if the loop above did not find enough threads to wake up
  if (uiNumThreadsToWakeUp > 0 && uiAllowedActiveThreads > 0)
  {
    // the new threads will start not-idle and take on some work
    AllocateThreads(type, plMath::Min(uiNumThreadsToWakeUp, uiAllowedActiveThreads));
  }
}

plWorkerThreadType::Enum plTaskSystem::GetCurrentThreadWorkerType()
{
  return tl_TaskWorkerInfo.m_WorkerType;
}

double plTaskSystem::GetThreadUtilization(plWorkerThreadType::Enum type, plUInt32 uiThreadIndex, plUInt32* pNumTasksExecuted /*= nullptr*/)
{
  return s_pThreadState->m_Workers[type][uiThreadIndex]->GetThreadUtilization(pNumTasksExecuted);
}

void plTaskSystem::DetermineTasksToExecuteOnThread(plTaskPriority::Enum& out_FirstPriority, plTaskPriority::Enum& out_LastPriority)
{
  switch (tl_TaskWorkerInfo.m_WorkerType)
  {
    case plWorkerThreadType::MainThread:
    {
      out_FirstPriority = plTaskPriority::ThisFrameMainThread;
      out_LastPriority = plTaskPriority::SomeFrameMainThread;
      break;
    }

    case plWorkerThreadType::FileAccess:
    {
      out_FirstPriority = plTaskPriority::FileAccessHighPriority;
      out_LastPriority = plTaskPriority::FileAccess;
      break;
    }

    case plWorkerThreadType::LongTasks:
    {
      out_FirstPriority = plTaskPriority::LongRunningHighPriority;
      out_LastPriority = plTaskPriority::LongRunning;
      break;
    }

    case plWorkerThreadType::ShortTasks:
    {
      out_FirstPriority = plTaskPriority::EarlyThisFrame;
      out_LastPriority = plTaskPriority::In9Frames;
      break;
    }

    case plWorkerThreadType::Unknown:
    {
      // probably a thread not launched through pl
      out_FirstPriority = plTaskPriority::EarlyThisFrame;
      out_LastPriority = plTaskPriority::In9Frames;
      break;
    }

    default:
    {
      PL_ASSERT_NOT_IMPLEMENTED;
      break;
    }
  }
}


