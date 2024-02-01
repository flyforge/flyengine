#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>

static const char* g_szAssetTag = "plAsset";

plAssetFileHeader::plAssetFileHeader() = default;

enum plAssetFileHeaderVersion : plUInt8
{
  Version1 = 1,
  Version2,
  Version3,

  VersionCount,
  VersionCurrent = VersionCount - 1
};

plResult plAssetFileHeader::Write(plStreamWriter& inout_stream) const
{
  PL_ASSERT_DEBUG(m_uiHash != 0xFFFFFFFFFFFFFFFF, "Cannot write an invalid hash to file");

  // 8 Bytes for identification + version
  PL_SUCCEED_OR_RETURN(inout_stream.WriteBytes(g_szAssetTag, 7));

  const plUInt8 uiVersion = plAssetFileHeaderVersion::VersionCurrent;
  inout_stream << uiVersion;

  // 8 Bytes for the hash
  inout_stream << m_uiHash;
  // 2 for the type version
  inout_stream << m_uiVersion;

  inout_stream << m_sGenerator;
  return PL_SUCCESS;
}

plResult plAssetFileHeader::Read(plStreamReader& inout_stream)
{
  // initialize to 'invalid'
  m_uiHash = 0xFFFFFFFFFFFFFFFF;
  m_uiVersion = 0;

  char szTag[8] = {0};
  if (inout_stream.ReadBytes(szTag, 7) < 7)
  {
    PL_REPORT_FAILURE("The stream does not contain a valid asset file header");
    return PL_FAILURE;
  }

  szTag[7] = '\0';

  // invalid asset file ... this is not going to end well
  PL_ASSERT_DEBUG(plStringUtils::IsEqual(szTag, g_szAssetTag), "The stream does not contain a valid asset file header");

  if (!plStringUtils::IsEqual(szTag, g_szAssetTag))
    return PL_FAILURE;

  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  plUInt64 uiHash = 0;
  inout_stream >> uiHash;

  // future version?
  PL_ASSERT_DEV(uiVersion <= plAssetFileHeaderVersion::VersionCurrent, "Unknown asset header version {0}", uiVersion);

  if (uiVersion >= plAssetFileHeaderVersion::Version2)
  {
    inout_stream >> m_uiVersion;
  }

  if (uiVersion >= plAssetFileHeaderVersion::Version3)
  {
    inout_stream >> m_sGenerator;
  }

  // older version? set the hash to 'invalid'
  if (uiVersion != plAssetFileHeaderVersion::VersionCurrent)
    return PL_FAILURE;

  m_uiHash = uiHash;

  return PL_SUCCESS;
}
