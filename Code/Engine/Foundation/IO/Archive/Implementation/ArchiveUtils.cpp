#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveUtils.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>

plHybridArray<plString, 4, plStaticAllocatorWrapper>& plArchiveUtils::GetAcceptedArchiveFileExtensions()
{
  static plHybridArray<plString, 4, plStaticAllocatorWrapper> extensions;

  if (extensions.IsEmpty())
  {
    extensions.PushBack("plArchive");
  }

  return extensions;
}

bool plArchiveUtils::IsAcceptedArchiveFileExtensions(plStringView sExtension)
{
  for (const auto& ext : GetAcceptedArchiveFileExtensions())
  {
    if (sExtension.IsEqual_NoCase(ext.GetView()))
      return true;
  }

  return false;
}

plResult plArchiveUtils::WriteHeader(plStreamWriter& inout_stream)
{
  const char* szTag = "PLASMAARCHIVE";
  PLASMA_SUCCEED_OR_RETURN(inout_stream.WriteBytes(szTag, 10));

  const plUInt8 uiArchiveVersion = 4;

  // Version 2: Added end-of-file marker for file corruption (cutoff) detection
  // Version 3: HashedStrings changed from MurmurHash to xxHash
  // Version 4: use 64 Bit string hashes
  inout_stream << uiArchiveVersion;

  const plUInt8 uiPadding[5] = {0, 0, 0, 0, 0};
  PLASMA_SUCCEED_OR_RETURN(inout_stream.WriteBytes(uiPadding, 5));

  return PLASMA_SUCCESS;
}

plResult plArchiveUtils::ReadHeader(plStreamReader& inout_stream, plUInt8& out_uiVersion)
{
  char szTag[10];
  if (inout_stream.ReadBytes(szTag, 10) != 10 || !plStringUtils::IsEqual(szTag, "PLASMAARCHIVE"))
  {
    plLog::Error("Invalid or corrupted archive. Archive-marker not found.");
    return PLASMA_FAILURE;
  }

  out_uiVersion = 0;
  inout_stream >> out_uiVersion;

  if (out_uiVersion != 1 && out_uiVersion != 2 && out_uiVersion != 3 && out_uiVersion != 4)
  {
    plLog::Error("Unsupported archive version '{}'.", out_uiVersion);
    return PLASMA_FAILURE;
  }

  plUInt8 uiPadding[5] = {255, 255, 255, 255, 255};
  if (inout_stream.ReadBytes(uiPadding, 5) != 5)
  {
    plLog::Error("Invalid or corrupted archive. Missing header data.");
    return PLASMA_FAILURE;
  }

  const plUInt8 uiZeroPadding[5] = {0, 0, 0, 0, 0};

  if (plMemoryUtils::Compare<plUInt8>(uiPadding, uiZeroPadding, 5) != 0)
  {
    plLog::Error("Invalid or corrupted archive. Unexpected header data.");
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plResult plArchiveUtils::WriteEntry(
  plStreamWriter& inout_stream, plStringView sAbsSourcePath, plUInt32 uiPathStringOffset, plArchiveCompressionMode compression,
  plInt32 iCompressionLevel, plArchiveEntry& inout_tocEntry, plUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  plFileReader file;
  PLASMA_SUCCEED_OR_RETURN(file.Open(sAbsSourcePath, 1024 * 1024));

  const plUInt64 uiMaxBytes = file.GetFileSize();

  plUInt8 uiTemp[1024 * 8];

  inout_tocEntry.m_uiPathStringOffset = uiPathStringOffset;
  inout_tocEntry.m_uiDataStartOffset = inout_uiCurrentStreamPosition;
  inout_tocEntry.m_uiUncompressedDataSize = 0;

  plStreamWriter* pWriter = &inout_stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  plCompressedStreamWriterZstd zstdWriter;
#endif

  switch (compression)
  {
    case plArchiveCompressionMode::Uncompressed:
      break;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case plArchiveCompressionMode::Compressed_zstd:
    {
      constexpr plUInt32 uiMaxNumWorkerThreads = 12u;
      zstdWriter.SetOutputStream(&inout_stream, uiMaxNumWorkerThreads, (plCompressedStreamWriterZstd::Compression)iCompressionLevel);
      pWriter = &zstdWriter;
    }
    break;
#endif

    default:
      compression = plArchiveCompressionMode::Uncompressed;
      break;
  }

  inout_tocEntry.m_CompressionMode = compression;

  plUInt64 uiRead = 0;
  while (true)
  {
    uiRead = file.ReadBytes(uiTemp, PLASMA_ARRAY_SIZE(uiTemp));

    if (uiRead == 0)
      break;

    inout_tocEntry.m_uiUncompressedDataSize += uiRead;

    if (progress.IsValid())
    {
      if (!progress(inout_tocEntry.m_uiUncompressedDataSize, uiMaxBytes))
        return PLASMA_FAILURE;
    }

    PLASMA_SUCCEED_OR_RETURN(pWriter->WriteBytes(uiTemp, uiRead));
  }


  switch (compression)
  {
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case plArchiveCompressionMode::Compressed_zstd:
      PLASMA_SUCCEED_OR_RETURN(zstdWriter.FinishCompressedStream());
      inout_tocEntry.m_uiStoredDataSize = zstdWriter.GetWrittenBytes();
      break;
#endif

    case plArchiveCompressionMode::Uncompressed:
    default:
      inout_tocEntry.m_uiStoredDataSize = inout_tocEntry.m_uiUncompressedDataSize;
      break;
  }

  inout_uiCurrentStreamPosition += inout_tocEntry.m_uiStoredDataSize;

  return PLASMA_SUCCESS;
}

plResult plArchiveUtils::WriteEntryOptimal(plStreamWriter& inout_stream, plStringView sAbsSourcePath, plUInt32 uiPathStringOffset, plArchiveCompressionMode compression, plInt32 iCompressionLevel, plArchiveEntry& ref_tocEntry, plUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  if (compression == plArchiveCompressionMode::Uncompressed)
  {
    return WriteEntry(inout_stream, sAbsSourcePath, uiPathStringOffset, plArchiveCompressionMode::Uncompressed, iCompressionLevel, ref_tocEntry, inout_uiCurrentStreamPosition, progress);
  }
  else
  {
    plDefaultMemoryStreamStorage storage;
    plMemoryStreamWriter writer(&storage);

    plUInt64 streamPos = inout_uiCurrentStreamPosition;
    PLASMA_SUCCEED_OR_RETURN(WriteEntry(writer, sAbsSourcePath, uiPathStringOffset, compression, iCompressionLevel, ref_tocEntry, streamPos, progress));

    if (ref_tocEntry.m_uiStoredDataSize * 12 >= ref_tocEntry.m_uiUncompressedDataSize * 10)
    {
      // less than 20% size saving -> go uncompressed
      return WriteEntry(inout_stream, sAbsSourcePath, uiPathStringOffset, plArchiveCompressionMode::Uncompressed, iCompressionLevel, ref_tocEntry, inout_uiCurrentStreamPosition, progress);
    }
    else
    {
      auto res = storage.CopyToStream(inout_stream);
      inout_uiCurrentStreamPosition = streamPos;

      return res;
    }
  }
}

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

class plCompressedStreamReaderZstdWithSource : public plCompressedStreamReaderZstd
{
public:
  plRawMemoryStreamReader m_Source;
};

#endif


plUniquePtr<plStreamReader> plArchiveUtils::CreateEntryReader(const plArchiveEntry& entry, const void* pStartOfArchiveData)
{
  plUniquePtr<plStreamReader> reader;

  switch (entry.m_CompressionMode)
  {
    case plArchiveCompressionMode::Uncompressed:
    {
      reader = PLASMA_DEFAULT_NEW(plRawMemoryStreamReader);
      plRawMemoryStreamReader* pRawReader = static_cast<plRawMemoryStreamReader*>(reader.Borrow());
      ConfigureRawMemoryStreamReader(entry, pStartOfArchiveData, *pRawReader);
      break;
    }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case plArchiveCompressionMode::Compressed_zstd:
    {
      reader = PLASMA_DEFAULT_NEW(plCompressedStreamReaderZstdWithSource);
      plCompressedStreamReaderZstdWithSource* pRawReader = static_cast<plCompressedStreamReaderZstdWithSource*>(reader.Borrow());
      ConfigureRawMemoryStreamReader(entry, pStartOfArchiveData, pRawReader->m_Source);
      pRawReader->SetInputStream(&pRawReader->m_Source);
      break;
    }
#endif

    default:
      PLASMA_REPORT_FAILURE("Archive entry compression mode '{}' is not supported by plArchiveReader", (int)entry.m_CompressionMode);
      break;
  }

  return std::move(reader);
}

void plArchiveUtils::ConfigureRawMemoryStreamReader(const plArchiveEntry& entry, const void* pStartOfArchiveData, plRawMemoryStreamReader& ref_memReader)
{
  ref_memReader.Reset(plMemoryUtils::AddByteOffset(pStartOfArchiveData, static_cast<std::ptrdiff_t>(entry.m_uiDataStartOffset)), entry.m_uiStoredDataSize);
}

static const char* szEndMarker = "PLASMAARCHIVE-END";

static plUInt32 GetEndMarkerSize(plUInt8 uiFileVersion)
{
  if (uiFileVersion == 1)
    return 0;

  return 14;
}

static plUInt32 GetTocMetaSize(plUInt8 uiFileVersion)
{
  if (uiFileVersion == 1)
    return sizeof(plUInt32); // TOC size

  return sizeof(plUInt32) /* TOC size */ + sizeof(plUInt64) /* TOC hash */;
}

struct TocMetaData
{
  plUInt32 m_uiSize = 0;
  plUInt64 m_uiHash = 0;
};

plResult plArchiveUtils::AppendTOC(plStreamWriter& inout_stream, const plArchiveTOC& toc)
{
  plDefaultMemoryStreamStorage storage;
  plMemoryStreamWriter writer(&storage);

  PLASMA_SUCCEED_OR_RETURN(toc.Serialize(writer));

  PLASMA_SUCCEED_OR_RETURN(storage.CopyToStream(inout_stream));

  TocMetaData tocMeta;

  plHashStreamWriter64 hashStream(tocMeta.m_uiSize);
  PLASMA_SUCCEED_OR_RETURN(storage.CopyToStream(hashStream));

  // Added in file version 2: hash of the TOC
  tocMeta.m_uiSize = storage.GetStorageSize32();
  tocMeta.m_uiHash = hashStream.GetHashValue();

  // append the TOC meta data
  inout_stream << tocMeta.m_uiSize;
  inout_stream << tocMeta.m_uiHash;

  // write an 'end' marker
  return inout_stream.WriteBytes(szEndMarker, 14);
}

static plResult VerifyEndMarker(plMemoryMappedFile& ref_memFile, plUInt8 uiArchiveVersion)
{
  const plUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);

  if (uiEndMarkerSize == 0)
    return PLASMA_SUCCESS;

  const void* pStart = ref_memFile.GetReadPointer(uiEndMarkerSize, plMemoryMappedFile::OffsetBase::End);

  plRawMemoryStreamReader reader(pStart, uiEndMarkerSize);

  char szMarker[32] = "";
  if (reader.ReadBytes(szMarker, uiEndMarkerSize) != uiEndMarkerSize || !plStringUtils::IsEqual(szMarker, szEndMarker))
  {
    plLog::Error("Archive is corrupt or cut off. End-marker not found.");
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plResult plArchiveUtils::ExtractTOC(plMemoryMappedFile& ref_memFile, plArchiveTOC& ref_toc, plUInt8 uiArchiveVersion)
{
  PLASMA_SUCCEED_OR_RETURN(VerifyEndMarker(ref_memFile, uiArchiveVersion));

  const plUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);
  const plUInt32 uiTocMetaSize = GetTocMetaSize(uiArchiveVersion);

  plUInt32 uiTocSize = 0;
  plUInt64 uiExpectedTocHash = 0;

  // read the TOC meta data
  {
    const void* pTocMetaStart = ref_memFile.GetReadPointer(uiEndMarkerSize + uiTocMetaSize, plMemoryMappedFile::OffsetBase::End);

    plRawMemoryStreamReader tocMetaReader(pTocMetaStart, uiTocMetaSize);

    tocMetaReader >> uiTocSize;

    if (uiTocSize > 1024 * 1024 * 1024) // 1GB of TOC is enough for ~16M entries...
    {
      plLog::Error("Archive TOC is probably corrupted. Unreasonable TOC size: {0}", plArgFileSize(uiTocSize));
      return PLASMA_FAILURE;
    }

    if (uiArchiveVersion >= 2)
    {
      tocMetaReader >> uiExpectedTocHash;
    }
  }

  const void* pTocStart = ref_memFile.GetReadPointer(uiTocSize + uiTocMetaSize + uiEndMarkerSize, plMemoryMappedFile::OffsetBase::End);

  // validate the TOC hash
  if (uiArchiveVersion >= 2)
  {
    const plUInt64 uiActualTocHash = plHashingUtils::xxHash64(pTocStart, uiTocSize);
    if (uiExpectedTocHash != uiActualTocHash)
    {
      plLog::Error("Archive TOC is corrupted. Hashes do not match.");
      return PLASMA_FAILURE;
    }
  }

  // read the actual TOC data
  {
    plRawMemoryStreamReader tocReader(pTocStart, uiTocSize);

    if (ref_toc.Deserialize(tocReader, uiArchiveVersion).Failed())
    {
      plLog::Error("Failed to deserialize plArchive TOC");
      return PLASMA_FAILURE;
    }
  }

  return PLASMA_SUCCESS;
}

namespace ZipFormat
{
  constexpr plUInt32 EndOfCDMagicSignature = 0x06054b50;
  constexpr plUInt32 EndOfCDHeaderLength = 22;

  constexpr plUInt32 MaxCommentLength = 65535;
  constexpr plUInt64 MaxEndOfCDSearchLength = MaxCommentLength + EndOfCDHeaderLength;

  constexpr plUInt32 LocalFileMagicSignature = 0x04034b50;
  constexpr plUInt32 LocalFileHeaderLength = 30;

  constexpr plUInt32 CDFileMagicSignature = 0x02014b50;
  constexpr plUInt32 CDFileHeaderLength = 46;

  enum CompressionType
  {
    Uncompressed = 0,
    Deflate = 8,

  };

  struct EndOfCDHeader
  {
    plUInt32 signature;
    plUInt16 diskNumber;
    plUInt16 diskWithCD;
    plUInt16 diskEntries;
    plUInt16 totalEntries;
    plUInt32 cdSize;
    plUInt32 cdOffset;
    plUInt16 commentLength;
  };

  plStreamReader& operator>>(plStreamReader& inout_stream, EndOfCDHeader& ref_value)
  {
    inout_stream >> ref_value.signature >> ref_value.diskNumber >> ref_value.diskWithCD >> ref_value.diskEntries >> ref_value.totalEntries >> ref_value.cdSize;
    inout_stream >> ref_value.cdOffset >> ref_value.commentLength;
    PLASMA_ASSERT_DEBUG(ref_value.signature == EndOfCDMagicSignature, "ZIP: Corrupt end of central directory header.");
    return inout_stream;
  }

  struct CDFileHeader
  {
    plUInt32 signature;
    plUInt16 version;
    plUInt16 versionNeeded;
    plUInt16 flags;
    plUInt16 compression;
    plUInt16 modTime;
    plUInt16 modDate;
    plUInt32 crc32;
    plUInt32 compressedSize;
    plUInt32 uncompressedSize;
    plUInt16 fileNameLength;
    plUInt16 extraFieldLength;
    plUInt16 fileCommentLength;
    plUInt16 diskNumStart;
    plUInt16 internalAttr;
    plUInt32 externalAttr;
    plUInt32 offsetLocalHeader;
  };

  plStreamReader& operator>>(plStreamReader& inout_stream, CDFileHeader& ref_value)
  {
    inout_stream >> ref_value.signature >> ref_value.version >> ref_value.versionNeeded >> ref_value.flags >> ref_value.compression >> ref_value.modTime >> ref_value.modDate;
    inout_stream >> ref_value.crc32 >> ref_value.compressedSize >> ref_value.uncompressedSize >> ref_value.fileNameLength >> ref_value.extraFieldLength;
    inout_stream >> ref_value.fileCommentLength >> ref_value.diskNumStart >> ref_value.internalAttr >> ref_value.externalAttr >> ref_value.offsetLocalHeader;
    PLASMA_ASSERT_DEBUG(ref_value.signature == CDFileMagicSignature, "ZIP: Corrupt central directory file entry header.");
    return inout_stream;
  }

  struct LocalFileHeader
  {
    plUInt32 signature;
    plUInt16 version;
    plUInt16 flags;
    plUInt16 compression;
    plUInt16 modTime;
    plUInt16 modDate;
    plUInt32 crc32;
    plUInt32 compressedSize;
    plUInt32 uncompressedSize;
    plUInt16 fileNameLength;
    plUInt16 extraFieldLength;
  };

  plStreamReader& operator>>(plStreamReader& inout_stream, LocalFileHeader& ref_value)
  {
    inout_stream >> ref_value.signature >> ref_value.version >> ref_value.flags >> ref_value.compression >> ref_value.modTime >> ref_value.modDate >> ref_value.crc32;
    inout_stream >> ref_value.compressedSize >> ref_value.uncompressedSize >> ref_value.fileNameLength >> ref_value.extraFieldLength;
    PLASMA_ASSERT_DEBUG(ref_value.signature == LocalFileMagicSignature, "ZIP: Corrupt local file entry header.");
    return inout_stream;
  }
}; // namespace ZipFormat

plResult plArchiveUtils::ReadZipHeader(plStreamReader& inout_stream, plUInt8& out_uiVersion)
{
  using namespace ZipFormat;

  plUInt32 header;
  inout_stream >> header;
  if (header == LocalFileMagicSignature)
  {
    out_uiVersion = 0;
    return PLASMA_SUCCESS;
  }
  return PLASMA_SUCCESS;
}

plResult plArchiveUtils::ExtractZipTOC(plMemoryMappedFile& ref_memFile, plArchiveTOC& ref_toc)
{
  using namespace ZipFormat;

  const plUInt8* pEndOfCDStart = nullptr;
  {
    // Find End of CD signature by searching from the end of the file.
    // As a comment can come after it we have to potentially walk max comment length backwards.
    const plUInt64 SearchEnd = ref_memFile.GetFileSize() - plMath::Min(MaxEndOfCDSearchLength, ref_memFile.GetFileSize());
    const plUInt8* pSearchEnd = static_cast<const plUInt8*>(ref_memFile.GetReadPointer(SearchEnd, plMemoryMappedFile::OffsetBase::End));
    const plUInt8* pSearchStart = static_cast<const plUInt8*>(ref_memFile.GetReadPointer(EndOfCDHeaderLength, plMemoryMappedFile::OffsetBase::End));
    while (pSearchStart >= pSearchEnd)
    {
      if (*reinterpret_cast<const plUInt32*>(pSearchStart) == EndOfCDMagicSignature)
      {
        pEndOfCDStart = pSearchStart;
        break;
      }
      pSearchStart--;
    }
    if (pEndOfCDStart == nullptr)
      return PLASMA_FAILURE;
  }

  plRawMemoryStreamReader tocReader(pEndOfCDStart, EndOfCDHeaderLength);
  EndOfCDHeader ecdHeader;
  tocReader >> ecdHeader;

  ref_toc.m_Entries.Reserve(ecdHeader.diskEntries);
  ref_toc.m_PathToEntryIndex.Reserve(ecdHeader.diskEntries);

  plStringBuilder sLowerCaseHash;
  plUInt64 uiEntryOffset = 0;
  for (plUInt16 uiEntry = 0; uiEntry < ecdHeader.diskEntries; ++uiEntry)
  {
    // First, read the current file's header from the central directory
    const void* pCdfStart = ref_memFile.GetReadPointer(ecdHeader.cdOffset + uiEntryOffset, plMemoryMappedFile::OffsetBase::Start);
    plRawMemoryStreamReader cdfReader(pCdfStart, ecdHeader.cdSize - uiEntryOffset);
    CDFileHeader cdfHeader;
    cdfReader >> cdfHeader;

    if (cdfHeader.compression == CompressionType::Uncompressed || cdfHeader.compression == CompressionType::Deflate)
    {
      auto& entry = ref_toc.m_Entries.ExpandAndGetRef();
      entry.m_uiUncompressedDataSize = cdfHeader.uncompressedSize;
      entry.m_uiStoredDataSize = cdfHeader.compressedSize;
      entry.m_uiPathStringOffset = ref_toc.m_AllPathStrings.GetCount();
      entry.m_CompressionMode = cdfHeader.compression == CompressionType::Uncompressed ? plArchiveCompressionMode::Uncompressed : plArchiveCompressionMode::Compressed_zip;

      auto nameBuffer = plArrayPtr<const plUInt8>(static_cast<const plUInt8*>(pCdfStart) + CDFileHeaderLength, cdfHeader.fileNameLength);
      ref_toc.m_AllPathStrings.PushBackRange(nameBuffer);
      ref_toc.m_AllPathStrings.PushBack(0);
      const char* szName = reinterpret_cast<const char*>(ref_toc.m_AllPathStrings.GetData() + entry.m_uiPathStringOffset);
      sLowerCaseHash = szName;
      sLowerCaseHash.ToLower();
      ref_toc.m_PathToEntryIndex.Insert(plArchiveStoredString(plHashingUtils::StringHash(sLowerCaseHash), entry.m_uiPathStringOffset), ref_toc.m_Entries.GetCount() - 1);

      // Compute data stream start location. We need to skip past the local (and redundant) file header to find it.
      const void* pLfStart = ref_memFile.GetReadPointer(cdfHeader.offsetLocalHeader, plMemoryMappedFile::OffsetBase::Start);
      plRawMemoryStreamReader lfReader(pLfStart, ref_memFile.GetFileSize() - cdfHeader.offsetLocalHeader);
      LocalFileHeader lfHeader;
      lfReader >> lfHeader;
      entry.m_uiDataStartOffset = cdfHeader.offsetLocalHeader + LocalFileHeaderLength + lfHeader.fileNameLength + lfHeader.extraFieldLength;
    }
    // Compute next file header location.
    uiEntryOffset += CDFileHeaderLength + cdfHeader.fileNameLength + cdfHeader.extraFieldLength + cdfHeader.fileCommentLength;
  }

  return PLASMA_SUCCESS;
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_ArchiveUtils);
