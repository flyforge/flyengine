#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>

/// \brief Simple class to handle asset file headers (the very first bytes in all transformed asset files)
class PL_FOUNDATION_DLL plAssetFileHeader
{
public:
  plAssetFileHeader();

  /// \brief Reads the hash from file. If the file is outdated, the hash is set to 0xFFFFFFFFFFFFFFFF.
  plResult Read(plStreamReader& inout_stream);

  /// \brief Writes the asset hash to file (plus a little version info)
  plResult Write(plStreamWriter& inout_stream) const;

  /// \brief Checks whether the stored file contains the same hash.
  bool IsFileUpToDate(plUInt64 uiExpectedHash, plUInt16 uiVersion) const { return (m_uiHash == uiExpectedHash && m_uiVersion == uiVersion); }

  /// \brief Returns the asset file hash
  plUInt64 GetFileHash() const { return m_uiHash; }

  /// \brief Sets the asset file hash
  void SetFileHashAndVersion(plUInt64 uiHash, plUInt16 v)
  {
    m_uiHash = uiHash;
    m_uiVersion = v;
  }

  /// \brief Returns the asset type version
  plUInt16 GetFileVersion() const { return m_uiVersion; }

  /// \brief Returns the generator which was used to produce the asset file
  const plHashedString& GetGenerator() { return m_sGenerator; }

  /// \brief Allows to set the generator string
  void SetGenerator(plStringView sGenerator) { m_sGenerator.Assign(sGenerator); }

private:
  // initialize to a 'valid' hash
  // this may get stored, unless someone sets the hash
  plUInt64 m_uiHash = 0;
  plUInt16 m_uiVersion = 0;
  plHashedString m_sGenerator;
};
