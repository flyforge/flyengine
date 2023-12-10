#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/Logging/Log.h>

void operator<<(plStreamWriter& inout_stream, const plArchiveStoredString& value)
{
  inout_stream << value.m_uiLowerCaseHash;
  inout_stream << value.m_uiSrcStringOffset;
}

void operator>>(plStreamReader& inout_stream, plArchiveStoredString& value)
{
  inout_stream >> value.m_uiLowerCaseHash;
  inout_stream >> value.m_uiSrcStringOffset;
}

plUInt32 plArchiveTOC::FindEntry(plStringView sFile) const
{
  plStringBuilder sLowerCasePath = sFile;
  sLowerCasePath.ToLower();

  plUInt32 uiIndex;

  plArchiveLookupString lookup(plHashingUtils::StringHash(sLowerCasePath.GetView()), sLowerCasePath, m_AllPathStrings);

  if (!m_PathToEntryIndex.TryGetValue(lookup, uiIndex))
    return plInvalidIndex;

  PLASMA_ASSERT_DEBUG(sFile.IsEqual_NoCase(GetEntryPathString(uiIndex)), "Hash table corruption detected.");
  return uiIndex;
}

plStringView plArchiveTOC::GetEntryPathString(plUInt32 uiEntryIdx) const
{
  return reinterpret_cast<const char*>(&m_AllPathStrings[m_Entries[uiEntryIdx].m_uiPathStringOffset]);
}

plResult plArchiveTOC::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(2);

  PLASMA_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Entries));

  // write the hash of a known string to the archive, to detect hash function changes
  plUInt64 uiStringHash = plHashingUtils::StringHash("plArchive");
  inout_stream << uiStringHash;

  PLASMA_SUCCEED_OR_RETURN(inout_stream.WriteHashTable(m_PathToEntryIndex));

  PLASMA_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_AllPathStrings));

  return PLASMA_SUCCESS;
}

struct plOldTempHashedString
{
  plUInt32 m_uiHash = 0;

  plResult Deserialize(plStreamReader& r)
  {
    r >> m_uiHash;
    return PLASMA_SUCCESS;
  }

  bool operator==(const plOldTempHashedString& rhs) const
  {
    return m_uiHash == rhs.m_uiHash;
  }
};

template <>
struct plHashHelper<plOldTempHashedString>
{
  static plUInt32 Hash(const plOldTempHashedString& value)
  {
    return value.m_uiHash;
  }

  static bool Equal(const plOldTempHashedString& a, const plOldTempHashedString& b) { return a == b; }
};

plResult plArchiveTOC::Deserialize(plStreamReader& inout_stream, plUInt8 uiArchiveVersion)
{
  PLASMA_ASSERT_ALWAYS(uiArchiveVersion <= 4, "Unsupported archive version {}", uiArchiveVersion);

  // we don't use the TOC version anymore, but the archive version instead
  const plTypeVersion version = inout_stream.ReadVersion(2);

  PLASMA_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Entries));

  bool bRecreateStringHashes = true;

  if (version == 1)
  {
    // read and discard the data, it is regenerated below
    plHashTable<plOldTempHashedString, plUInt32> m_PathToIndex;
    PLASMA_SUCCEED_OR_RETURN(inout_stream.ReadHashTable(m_PathToIndex));
  }
  else
  {
    if (uiArchiveVersion >= 4)
    {
      // read the hash of a known string from the archive, to detect hash function changes
      plUInt64 uiStringHash = 0;
      inout_stream >> uiStringHash;

      if (uiStringHash == plHashingUtils::StringHash("plArchive"))
      {
        bRecreateStringHashes = false;
      }
    }

    PLASMA_SUCCEED_OR_RETURN(inout_stream.ReadHashTable(m_PathToEntryIndex));
  }

  PLASMA_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_AllPathStrings));

  if (bRecreateStringHashes)
  {
    plLog::Info("Archive uses older string hashing, recomputing hashes.");

    // version 1 stores an older way for the path/hash -> entry lookup table, which is prone to hash collisions
    // in this case, rebuild the new hash table on the fly
    //
    // version 2 used MurmurHash
    // version 3 switched to 32 bit xxHash
    // version 4 switched to 64 bit hashes

    const plUInt32 uiNumEntries = m_Entries.GetCount();
    m_PathToEntryIndex.Clear();
    m_PathToEntryIndex.Reserve(uiNumEntries);

    plStringBuilder sLowerCasePath;

    for (plUInt32 i = 0; i < uiNumEntries; i++)
    {
      const plUInt32 uiSrcStringOffset = m_Entries[i].m_uiPathStringOffset;

      plStringView sEntryString = GetEntryPathString(i);

      sLowerCasePath = sEntryString;
      sLowerCasePath.ToLower();

      // cut off the upper 32 bit, we don't need them here
      const plUInt32 uiLowerCaseHash = plHashingUtils::StringHashTo32(plHashingUtils::StringHash(sLowerCasePath.GetView()) & 0xFFFFFFFFllu);

      m_PathToEntryIndex.Insert(plArchiveStoredString(uiLowerCaseHash, uiSrcStringOffset), i);

      // Verify that the conversion worked
      PLASMA_ASSERT_DEBUG(FindEntry(sEntryString) == i, "Hashed path retrieval did not yield inserted index");
    }
  }

  // path strings mustn't be empty and must be zero-terminated
  if (m_AllPathStrings.IsEmpty() || m_AllPathStrings.PeekBack() != '\0')
  {
    plLog::Error("Archive is corrupt. Invalid string data.");
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plResult plArchiveEntry::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream << m_uiDataStartOffset;
  inout_stream << m_uiUncompressedDataSize;
  inout_stream << m_uiStoredDataSize;
  inout_stream << (plUInt8)m_CompressionMode;
  inout_stream << m_uiPathStringOffset;

  return PLASMA_SUCCESS;
}

plResult plArchiveEntry::Deserialize(plStreamReader& inout_stream)
{
  inout_stream >> m_uiDataStartOffset;
  inout_stream >> m_uiUncompressedDataSize;
  inout_stream >> m_uiStoredDataSize;
  plUInt8 uiCompressionMode = 0;
  inout_stream >> uiCompressionMode;
  m_CompressionMode = (plArchiveCompressionMode)uiCompressionMode;
  inout_stream >> m_uiPathStringOffset;

  return PLASMA_SUCCESS;
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_Archive);
