#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/MiniDumpUtils.h>

plStatus plMiniDumpUtils::WriteExternalProcessMiniDump(plStringView sDumpFile, plUInt32 uiProcessID)
{
  return plStatus("Not implemented on Posix");
}

plStatus plMiniDumpUtils::LaunchMiniDumpTool(plStringView sDumpFile)
{
  return plStatus("Not implemented on Posix");
}
