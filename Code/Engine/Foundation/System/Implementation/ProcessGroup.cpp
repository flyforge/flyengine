#include <Foundation/FoundationPCH.h>

#if PLASMA_ENABLED(PLASMA_SUPPORTS_PROCESSES)
// Include inline file
#  if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#    include <Foundation/System/Implementation/Win/ProcessGroup_win.h>
#  else
#    include <Foundation/System/Implementation/other/ProcessGroup_other.h>
#  endif

const plHybridArray<plProcess, 8>& plProcessGroup::GetProcesses() const
{
  return m_Processes;
}

#endif

PLASMA_STATICLINK_FILE(Foundation, Foundation_System_Implementation_ProcessGroup);
