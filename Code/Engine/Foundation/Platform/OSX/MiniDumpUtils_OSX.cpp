#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_OSX)

#  include <Foundation/System/MiniDumpUtils.h>

plStatus plMiniDumpUtils::WriteExternalProcessMiniDump(plStringView sDumpFile, plUInt32 uiProcessID)
{
  return plStatus("Not implemented on OSX");
}

plStatus plMiniDumpUtils::LaunchMiniDumpTool(plStringView sDumpFile)
{
  return plStatus("Not implemented on OSX");
}

#endif


