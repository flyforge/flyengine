#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Configuration/Plugin.h>

using plPluginModule = void*;

void plPlugin::GetPluginPaths(plStringView sPluginName, plStringBuilder& sOriginalFile, plStringBuilder& sCopiedFile, plUInt8 uiFileCopyNumber)
{
  PLASMA_ASSERT_NOT_IMPLEMENTED;
}

plResult UnloadPluginModule(plPluginModule& Module, plStringView sPluginFile)
{
  PLASMA_ASSERT_NOT_IMPLEMENTED;

  return PLASMA_FAILURE;
}

plResult LoadPluginModule(plStringView sFileToLoad, plPluginModule& Module, plStringView sPluginFile)
{
  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return PLASMA_FAILURE;
}
