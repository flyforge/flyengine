#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/Task.h>

plTask::plTask() = default;
plTask::~plTask() = default;

void plTask::Reset()
{
  m_iRemainingRuns = (int)plMath::Max(1u, m_uiMultiplicity);
  m_bCancelExecution = false;
  m_bTaskIsScheduled = false;
  m_bUsesMultiplicity = m_uiMultiplicity > 0;
}

void plTask::ConfigureTask(const char* szTaskName, plTaskNesting nestingMode, plOnTaskFinishedCallback callback /*= plOnTaskFinishedCallback()*/)
{
  PLASMA_ASSERT_DEV(IsTaskFinished(), "This function must be called before the task is started.");

  m_sTaskName = szTaskName;
  m_NestingMode = nestingMode;
  m_OnTaskFinished = callback;
}

void plTask::SetMultiplicity(plUInt32 uiMultiplicity)
{
  m_uiMultiplicity = uiMultiplicity;
  m_bUsesMultiplicity = m_uiMultiplicity > 0;
}

void plTask::Run(plUInt32 uiInvocation)
{
  // actually this should not be possible to happen
  if (m_iRemainingRuns == 0 || m_bCancelExecution)
  {
    m_iRemainingRuns = 0;
    return;
  }

  {
    plStringBuilder scopeName = m_sTaskName;

    if (m_bUsesMultiplicity)
      scopeName.AppendFormat("-{}", uiInvocation);

    PLASMA_PROFILE_SCOPE(scopeName.GetData());

    if (m_bUsesMultiplicity)
    {
      ExecuteWithMultiplicity(uiInvocation);
    }
    else
    {
      Execute();
    }
  }

  m_iRemainingRuns.Decrement();
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_Task);
