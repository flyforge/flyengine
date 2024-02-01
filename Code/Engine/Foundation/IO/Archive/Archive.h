#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

class plRawMemoryStreamReader;

/// \brief Compression modes for plArchive file entries
enum class plArchiveCompressionMode : plUInt8
{
  Uncompressed,
  Compressed_zstd,
  Compressed_zip,
};

/// \brief Data for a single file entry in an plArchive file
class PL_FOUNDATION_DLL plArchiveEntry
{
public:
  plUInt64 m_uiDataStartOffset = 0;      ///< Byte offset for where the file's (compressed) data stream starts in the plArchive
  plUInt64 m_uiUncompressedDataSize = 0; ///< Size of the original uncompressed data.
  plUInt64 m_uiStoredDataSize = 0;       ///< The amount of (compressed) bytes actually stored in the plArchive.
  plUInt32 m_uiPathStringOffset = 0;     ///< Byte offset into plArchiveTOC::m_AllPathStrings where the path string for this entry resides.
  plArchiveCompressionMode m_CompressionMode = plArchiveCompressionMode::Uncompressed;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};

/// \brief Helper class to store a hashed string for quick lookup in the archive TOC
///
/// Stores a hash of the lower case string for quick comparison.
/// Additionally stores an offset into the plArchiveTOC::m_AllPathStrings array for final validation, to prevent hash collisions.
/// The proper string lookup with hash collision check only works together with plArchiveLookupString, which has the necessary context
/// to index the plArchiveTOC::m_AllPathStrings array.
class PL_FOUNDATION_DLL plArchiveStoredString
{
public:
  PL_DECLARE_POD_TYPE();

  plArchiveStoredString() = default;

  plArchiveStoredString(plUInt64 uiLowerCaseHash, plUInt32 uiSrcStringOffset)
    : m_uiLowerCaseHash(plHashingUtils::StringHashTo32(uiLowerCaseHash))
    , m_uiSrcStringOffset(uiSrcStringOffset)
  {
  }

  plUInt32 m_uiLowerCaseHash;
  plUInt32 m_uiSrcStringOffset;
};

void operator<<(plStreamWriter& inout_stream, const plArchiveStoredString& value);
void operator>>(plStreamReader& inout_stream, plArchiveStoredString& value);

/// \brief Helper class for looking up path strings in plArchiveTOC::FindEntry()
///
/// Only works together with plArchiveStoredString.
class plArchiveLookupString
{
  PL_DISALLOW_COPY_AND_ASSIGN(plArchiveLookupString);

public:
  PL_DECLARE_POD_TYPE();

  plArchiveLookupString(plUInt64 uiLowerCaseHash, plStringView sString, const plDynamicArray<plUInt8>& archiveAllPathStrings)
    : m_uiLowerCaseHash(plHashingUtils::StringHashTo32(uiLowerCaseHash))
    , m_sString(sString)
    , m_ArchiveAllPathStrings(archiveAllPathStrings)
  {
  }

  plUInt32 m_uiLowerCaseHash;
  plStringView m_sString;
  const plDynamicArray<plUInt8>& m_ArchiveAllPathStrings;
};

/// \brief Functions to enable plHashTable to 1) store plArchiveStoredString and 2) lookup strings efficiently with a plArchiveLookupString
template <>
struct plHashHelper<plArchiveStoredString>
{
  PL_ALWAYS_INLINE static plUInt32 Hash(const plArchiveStoredString& hs) { return hs.m_uiLowerCaseHash; }
  PL_ALWAYS_INLINE static plUInt32 Hash(const plArchiveLookupString& hs) { return hs.m_uiLowerCaseHash; }

  PL_ALWAYS_INLINE static bool Equal(const plArchiveStoredString& a, const plArchiveStoredString& b) { return a.m_uiSrcStringOffset == b.m_uiSrcStringOffset; }

  PL_ALWAYS_INLINE static bool Equal(const plArchiveStoredString& a, const plArchiveLookupString& b)
  {
    // in case that we want to lookup a string using a plArchiveLookupString, we validate
    // that the stored string is actually equal to the lookup string, to enable handling of hash collisions
    return b.m_sString.IsEqual_NoCase(reinterpret_cast<const char*>(&b.m_ArchiveAllPathStrings[a.m_uiSrcStringOffset]));
  }
};

/// \brief Table-of-contents for an plArchive file
class PL_FOUNDATION_DLL plArchiveTOC
{
public:
  /// all files stored in the plArchive
  plDynamicArray<plArchiveEntry> m_Entries;
  /// allows to map a hashed string to the index of the file entry for the file path
  plHashTable<plArchiveStoredString, plUInt32> m_PathToEntryIndex;
  /// one large array holding all path strings for the file entries, to reduce allocations
  plDynamicArray<plUInt8> m_AllPathStrings;

  /// \brief Returns the entry index for the given file or plInvalidIndex, if not found.
  plUInt32 FindEntry(plStringView sFile) const;

  plStringView GetEntryPathString(plUInt32 uiEntryIdx) const;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream, plUInt8 uiArchiveVersion);
};
