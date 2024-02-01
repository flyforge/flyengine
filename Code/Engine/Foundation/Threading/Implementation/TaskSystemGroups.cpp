#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>


plTaskGroupID plTaskSystem::CreateTaskGroup(plTaskPriority::Enum priority, plOnTaskGroupFinishedCallback callback)
{
  PL_LOCK(s_TaskSystemMutex);

  plUInt32 i = 0;

  // this search could be speed up with a stack of free groups
  for (; i < s_pState->m_TaskGroups.GetCount(); ++i)
  {
    if (!s_pState->m_TaskGroups[i].m_bInUse)
    {
      goto foundtaskgroup;
    }
  }

  // no free group found, create a new one
  s_pState->m_TaskGroups.ExpandAndGetRef();
  s_pState->m_TaskGroups[i].m_uiTaskGroupIndex = static_cast<plUInt16>(i);

foundtaskgroup:

  s_pState->m_TaskGroups[i].Reuse(priority, callback);

  plTaskGroupID id;
  id.m_pTaskGroup = &s_pState->m_TaskGroups[i];
  id.m_uiGroupCounter = s_pState->m_TaskGroups[i].m_uiGroupCounter;
  return id;
}

void plTaskSystem::AddTaskToGroup(plTaskGroupID groupID, const plSharedPtr<plTask>& pTask)
{
  PL_ASSERT_DEBUG(pTask != nullptr, "Cannot add nullptr tasks.");
  PL_ASSERT_DEV(pTask->IsTaskFinished(), "The given task is not finished! Cannot reuse a task before it is done.");
  PL_ASSERT_DEBUG(!pTask->m_sTaskName.IsEmpty(), "Every task should have a name");

  plTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  pTask->Reset();
  pTask->m_BelongsToGroup = groupID;
  groupID.m_pTaskGroup->m_Tasks.PushBack(pTask);
}

void plTaskSystem::AddTaskGroupDependency(plTaskGroupID groupID, plTaskGroupID dependsOn)
{
  PL_ASSERT_DEBUG(dependsOn.IsValid(), "Invalid dependency");
  PL_ASSERT_DEBUG(groupID.m_pTaskGroup != dependsOn.m_pTaskGroup || groupID.m_uiGroupCounter != dependsOn.m_uiGroupCounter, "Group cannot depend on itselfs");

  plTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  groupID.m_pTaskGroup->m_DependsOnGroups.PushBack(dependsOn);
}

void plTaskSystem::AddTaskGroupDependencyBatch(plArrayPtr<const plTaskGroupDependency> batch)
{
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  // lock here once to reduce the overhead of plTaskGroup::DebugCheckTaskGroup inside AddTaskGroupDependency
  PL_LOCK(s_TaskSystemMutex);
#endif

  for (const plTaskGroupDependency& dep : batch)
  {
    AddTaskGroupDependency(dep.m_TaskGroup, dep.m_DependsOn);
  }
}

void plTaskSystem::StartTaskGroup(plTaskGroupID groupID)
{
  PL_ASSERT_DEV(s_pThreadState->m_Workers[plWorkerThreadType::ShortTasks].GetCount() > 0, "No worker threads started.");

  plTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  plInt32 iActiveDependencies = 0;

  {
    PL_LOCK(s_TaskSystemMutex);

    plTaskGroup& tg = *groupID.m_pTaskGroup;

    tg.m_bStartedByUser = true;

    for (plUInt32 i = 0; i < tg.m_DependsOnGroups.GetCount(); ++i)
    {
      if (!IsTaskGroupFinished(tg.m_DependsOnGroups[i]))
      {
        plTaskGroup& Dependency = *tg.m_DependsOnGroups[i].m_pTaskGroup;

        // add this task group to the list of dependencies, such that when that group finishes, this task group can get woken up
        Dependency.m_OthersDependingOnMe.PushBack(groupID);

        // count how many other groups need to finish before this task group can be executed
        ++iActiveDependencies;
      }
    }

    if (iActiveDependencies != 0)
    {
      // atomic integers are quite slow, so do not use them in the loop, where they are not yet needed
      tg.m_iNumActiveDependencies = iActiveDependencies;
    }
  }

  if (iActiveDependencies == 0)
  {
    ScheduleGroupTasks(groupID.m_pTaskGroup, false);
  }
}

void plTaskSystem::StartTaskGroupBatch(plArrayPtr<const plTaskGroupID> batch)
{
  PL_LOCK(s_TaskSystemMutex);

  for (const plTaskGroupID& group : batch)
  {
    StartTaskGroup(group);
  }
}

bool plTaskSystem::IsTaskGroupFinished(plTaskGroupID group)
{
  // if the counters differ, the task group has been reused since the GroupID was created, so that group has finished
  return (group.m_pTaskGroup == nullptr) || (group.m_pTaskGroup->m_uiGroupCounter != group.m_uiGroupCounter);
}

void plTaskSystem::ScheduleGroupTasks(plTaskGroup* pGroup, bool bHighPriority)
{
  if (pGroup->m_Tasks.IsEmpty())
  {
    pGroup->m_iNumRemainingTasks = 1;

    // "finish" one task -> will finish the task group and kick off dependent groups
    TaskHasFinished(nullptr, pGroup);
    return;
  }

  plInt32 iRemainingTasks = 0;

  // add all the tasks to the task list, so that they will be processed
  {
    PL_LOCK(s_TaskSystemMutex);


    // store how many tasks from this groups still need to be processed

    for (auto pTask : pGroup->m_Tasks)
    {
      iRemainingTasks += plMath::Max(1u, pTask->m_uiMultiplicity);
      pTask->m_iRemainingRuns = plMath::Max(1u, pTask->m_uiMultiplicity);
    }

    pGroup->m_iNumRemainingTasks = iRemainingTasks;


    for (plUInt32 task = 0; task < pGroup->m_Tasks.GetCount(); ++task)
    {
      auto& pTask = pGroup->m_Tasks[task];

      for (plUInt32 mult = 0; mult < plMath::Max(1u, pTask->m_uiMultiplicity); ++mult)
      {
        TaskData td;
        td.m_pBelongsToGroup = pGroup;
        td.m_pTask = pTask;
        td.m_pTask->m_bTaskIsScheduled = true;
        td.m_uiInvocation = mult;

        if (bHighPriority)
          s_pState->m_Tasks[pGroup->m_Priority].PushFront(td);
        else
          s_pState->m_Tasks[pGroup->m_Priority].PushBack(td);
      }
    }

    // send the proper thread signal, to make sure one of the correct worker threads is awake
    switch (pGroup->m_Priority)
    {
      case plTaskPriority::EarlyThisFrame:
      case plTaskPriority::ThisFrame:
      case plTaskPriority::LateThisFrame:
      case plTaskPriority::EarlyNextFrame:
      case plTaskPriority::NextFrame:
      case plTaskPriority::LateNextFrame:
      case plTaskPriority::In2Frames:
      case plTaskPriority::In3Frames:
      case plTaskPriority::In4Frames:
      case plTaskPriority::In5Frames:
      case plTaskPriority::In6Frames:
      case plTaskPriority::In7Frames:
      case plTaskPriority::In8Frames:
      case plTaskPriority::In9Frames:
      {
        WakeUpThreads(plWorkerThreadType::ShortTasks, iRemainingTasks);
        break;
      }

      case plTaskPriority::LongRunning:
      case plTaskPriority::LongRunningHighPriority:
      {
        WakeUpThreads(plWorkerThreadType::LongTasks, iRemainingTasks);
        break;
      }

      case plTaskPriority::FileAccess:
      case plTaskPriority::FileAccessHighPriority:
      {
        WakeUpThreads(plWorkerThreadType::FileAccess, iRemainingTasks);
        break;
      }

      case plTaskPriority::SomeFrameMainThread:
      case plTaskPriority::ThisFrameMainThread:
      case plTaskPriority::ENUM_COUNT:
        // nothing to do for these enum values
        break;
    }
  }
}

void plTaskSystem::DependencyHasFinished(plTaskGroup* pGroup)
{
  // remove one dependency from the group
  if (pGroup->m_iNumActiveDependencies.Decrement() == 0)
  {
    // if there are no remaining dependencies, kick off all tasks in this group
    ScheduleGroupTasks(pGroup, true);
  }
}

plResult plTaskSystem::CancelGroup(plTaskGroupID group, plOnTaskRunning::Enum onTaskRunning)
{
  if (plTaskSystem::IsTaskGroupFinished(group))
    return PL_SUCCESS;

  PL_PROFILE_SCOPE("CancelGroup");

  PL_LOCK(s_TaskSystemMutex);

  plResult res = PL_SUCCESS;

  auto TasksCopy = group.m_pTaskGroup->m_Tasks;

  // first cancel ALL the tasks in the group, without waiting for anything
  for (plUInt32 task = 0; task < TasksCopy.GetCount(); ++task)
  {
    if (CancelTask(TasksCopy[task], plOnTaskRunning::ReturnWithoutBlocking) == PL_FAILURE)
    {
      res = PL_FAILURE;
    }
  }

  // if all tasks could be removed without problems, we do not need to try it again with blocking

  if (onTaskRunning == plOnTaskRunning::WaitTillFinished && res == PL_FAILURE)
  {
    // now cancel the tasks in the group again, this time wait for those that are already running
    for (plUInt32 task = 0; task < TasksCopy.GetCount(); ++task)
    {
      CancelTask(TasksCopy[task], plOnTaskRunning::WaitTillFinished).IgnoreResult();
    }
  }

  return res;
}

void plTaskSystem::WaitForGroup(plTaskGroupID group)
{
  PL_PROFILE_SCOPE("WaitForGroup");

  PL_ASSERT_DEV(tl_TaskWorkerInfo.m_bAllowNestedTasks, "The executing task '{}' is flagged to never wait for other tasks but does so anyway. Remove the flag or remove the wait-dependency.", tl_TaskWorkerInfo.m_szTaskName);

  const auto ThreadTaskType = tl_TaskWorkerInfo.m_WorkerType;
  const bool bAllowSleep = ThreadTaskType != plWorkerThreadType::MainThread;

  while (!plTaskSystem::IsTaskGroupFinished(group))
  {
    if (!HelpExecutingTasks(group))
    {
      if (bAllowSleep)
      {
        const plWorkerThreadType::Enum typeToWakeUp = (ThreadTaskType == plWorkerThreadType::Unknown) ? plWorkerThreadType::ShortTasks : ThreadTaskType;

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          PL_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)plTaskWorkerState::Blocked) == (int)plTaskWorkerState::Active, "Corrupt worker state");
        }

        WakeUpThreads(typeToWakeUp, 1);

        group.m_pTaskGroup->WaitForFinish(group);

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          PL_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)plTaskWorkerState::Active) == (int)plTaskWorkerState::Blocked, "Corrupt worker state");
        }

        break;
      }
      else
      {
        plThreadUtils::YieldTimeSlice();
      }
    }
  }
}

void plTaskSystem::WaitForCondition(plDelegate<bool()> condition)
{
  PL_PROFILE_SCOPE("WaitForCondition");

  PL_ASSERT_DEV(tl_TaskWorkerInfo.m_bAllowNestedTasks, "The executing task '{}' is flagged to never wait for other tasks but does so anyway. Remove the flag or remove the wait-dependency.", tl_TaskWorkerInfo.m_szTaskName);

  const auto ThreadTaskType = tl_TaskWorkerInfo.m_WorkerType;
  const bool bAllowSleep = ThreadTaskType != plWorkerThreadType::MainThread;

  while (!condition())
  {
    if (!HelpExecutingTasks(plTaskGroupID()))
    {
      if (bAllowSleep)
      {
        const plWorkerThreadType::Enum typeToWakeUp = (ThreadTaskType == plWorkerThreadType::Unknown) ? plWorkerThreadType::ShortTasks : ThreadTaskType;

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          PL_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)plTaskWorkerState::Blocked) == (int)plTaskWorkerState::Active, "Corrupt worker state");
        }

        WakeUpThreads(typeToWakeUp, 1);

        while (!condition())
        {
          // TODO: busy loop for now
          plThreadUtils::YieldTimeSlice();
        }

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          PL_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)plTaskWorkerState::Active) == (int)plTaskWorkerState::Blocked, "Corrupt worker state");
        }

        break;
      }
      else
      {
        plThreadUtils::YieldTimeSlice();
      }
    }
  }
}


