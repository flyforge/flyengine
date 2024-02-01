
///
/// Implements plProcessGroup by using plProcess
///

#include <Foundation/FoundationInternal.h>
PL_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/ProcessGroup.h>

#if PL_ENABLED(PL_SUPPORTS_PROCESSES)

struct plProcessGroupImpl
{
  PL_DECLARE_POD_TYPE();
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

plResult plProcessGroup::WaitToFinish(plTime timeout /*= plTime::MakeZero()*/)
{
  for (auto& process : m_Processes)
  {
    if (process.GetState() != plProcessState::Finished && process.WaitToFinish(timeout).Failed())
    {
      return PL_FAILURE;
    }
  }

  return PL_SUCCESS;
}

plResult plProcessGroup::TerminateAll(plInt32 iForcedExitCode /*= -2*/)
{
  auto result = PL_SUCCESS;
  for (auto& process : m_Processes)
  {
    if (process.GetState() == plProcessState::Running && process.Terminate().Failed())
    {
      result = PL_FAILURE;
    }
  }

  return result;
}

#endif
