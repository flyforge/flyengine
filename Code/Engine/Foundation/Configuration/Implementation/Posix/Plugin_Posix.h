#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <dlfcn.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>

using plPluginModule = void*;

void plPlugin::GetPluginPaths(plStringView sPluginName, plStringBuilder& sOriginalFile, plStringBuilder& sCopiedFile, plUInt8 uiFileCopyNumber)
{
  sOriginalFile = plOSFile::GetApplicationDirectory();
  sOriginalFile.AppendPath(sPluginName);
  sOriginalFile.Append(".so");

  sCopiedFile = plOSFile::GetApplicationDirectory();
  sCopiedFile.AppendPath(sPluginName);

  if (uiFileCopyNumber > 0)
    sCopiedFile.AppendFormat("{0}", uiFileCopyNumber);

  sCopiedFile.Append(".loaded");
}

plResult UnloadPluginModule(plPluginModule& Module, plStringView sPluginFile)
{
  if (dlclose(Module) != 0)
  {
    plStringBuilder tmp;
    plLog::Error("Could not unload plugin '{0}'. Error {1}", sPluginFile.GetData(tmp), static_cast<const char*>(dlerror()));
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plResult LoadPluginModule(plStringView sFileToLoad, plPluginModule& Module, plStringView sPluginFile)
{
  plStringBuilder tmp;
  Module = dlopen(sFileToLoad.GetData(tmp), RTLD_NOW | RTLD_GLOBAL);
  if (Module == nullptr)
  {
    plLog::Error("Could not load plugin '{0}'. Error {1}.\nSet the environment variable LD_DEBUG=all to get more information.", sPluginFile.GetData(tmp), static_cast<const char*>(dlerror()));
    return PLASMA_FAILURE;
  }
  return PLASMA_SUCCESS;
}
