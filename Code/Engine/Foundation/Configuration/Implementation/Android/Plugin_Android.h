#include <Foundation/FoundationInternal.h>
PL_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Configuration/Plugin.h>

using plPluginModule = void*;

bool plPlugin::PlatformNeedsPluginCopy()
{
  PL_ASSERT_NOT_IMPLEMENTED;
  return false;
}

void plPlugin::GetPluginPaths(plStringView sPluginName, plStringBuilder& sOriginalFile, plStringBuilder& sCopiedFile, plUInt8 uiFileCopyNumber)
{
  PL_ASSERT_NOT_IMPLEMENTED;
}

plResult UnloadPluginModule(plPluginModule& Module, plStringView sPluginFile)
{
  PL_ASSERT_NOT_IMPLEMENTED;

  return PL_FAILURE;
}

plResult LoadPluginModule(plStringView sFileToLoad, plPluginModule& Module, plStringView sPluginFile)
{
  PL_ASSERT_NOT_IMPLEMENTED;
  return PL_FAILURE;
}
