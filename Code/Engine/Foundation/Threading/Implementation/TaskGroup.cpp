#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Implementation/Task.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Lock.h>

plTaskGroup::plTaskGroup() = default;
plTaskGroup::~plTaskGroup() = default;

void plTaskGroup::WaitForFinish(plTaskGroupID group) const
{
  if (m_uiGroupCounter != group.m_uiGroupCounter)
    return;

  PLASMA_LOCK(m_CondVarGroupFinished);

  while (m_uiGroupCounter == group.m_uiGroupCounter)
  {
    m_CondVarGroupFinished.UnlockWaitForSignalAndLock();
  }
}

void plTaskGroup::Reuse(plTaskPriority::Enum priority, plOnTaskGroupFinishedCallback callback)
{
  m_bInUse = true;
  m_bStartedByUser = false;
  m_uiGroupCounter += 2; // even if it wraps around, it will never be zero, thus zero stays an invalid group counter
  m_Tasks.Clear();
  m_DependsOnGroups.Clear();
  m_OthersDependingOnMe.Clear();
  m_Priority = priority;
  m_OnFinishedCallback = callback;
}

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
void plTaskGroup::DebugCheckTaskGroup(plTaskGroupID groupID, plMutex& mutex)
{
  PLASMA_LOCK(mutex);

  const plTaskGroup* pGroup = groupID.m_pTaskGroup;

  PLASMA_ASSERT_DEV(pGroup != nullptr, "TaskGroupID is invalid.");
  PLASMA_ASSERT_DEV(pGroup->m_uiGroupCounter == groupID.m_uiGroupCounter, "The given TaskGroupID is not valid anymore.");
  PLASMA_ASSERT_DEV(!pGroup->m_bStartedByUser, "The given TaskGroupID is already started, you cannot modify it anymore.");
  PLASMA_ASSERT_DEV(pGroup->m_iNumActiveDependencies == 0, "Invalid active dependenices");
}
#endif


PLASMA_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskGroup);
