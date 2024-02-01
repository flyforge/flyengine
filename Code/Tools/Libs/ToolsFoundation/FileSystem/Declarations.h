#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/Uuid.h>

#if 0 // Define to enable extensive file system profile scopes
#  define FILESYSTEM_PROFILE(szName) PL_PROFILE_SCOPE(szName)

#else
#  define FILESYSTEM_PROFILE(Name)

#endif

/// \brief Information about a single file on disk. The file might be a document or any other file found in the data directories.
struct PL_TOOLSFOUNDATION_DLL plFileStatus
{
  enum class Status : plUInt8
  {
    Unknown,    ///< Since the file has been tagged as 'Unknown' it has not been encountered again on disk (yet). Use internally to find stale entries in the model.
    FileLocked, ///< The file is locked, i.e. reading is currently not possible. Try again at a later date.
    Valid       ///< The file exists on disk.
  };

  plFileStatus() = default;

  plTimestamp m_LastModified;
  plUInt64 m_uiHash = 0;
  plUuid m_DocumentID; ///< If the file is linked to a document, the GUID is valid, otherwise not.
  Status m_Status = Status::Unknown;
};
PL_DECLARE_REFLECTABLE_TYPE(PL_TOOLSFOUNDATION_DLL, plFileStatus);

PL_ALWAYS_INLINE plStreamWriter& operator<<(plStreamWriter& inout_stream, const plFileStatus& value)
{
  inout_stream.WriteBytes(&value, sizeof(plFileStatus)).IgnoreResult();
  return inout_stream;
}

PL_ALWAYS_INLINE plStreamReader& operator>>(plStreamReader& inout_stream, plFileStatus& ref_value)
{
  inout_stream.ReadBytes(&ref_value, sizeof(plFileStatus));
  return inout_stream;
}
