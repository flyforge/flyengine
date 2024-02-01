#pragma once

#include <Foundation/FoundationInternal.h>
PL_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringConversion.h>

/// \brief Converts an absolute path to a 'DOS device path'
///
/// https://docs.microsoft.com/dotnet/standard/io/file-path-formats#dos-device-paths
///
/// This is necessary to support very long file paths, ie. more than 260 characters.
class plDosDevicePath
{
public:
  plDosDevicePath(plStringView sPath)
  {
    plStringBuilder tmp("\\\\?\\", sPath);
    tmp.ReplaceAll("/", "\\");
    m_Data = tmp.GetData();
  }

  const wchar_t* GetData() const { return m_Data.GetData(); }

  operator const wchar_t*() const { return m_Data.GetData(); }

  plStringWChar m_Data;
};
