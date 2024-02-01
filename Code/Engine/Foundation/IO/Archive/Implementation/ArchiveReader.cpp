#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>

#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/Types.h>

#include <Foundation/Logging/Log.h>

plResult plArchiveReader::OpenArchive(plStringView sPath)
{
#if PL_ENABLED(PL_SUPPORTS_MEMORY_MAPPED_FILE)
  PL_LOG_BLOCK("OpenArchive", sPath);

  PL_SUCCEED_OR_RETURN(m_MemFile.Open(sPath, plMemoryMappedFile::Mode::ReadOnly));
  m_uiMemFileSize = m_MemFile.GetFileSize();

  // validate the archive
  {
    plRawMemoryStreamReader reader(m_MemFile.GetReadPointer(), m_MemFile.GetFileSize());

    plStringView extension = plPathUtils::GetFileExtension(sPath);

    if (plArchiveUtils::IsAcceptedArchiveFileExtensions(extension))
    {
      PL_SUCCEED_OR_RETURN(plArchiveUtils::ReadHeader(reader, m_uiArchiveVersion));

      m_pDataStart = m_MemFile.GetReadPointer(16, plMemoryMappedFile::OffsetBase::Start);

      PL_SUCCEED_OR_RETURN(plArchiveUtils::ExtractTOC(m_MemFile, m_ArchiveTOC, m_uiArchiveVersion));
    }

    else
    {
      plLog::Error("Unknown archive file extension '{}'", extension);
      return PL_FAILURE;
    }
  }

  // validate the entries
  {
    const plUInt32 uiMaxPathString = m_ArchiveTOC.m_AllPathStrings.GetCount();
    const plUInt64 uiValidSize = m_uiMemFileSize - uiMaxPathString;

    for (const auto& e : m_ArchiveTOC.m_Entries)
    {
      if (e.m_uiDataStartOffset + e.m_uiStoredDataSize > uiValidSize)
      {
        plLog::Error("Archive is corrupt. Invalid entry data range.");
        return PL_FAILURE;
      }

      if (e.m_uiUncompressedDataSize < e.m_uiStoredDataSize)
      {
        plLog::Error("Archive is corrupt. Invalid compression info.");
        return PL_FAILURE;
      }

      if (e.m_uiPathStringOffset >= uiMaxPathString)
      {
        plLog::Error("Archive is corrupt. Invalid entry path-string offset.");
        return PL_FAILURE;
      }
    }
  }

  return PL_SUCCESS;
#else
  PL_REPORT_FAILURE("Memory mapped files are unsupported on this platform.");
  return PL_FAILURE;
#endif
}

const plArchiveTOC& plArchiveReader::GetArchiveTOC()
{
  return m_ArchiveTOC;
}

plResult plArchiveReader::ExtractAllFiles(plStringView sTargetFolder) const
{
  PL_LOG_BLOCK("ExtractAllFiles", sTargetFolder);

  const plUInt32 numEntries = m_ArchiveTOC.m_Entries.GetCount();

  for (plUInt32 e = 0; e < numEntries; ++e)
  {
    const char* szPath = reinterpret_cast<const char*>(&m_ArchiveTOC.m_AllPathStrings[m_ArchiveTOC.m_Entries[e].m_uiPathStringOffset]);

    if (!ExtractNextFileCallback(e + 1, numEntries, szPath))
      return PL_FAILURE;

    PL_SUCCEED_OR_RETURN(ExtractFile(e, sTargetFolder));
  }

  return PL_SUCCESS;
}

void plArchiveReader::ConfigureRawMemoryStreamReader(plUInt32 uiEntryIdx, plRawMemoryStreamReader& ref_memReader) const
{
  plArchiveUtils::ConfigureRawMemoryStreamReader(m_ArchiveTOC.m_Entries[uiEntryIdx], m_pDataStart, ref_memReader);
}

plUniquePtr<plStreamReader> plArchiveReader::CreateEntryReader(plUInt32 uiEntryIdx) const
{
  return plArchiveUtils::CreateEntryReader(m_ArchiveTOC.m_Entries[uiEntryIdx], m_pDataStart);
}

plResult plArchiveReader::ExtractFile(plUInt32 uiEntryIdx, plStringView sTargetFolder) const
{
  plStringView sFilePath = m_ArchiveTOC.GetEntryPathString(uiEntryIdx);
  const plUInt64 uiMaxSize = m_ArchiveTOC.m_Entries[uiEntryIdx].m_uiUncompressedDataSize;

  plUniquePtr<plStreamReader> pReader = CreateEntryReader(uiEntryIdx);

  plStringBuilder sOutputFile = sTargetFolder;
  sOutputFile.AppendPath(sFilePath);

  plFileWriter file;
  PL_SUCCEED_OR_RETURN(file.Open(sOutputFile));

  plUInt8 uiTemp[1024 * 8];

  plUInt64 uiRead = 0;
  plUInt64 uiReadTotal = 0;
  while (true)
  {
    uiRead = pReader->ReadBytes(uiTemp, PL_ARRAY_SIZE(uiTemp));

    if (uiRead == 0)
      break;

    PL_SUCCEED_OR_RETURN(file.WriteBytes(uiTemp, uiRead));

    uiReadTotal += uiRead;

    if (!ExtractFileProgressCallback(uiReadTotal, uiMaxSize))
      return PL_FAILURE;
  }

  PL_ASSERT_DEV(uiReadTotal == uiMaxSize, "Failed to read entire file");

  return PL_SUCCESS;
}

bool plArchiveReader::ExtractNextFileCallback(plUInt32 uiCurEntry, plUInt32 uiMaxEntries, plStringView sSourceFile) const
{
  return true;
}

bool plArchiveReader::ExtractFileProgressCallback(plUInt64 bytesWritten, plUInt64 bytesTotal) const
{
  return true;
}


