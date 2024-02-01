#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Threading/ConditionVariable.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Types/SharedPtr.h>

/// \internal Represents the state of a group of tasks that can be waited on
class plTaskGroup
{
  PL_DISALLOW_COPY_AND_ASSIGN(plTaskGroup);

public:
  plTaskGroup();
  ~plTaskGroup();

private:
  friend class plTaskSystem;

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  static void DebugCheckTaskGroup(plTaskGroupID groupID, plMutex& mutex);
#else
  PL_ALWAYS_INLINE static void DebugCheckTaskGroup(plTaskGroupID groupID, plMutex& mutex) {}
#endif

  /// \brief Puts the calling thread to sleep until this group is fully finished.
  void WaitForFinish(plTaskGroupID group) const;
  void Reuse(plTaskPriority::Enum priority, plOnTaskGroupFinishedCallback callback);

  bool m_bInUse = true;
  bool m_bStartedByUser = false;
  plUInt16 m_uiTaskGroupIndex = 0xFFFF; // only there as a debugging aid
  plUInt32 m_uiGroupCounter = 1;
  plHybridArray<plSharedPtr<plTask>, 16> m_Tasks;
  plHybridArray<plTaskGroupID, 4> m_DependsOnGroups;
  plHybridArray<plTaskGroupID, 8> m_OthersDependingOnMe;
  plAtomicInteger32 m_iNumActiveDependencies;
  plAtomicInteger32 m_iNumRemainingTasks;
  plOnTaskGroupFinishedCallback m_OnFinishedCallback;
  plTaskPriority::Enum m_Priority = plTaskPriority::ThisFrame;
  mutable plConditionVariable m_CondVarGroupFinished;
};
