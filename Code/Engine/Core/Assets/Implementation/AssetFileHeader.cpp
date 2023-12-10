#include <Core/CorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/MemoryStream.h>

static const char* g_szAssetTag = "plAsset";

plAssetFileHeader::plAssetFileHeader()
{
  // initialize to a 'valid' hash
  // this may get stored, unless someone sets the hash
  m_uiHash = 0;
  m_uiVersion = 0;
}

enum plAssetFileHeaderVersion : plUInt8
{
  Version1 = 1,
  Version2,
  Version3,

  VersionCount,
  VersionCurrent = VersionCount - 1
};

plResult plAssetFileHeader::Write(plStreamWriter& stream) const
{
  PLASMA_ASSERT_DEBUG(m_uiHash != 0xFFFFFFFFFFFFFFFF, "Cannot write an invalid hash to file");

  // 8 Bytes for identification + version
  PLASMA_SUCCEED_OR_RETURN(stream.WriteBytes(g_szAssetTag, 7));

  const plUInt8 uiVersion = plAssetFileHeaderVersion::VersionCurrent;
  stream << uiVersion;

  // 8 Bytes for the hash
  stream << m_uiHash;
  // 2 for the type version
  stream << m_uiVersion;

  stream << m_sGenerator;
  return PLASMA_SUCCESS;
}

plResult plAssetFileHeader::Read(plStreamReader& stream)
{
  // initialize to 'invalid'
  m_uiHash = 0xFFFFFFFFFFFFFFFF;
  m_uiVersion = 0;

  char szTag[8] = {0};
  if (stream.ReadBytes(szTag, 7) < 7)
  {
    PLASMA_REPORT_FAILURE("The stream does not contain a valid asset file header");
    return PLASMA_FAILURE;
  }

  szTag[7] = '\0';

  // invalid asset file ... this is not going to end well
  PLASMA_ASSERT_DEBUG(plStringUtils::IsEqual(szTag, g_szAssetTag), "The stream does not contain a valid asset file header");

  if (!plStringUtils::IsEqual(szTag, g_szAssetTag))
    return PLASMA_FAILURE;

  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  plUInt64 uiHash = 0;
  stream >> uiHash;

  // future version?
  PLASMA_ASSERT_DEV(uiVersion <= plAssetFileHeaderVersion::VersionCurrent, "Unknown asset header version {0}", uiVersion);

  if (uiVersion >= plAssetFileHeaderVersion::Version2)
  {
    stream >> m_uiVersion;
  }

  if (uiVersion >= plAssetFileHeaderVersion::Version3)
  {
    stream >> m_sGenerator;
  }

  // older version? set the hash to 'invalid'
  if (uiVersion != plAssetFileHeaderVersion::VersionCurrent)
    return PLASMA_FAILURE;

  m_uiHash = uiHash;

  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(Core, Core_Assets_Implementation_AssetFileHeader);
