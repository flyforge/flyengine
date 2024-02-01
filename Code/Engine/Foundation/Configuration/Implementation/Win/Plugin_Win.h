
#include <Foundation/FoundationInternal.h>
PL_FOUNDATION_INTERNAL_HEADER

#if PL_ENABLED(PL_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Configuration/Plugin.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Strings/StringBuilder.h>

using plPluginModule = HMODULE;

bool plPlugin::PlatformNeedsPluginCopy()
{
  return true;
}

void plPlugin::GetPluginPaths(plStringView sPluginName, plStringBuilder& ref_sOriginalFile, plStringBuilder& ref_sCopiedFile, plUInt8 uiFileCopyNumber)
{
  ref_sOriginalFile = plOSFile::GetApplicationDirectory();
  ref_sOriginalFile.AppendPath(sPluginName);
  ref_sOriginalFile.Append(".dll");

  ref_sCopiedFile = plOSFile::GetApplicationDirectory();
  ref_sCopiedFile.AppendPath(sPluginName);

  if (!plOSFile::ExistsFile(ref_sOriginalFile))
  {
    ref_sOriginalFile = plOSFile::GetCurrentWorkingDirectory();
    ref_sOriginalFile.AppendPath(sPluginName);
    ref_sOriginalFile.Append(".dll");

    ref_sCopiedFile = plOSFile::GetCurrentWorkingDirectory();
    ref_sCopiedFile.AppendPath(sPluginName);
  }

  if (uiFileCopyNumber > 0)
    ref_sCopiedFile.AppendFormat("{0}", uiFileCopyNumber);

  ref_sCopiedFile.Append(".loaded");
}

plResult UnloadPluginModule(plPluginModule& ref_pModule, plStringView sPluginFile)
{
  // reset last error code
  SetLastError(ERROR_SUCCESS);

  if (FreeLibrary(ref_pModule) == FALSE)
  {
    plLog::Error("Could not unload plugin '{0}'. Error-Code {1}", sPluginFile, plArgErrorCode(GetLastError()));
    return PL_FAILURE;
  }

  ref_pModule = nullptr;
  return PL_SUCCESS;
}

plResult LoadPluginModule(plStringView sFileToLoad, plPluginModule& ref_pModule, plStringView sPluginFile)
{
  // reset last error code
  SetLastError(ERROR_SUCCESS);

#  if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
  plStringBuilder relativePath = sFileToLoad;
  PL_SUCCEED_OR_RETURN(relativePath.MakeRelativeTo(plOSFile::GetApplicationDirectory()));
  ref_pModule = LoadPackagedLibrary(plStringWChar(relativePath).GetData(), 0);
#  else
  ref_pModule = LoadLibraryW(plStringWChar(sFileToLoad).GetData());
#  endif

  if (ref_pModule == nullptr)
  {
    const DWORD err = GetLastError();
    plLog::Error("Could not load plugin '{0}'. Error-Code {1}", sPluginFile, plArgErrorCode(err));

    if (err == 126)
    {
      plLog::Error("Please Note: This means that the plugin exists, but a DLL dependency of the plugin is missing. You probably need to copy 3rd "
                   "party DLLs next to the plugin.");
    }

    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

#else
#  error "This file should not have been included."
#endif
