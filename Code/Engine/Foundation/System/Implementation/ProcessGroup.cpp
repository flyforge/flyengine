#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_SUPPORTS_PROCESSES)

#  include <Foundation/System/ProcessGroup.h>

const plHybridArray<plProcess, 8>& plProcessGroup::GetProcesses() const
{
  return m_Processes;
}

#endif


