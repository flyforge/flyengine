#pragma once

#include <Foundation/Basics.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

struct plOSFileData
{
  plOSFileData() { m_pFileHandle = nullptr; }

  FILE* m_pFileHandle;
};

#if PL_ENABLED(PL_SUPPORTS_FILE_ITERATORS)

struct plFileIterationData
{
  // This is storing DIR*, which we can't forward declare
  plHybridArray<void*, 16> m_Handles;
  plString m_wildcardSearch;
};

#endif

/// \endcond
