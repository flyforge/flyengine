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

  PL_LOCK(m_CondVarGroupFinished);

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

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
void plTaskGroup::DebugCheckTaskGroup(plTaskGroupID groupID, plMutex& mutex)
{
  PL_LOCK(mutex);

  const plTaskGroup* pGroup = groupID.m_pTaskGroup;

  PL_ASSERT_DEV(pGroup != nullptr, "TaskGroupID is invalid.");
  PL_ASSERT_DEV(pGroup->m_uiGroupCounter == groupID.m_uiGroupCounter, "The given TaskGroupID is not valid anymore.");
  PL_ASSERT_DEV(!pGroup->m_bStartedByUser, "The given TaskGroupID is already started, you cannot modify it anymore.");
  PL_ASSERT_DEV(pGroup->m_iNumActiveDependencies == 0, "Invalid active dependenices");
}
#endif


