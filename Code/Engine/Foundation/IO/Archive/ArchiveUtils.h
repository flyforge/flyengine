#pragma once

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

class plStreamReader;
class plStreamWriter;
class plMemoryMappedFile;
class plArchiveTOC;
class plArchiveEntry;
class plRawMemoryStreamReader;

/// \brief Utilities for working with plArchive files
namespace plArchiveUtils
{
  using FileWriteProgressCallback = plDelegate<bool(plUInt64, plUInt64)>;

  /// \brief Returns a modifiable array of file extensions that the engine considers to be valid plArchive file extensions.
  ///
  /// By default it always contains 'plArchive'.
  /// Add or overwrite the values, if you want custom file extensions to be handled as plArchives.
  PL_FOUNDATION_DLL plHybridArray<plString, 4, plStaticsAllocatorWrapper>& GetAcceptedArchiveFileExtensions();

  /// \brief Checks case insensitive, whether the given extension is in the list of GetAcceptedArchiveFileExtensions().
  PL_FOUNDATION_DLL bool IsAcceptedArchiveFileExtensions(plStringView sExtension);

  /// \brief Writes the header that identifies the plArchive file and version to the stream
  PL_FOUNDATION_DLL plResult WriteHeader(plStreamWriter& inout_stream);

  /// \brief Reads the plArchive header. Returns success and the version, if the stream is a valid plArchive file.
  PL_FOUNDATION_DLL plResult ReadHeader(plStreamReader& inout_stream, plUInt8& out_uiVersion);

  /// \brief Writes the archive TOC to the stream. This must be the last thing in the stream, if ExtractTOC() is supposed to work.
  PL_FOUNDATION_DLL plResult AppendTOC(plStreamWriter& inout_stream, const plArchiveTOC& toc);

  /// \brief Deserializes the TOC from the memory mapped file. Assumes the TOC is the very last data in the file and reads it from the back.
  PL_FOUNDATION_DLL plResult ExtractTOC(plMemoryMappedFile& ref_memFile, plArchiveTOC& ref_toc, plUInt8 uiArchiveVersion);

  /// \brief Writes a single file entry to an plArchive stream with the given compression level.
  ///
  /// Appends information to the TOC for finding the data in the stream. Reads and updates inout_uiCurrentStreamPosition with the data byte
  /// offset. The progress callback is executed for every couple of KB of data that were written.
  PL_FOUNDATION_DLL plResult WriteEntry(plStreamWriter& inout_stream, plStringView sAbsSourcePath, plUInt32 uiPathStringOffset,
    plArchiveCompressionMode compression, plInt32 iCompressionLevel, plArchiveEntry& ref_tocEntry, plUInt64& inout_uiCurrentStreamPosition,
    FileWriteProgressCallback progress = FileWriteProgressCallback());

  /// \brief Similar to WriteEntry, but if compression is enabled, checks that compression makes enough of a difference.
  /// If compression does not reduce file size enough, the file is stored uncompressed instead.
  PL_FOUNDATION_DLL plResult WriteEntryOptimal(plStreamWriter& inout_stream, plStringView sAbsSourcePath, plUInt32 uiPathStringOffset,
    plArchiveCompressionMode compression, plInt32 iCompressionLevel, plArchiveEntry& ref_tocEntry, plUInt64& inout_uiCurrentStreamPosition,
    FileWriteProgressCallback progress = FileWriteProgressCallback());

  /// \brief Configures \a memReader as a view into the data stored for \a entry in the archive file.
  ///
  /// The raw memory stream may be compressed or uncompressed. This only creates a view for the stored data, it does not interpret it.
  PL_FOUNDATION_DLL void ConfigureRawMemoryStreamReader(
    const plArchiveEntry& entry, const void* pStartOfArchiveData, plRawMemoryStreamReader& ref_memReader);

  /// \brief Creates a new stream reader which allows to read the uncompressed data for the given archive entry.
  ///
  /// Under the hood it may create different types of stream readers to uncompress or decode the data.
  PL_FOUNDATION_DLL plUniquePtr<plStreamReader> CreateEntryReader(const plArchiveEntry& entry, const void* pStartOfArchiveData);

  PL_FOUNDATION_DLL plResult ReadZipHeader(plStreamReader& inout_stream, plUInt8& out_uiVersion);
  PL_FOUNDATION_DLL plResult ExtractZipTOC(plMemoryMappedFile& ref_memFile, plArchiveTOC& ref_toc);


} // namespace plArchiveUtils
