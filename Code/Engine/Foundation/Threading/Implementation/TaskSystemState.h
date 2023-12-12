#pragma once

#include <Foundation/Threading/TaskSystem.h>

class plTaskSystemThreadState
{
private:
  friend class plTaskSystem;
  friend class plTaskWorkerThread;

  // The arrays of all the active worker threads.
  plDynamicArray<plTaskWorkerThread*> m_Workers[plWorkerThreadType::ENUM_COUNT];

  // the number of allocated (non-null) worker threads in m_Workers
  plAtomicInteger32 m_iAllocatedWorkers[plWorkerThreadType::ENUM_COUNT];

  // the maximum number of worker threads that should be non-idle (and not blocked) at any time
  plUInt32 m_uiMaxWorkersToUse[plWorkerThreadType::ENUM_COUNT] = {};
};

class plTaskSystemState
{
private:
  friend class plTaskSystem;

  // The target frame time used by FinishFrameTasks()
  plTime m_TargetFrameTime = plTime::Seconds(1.0 / 40.0); // => 25 ms

  // The deque can grow without relocating existing data, therefore the plTaskGroupID's can store pointers directly to the data
  plDeque<plTaskGroup> m_TaskGroups;

  // The lists of all scheduled tasks, for each priority.
  plList<plTaskSystem::TaskData> m_Tasks[plTaskPriority::ENUM_COUNT];
};
