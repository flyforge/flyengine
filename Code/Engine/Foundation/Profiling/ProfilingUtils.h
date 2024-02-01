#pragma once

#include <Foundation/Basics.h>

class PL_FOUNDATION_DLL plProfilingUtils
{
public:
  /// \brief Captures profiling data via plProfilingSystem::Capture and saves it to the giben file location.
  static plResult SaveProfilingCapture(plStringView sCapturePath);
  /// \brief Reads two profiling captures and merges them into one.
  static plResult MergeProfilingCaptures(plStringView sCapturePath1, plStringView sCapturePath2, plStringView sMergedCapturePath);
};
