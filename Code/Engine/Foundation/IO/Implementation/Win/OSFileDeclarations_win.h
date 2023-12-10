#pragma once

#include <Foundation/Basics.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

// Avoid conflicts with windows.h
#ifdef DeleteFile
#  undef DeleteFile
#endif

#ifdef CopyFile
#  undef CopyFile
#endif

#if PLASMA_DISABLED(PLASMA_USE_POSIX_FILE_API)

#  include <Foundation/Basics/Platform/Win/MinWindows.h>

struct plOSFileData
{
  plOSFileData() { m_pFileHandle = PLASMA_WINDOWS_INVALID_HANDLE_VALUE; }

  plMinWindows::HANDLE m_pFileHandle;
};

struct plFileIterationData
{
  plHybridArray<plMinWindows::HANDLE, 16> m_Handles;
};

#endif

/// \endcond
