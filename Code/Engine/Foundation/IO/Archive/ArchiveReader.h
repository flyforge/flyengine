#pragma once

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Types/UniquePtr.h>

class plRawMemoryStreamReader;
class plStreamReader;

/// \brief A utility class for reading from plArchive files
class PL_FOUNDATION_DLL plArchiveReader
{
public:
  /// \brief Opens the given file and validates that it is a valid archive file.
  plResult OpenArchive(plStringView sPath);

  /// \brief Returns the table-of-contents for the previously opened archive.
  const plArchiveTOC& GetArchiveTOC();

  /// \brief Extracts the given entry to the target folder.
  ///
  /// Calls ExtractFileProgressCallback() to report progress.
  plResult ExtractFile(plUInt32 uiEntryIdx, plStringView sTargetFolder) const;

  /// \brief Extracts all files to the target folder.
  ///
  /// Calls ExtractNextFileCallback() for every file that is being extracted.
  plResult ExtractAllFiles(plStringView sTargetFolder) const;

  /// \brief Sets up \a memReader for reading the raw (potentially compressed) data that is stored for the given entry in the archive.
  void ConfigureRawMemoryStreamReader(plUInt32 uiEntryIdx, plRawMemoryStreamReader& ref_memReader) const;

  /// \brief Creates a reader that will decompress the given file entry.
  plUniquePtr<plStreamReader> CreateEntryReader(plUInt32 uiEntryIdx) const;

protected:
  /// \brief Called by ExtractAllFiles() for progress reporting. Return false to abort.
  virtual bool ExtractNextFileCallback(plUInt32 uiCurEntry, plUInt32 uiMaxEntries, plStringView sSourceFile) const;

  /// \brief Called by ExtractFile() for progress reporting. Return false to abort.
  virtual bool ExtractFileProgressCallback(plUInt64 bytesWritten, plUInt64 bytesTotal) const;

  plMemoryMappedFile m_MemFile;
  plArchiveTOC m_ArchiveTOC;
  plUInt8 m_uiArchiveVersion = 0;
  const void* m_pDataStart = nullptr;
  plUInt64 m_uiMemFileSize = 0;
};
