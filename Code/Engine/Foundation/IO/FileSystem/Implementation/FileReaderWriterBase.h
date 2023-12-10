#pragma once

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/Stream.h>

/// The base class for all file readers.
/// Provides access to plFileSystem::GetFileReader, which is necessary to get access to the streams that
/// plDataDirectoryType's provide.
/// Derive from this class if you want to implement different policies on how to read files.
/// E.g. the default reader (plFileReader) implements a buffered read policy (using an internal cache).
class PLASMA_FOUNDATION_DLL plFileReaderBase : public plStreamReader
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plFileReaderBase);

public:
  plFileReaderBase() { m_pDataDirReader = nullptr; }

  /// Returns the absolute path with which the file was opened (including the prefix of the data directory).
  plString128 GetFilePathAbsolute() const
  {
    plStringBuilder sAbs = m_pDataDirReader->GetDataDirectory()->GetRedirectedDataDirectoryPath();
    sAbs.AppendPath(m_pDataDirReader->GetFilePath().GetView());
    return sAbs;
  }

  /// Returns the relative path of the file within its data directory (excluding the prefix of the data directory).
  plString128 GetFilePathRelative() const { return m_pDataDirReader->GetFilePath(); }

  /// Returns the plDataDirectoryType over which this file has been opened.
  plDataDirectoryType* GetDataDirectory() const { return m_pDataDirReader->GetDataDirectory(); }

  /// Returns true, if the file is currently open.
  bool IsOpen() const { return m_pDataDirReader != nullptr; }

  /// \brief Returns the current total size of the file.
  plUInt64 GetFileSize() const { return m_pDataDirReader->GetFileSize(); }

protected:
  plDataDirectoryReader* GetFileReader(plStringView sFile, plFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
  {
    return plFileSystem::GetFileReader(sFile, FileShareMode, bAllowFileEvents);
  }

  plDataDirectoryReader* m_pDataDirReader;
};


/// The base class for all file writers.
/// Provides access to plFileSystem::GetFileWriter, which is necessary to get access to the streams that
/// plDataDirectoryType's provide.
/// Derive from this class if you want to implement different policies on how to write files.
/// E.g. the default writer (plFileWriter) implements a buffered write policy (using an internal cache).
class PLASMA_FOUNDATION_DLL plFileWriterBase : public plStreamWriter
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plFileWriterBase);

public:
  plFileWriterBase() { m_pDataDirWriter = nullptr; }

  /// Returns the absolute path with which the file was opened (including the prefix of the data directory).
  plString128 GetFilePathAbsolute() const
  {
    plStringBuilder sAbs = m_pDataDirWriter->GetDataDirectory()->GetRedirectedDataDirectoryPath();
    sAbs.AppendPath(m_pDataDirWriter->GetFilePath().GetView());
    return sAbs;
  }

  /// Returns the relative path of the file within its data directory (excluding the prefix of the data directory).
  plString128 GetFilePathRelative() const { return m_pDataDirWriter->GetFilePath(); }

  /// Returns the plDataDirectoryType over which this file has been opened.
  plDataDirectoryType* GetDataDirectory() const { return m_pDataDirWriter->GetDataDirectory(); }

  /// Returns true, if the file is currently open.
  bool IsOpen() const { return m_pDataDirWriter != nullptr; }

  /// \brief Returns the current total size of the file.
  plUInt64 GetFileSize() const { return m_pDataDirWriter->GetFileSize(); } // [tested]

protected:
  plDataDirectoryWriter* GetFileWriter(plStringView sFile, plFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
  {
    return plFileSystem::GetFileWriter(sFile, FileShareMode, bAllowFileEvents);
  }

  plDataDirectoryWriter* m_pDataDirWriter;
};
