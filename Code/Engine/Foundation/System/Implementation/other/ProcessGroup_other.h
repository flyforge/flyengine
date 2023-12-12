
///
/// Implements plProcessGroup by using plProcess
///

#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/ProcessGroup.h>

struct plProcessGroupImpl
{
  PLASMA_DECLARE_POD_TYPE();
};

plProcessGroup::plProcessGroup(plStringView sGroupName)
{
}

plProcessGroup::~plProcessGroup()
{
  TerminateAll().IgnoreResult();
}

plResult plProcessGroup::Launch(const plProcessOptions& opt)
{
  plProcess& process = m_Processes.ExpandAndGetRef();
  return process.Launch(opt);
}

plResult plProcessGroup::WaitToFinish(plTime timeout /*= plTime::Zero()*/)
{
  for (auto& process : m_Processes)
  {
    if (process.GetState() != plProcessState::Finished && process.WaitToFinish(timeout).Failed())
    {
      return PLASMA_FAILURE;
    }
  }

  return PLASMA_SUCCESS;
}

plResult plProcessGroup::TerminateAll(plInt32 iForcedExitCode /*= -2*/)
{
  auto result = PLASMA_SUCCESS;
  for (auto& process : m_Processes)
  {
    if (process.GetState() == plProcessState::Running && process.Terminate().Failed())
    {
      result = PLASMA_FAILURE;
    }
  }

  return result;
}
