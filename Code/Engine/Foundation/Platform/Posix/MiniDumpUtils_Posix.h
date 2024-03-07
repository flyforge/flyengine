#include <Foundation/FoundationInternal.h>
PL_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/MiniDumpUtils.h>

plStatus plMiniDumpUtils::WriteExternalProcessMiniDump(plStringView sDumpFile, plUInt32 uiProcessID, plDumpType dumpTypeOverride)
{
  return plStatus("Not implemented on Posix");
}

plStatus plMiniDumpUtils::LaunchMiniDumpTool(plStringView sDumpFile, plDumpType dumpTypeOverride)
{
  return plStatus("Not implemented on Posix");
}