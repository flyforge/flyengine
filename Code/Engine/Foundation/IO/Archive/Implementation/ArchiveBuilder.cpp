#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveBuilder.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Stopwatch.h>

void plArchiveBuilder::AddFolder(plStringView sAbsFolderPath, plArchiveCompressionMode defaultMode /*= plArchiveCompressionMode::Uncompressed*/, InclusionCallback callback /*= InclusionCallback()*/)
{
#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS)
  plFileSystemIterator fileIt;

  plStringBuilder sBasePath = sAbsFolderPath;
  sBasePath.MakeCleanPath();

  plStringBuilder fullPath;
  plStringBuilder relPath;

  for (fileIt.StartSearch(sBasePath, plFileSystemIteratorFlags::ReportFilesRecursive); fileIt.IsValid(); fileIt.Next())
  {
    const auto& stat = fileIt.GetStats();

    stat.GetFullPath(fullPath);
    relPath = fullPath;

    if (relPath.MakeRelativeTo(sBasePath).Succeeded())
    {
      plArchiveCompressionMode compression = defaultMode;
      plInt32 iCompressionLevel = 0;

      if (callback.IsValid())
      {
        switch (callback(fullPath))
        {
          case InclusionMode::Exclude:
            continue;

          case InclusionMode::Uncompressed:
            compression = plArchiveCompressionMode::Uncompressed;
            break;

#  ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
          case InclusionMode::Compress_zstd_fastest:
            compression = plArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<plInt32>(plCompressedStreamWriterZstd::Compression::Fastest);
            break;
          case InclusionMode::Compress_zstd_fast:
            compression = plArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<plInt32>(plCompressedStreamWriterZstd::Compression::Fast);
            break;
          case InclusionMode::Compress_zstd_average:
            compression = plArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<plInt32>(plCompressedStreamWriterZstd::Compression::Average);
            break;
          case InclusionMode::Compress_zstd_high:
            compression = plArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<plInt32>(plCompressedStreamWriterZstd::Compression::High);
            break;
          case InclusionMode::Compress_zstd_highest:
            compression = plArchiveCompressionMode::Compressed_zstd;
            iCompressionLevel = static_cast<plInt32>(plCompressedStreamWriterZstd::Compression::Highest);
            break;
#  endif
        }
      }

      auto& e = m_Entries.ExpandAndGetRef();
      e.m_sAbsSourcePath = fullPath;
      e.m_sRelTargetPath = relPath;
      e.m_CompressionMode = compression;
      e.m_iCompressionLevel = iCompressionLevel;
    }
  }

#else
  PLASMA_ASSERT_NOT_IMPLEMENTED;
#endif
}

plResult plArchiveBuilder::WriteArchive(plStringView sFile) const
{
  PLASMA_LOG_BLOCK("WriteArchive", sFile);

  plFileWriter file;
  if (file.Open(sFile, 1024 * 1024 * 16).Failed())
  {
    plLog::Error("Could not open file for writing archive to: '{}'", sFile);
    return PLASMA_FAILURE;
  }

  return WriteArchive(file);
}

plResult plArchiveBuilder::WriteArchive(plStreamWriter& inout_stream) const
{
  PLASMA_SUCCEED_OR_RETURN(plArchiveUtils::WriteHeader(inout_stream));

  plArchiveTOC toc;

  plStringBuilder sHashablePath;

  plUInt64 uiStreamSize = 0;
  const plUInt32 uiNumEntries = m_Entries.GetCount();

  plStopwatch sw;

  for (plUInt32 i = 0; i < uiNumEntries; ++i)
  {
    const SourceEntry& e = m_Entries[i];

    const plUInt32 uiPathStringOffset = toc.m_AllPathStrings.GetCount();
    toc.m_AllPathStrings.PushBackRange(plArrayPtr<const plUInt8>(reinterpret_cast<const plUInt8*>(e.m_sRelTargetPath.GetData()), e.m_sRelTargetPath.GetElementCount() + 1));

    sHashablePath = e.m_sRelTargetPath;
    sHashablePath.ToLower();

    toc.m_PathToEntryIndex[plArchiveStoredString(plHashingUtils::StringHash(sHashablePath), uiPathStringOffset)] = toc.m_Entries.GetCount();

    if (!WriteNextFileCallback(i + 1, uiNumEntries, e.m_sAbsSourcePath))
      return PLASMA_FAILURE;

    plArchiveEntry& tocEntry = toc.m_Entries.ExpandAndGetRef();

    PLASMA_SUCCEED_OR_RETURN(plArchiveUtils::WriteEntryOptimal(inout_stream, e.m_sAbsSourcePath, uiPathStringOffset, e.m_CompressionMode, e.m_iCompressionLevel, tocEntry, uiStreamSize, plMakeDelegate(&plArchiveBuilder::WriteFileProgressCallback, this)));

    WriteFileResultCallback(i + 1, uiNumEntries, e.m_sAbsSourcePath, tocEntry.m_uiUncompressedDataSize, tocEntry.m_uiStoredDataSize, sw.Checkpoint());
  }

  PLASMA_SUCCEED_OR_RETURN(plArchiveUtils::AppendTOC(inout_stream, toc));

  return PLASMA_SUCCESS;
}

bool plArchiveBuilder::WriteNextFileCallback(plUInt32 uiCurEntry, plUInt32 uiMaxEntries, plStringView sSourceFile) const
{
  return true;
}

bool plArchiveBuilder::WriteFileProgressCallback(plUInt64 bytesWritten, plUInt64 bytesTotal) const
{
  return true;
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_ArchiveBuilder);
