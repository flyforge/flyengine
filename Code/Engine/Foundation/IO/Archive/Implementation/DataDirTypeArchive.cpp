#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ArchiveDataDirectory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "FileSystem", "FolderDataDirectory"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  plFileSystem::RegisterDataDirectoryFactory(plDataDirectory::ArchiveType::Factory);
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plDataDirectory::ArchiveType::ArchiveType() = default;
plDataDirectory::ArchiveType::~ArchiveType() = default;

plDataDirectoryType* plDataDirectory::ArchiveType::Factory(plStringView sDataDirectory, plStringView sGroup, plStringView sRootName, plFileSystem::DataDirUsage usage)
{
  ArchiveType* pDataDir = PLASMA_DEFAULT_NEW(ArchiveType);

  if (pDataDir->InitializeDataDirectory(sDataDirectory) == PLASMA_SUCCESS)
    return pDataDir;

  PLASMA_DEFAULT_DELETE(pDataDir);
  return nullptr;
}

plDataDirectoryReader* plDataDirectory::ArchiveType::OpenFileToRead(plStringView sFile, plFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir)
{
  const plArchiveTOC& toc = m_ArchiveReader.GetArchiveTOC();
  plStringBuilder sArchivePath = m_sArchiveSubFolder;
  sArchivePath.AppendPath(sFile);

  const plUInt32 uiEntryIndex = toc.FindEntry(sArchivePath);

  if (uiEntryIndex == plInvalidIndex)
    return nullptr;

  const plArchiveEntry* pEntry = &toc.m_Entries[uiEntryIndex];

  ArchiveReaderUncompressed* pReader = nullptr;

  {
    PLASMA_LOCK(m_ReaderMutex);

    switch (pEntry->m_CompressionMode)
    {
      case plArchiveCompressionMode::Uncompressed:
      {
        if (!m_FreeReadersUncompressed.IsEmpty())
        {
          pReader = m_FreeReadersUncompressed.PeekBack();
          m_FreeReadersUncompressed.PopBack();
        }
        else
        {
          m_ReadersUncompressed.PushBack(PLASMA_DEFAULT_NEW(ArchiveReaderUncompressed, 0));
          pReader = m_ReadersUncompressed.PeekBack().Borrow();
        }
        break;
      }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      case plArchiveCompressionMode::Compressed_zstd:
      {
        if (!m_FreeReadersZstd.IsEmpty())
        {
          pReader = m_FreeReadersZstd.PeekBack();
          m_FreeReadersZstd.PopBack();
        }
        else
        {
          m_ReadersZstd.PushBack(PLASMA_DEFAULT_NEW(ArchiveReaderZstd, 1));
          pReader = m_ReadersZstd.PeekBack().Borrow();
        }
        break;
      }
#endif

      default:
        PLASMA_REPORT_FAILURE("Compression mode {} is unknown (or not compiled in)", (plUInt8)pEntry->m_CompressionMode);
        return nullptr;
    }
  }

  pReader->m_uiUncompressedSize = pEntry->m_uiUncompressedDataSize;
  pReader->m_uiCompressedSize = pEntry->m_uiStoredDataSize;

  m_ArchiveReader.ConfigureRawMemoryStreamReader(uiEntryIndex, pReader->m_MemStreamReader);

  if (pReader->Open(sArchivePath, this, FileShareMode).Failed())
  {
    PLASMA_DEFAULT_DELETE(pReader);
    return nullptr;
  }

  return pReader;
}

void plDataDirectory::ArchiveType::RemoveDataDirectory()
{
  ArchiveType* pThis = this;
  PLASMA_DEFAULT_DELETE(pThis);
}

bool plDataDirectory::ArchiveType::ExistsFile(plStringView sFile, bool bOneSpecificDataDir)
{
  plStringBuilder sArchivePath = m_sArchiveSubFolder;
  sArchivePath.AppendPath(sFile);
  return m_ArchiveReader.GetArchiveTOC().FindEntry(sArchivePath) != plInvalidIndex;
}

plResult plDataDirectory::ArchiveType::GetFileStats(plStringView sFileOrFolder, bool bOneSpecificDataDir, plFileStats& out_Stats)
{
  const plArchiveTOC& toc = m_ArchiveReader.GetArchiveTOC();
  plStringBuilder sArchivePath = m_sArchiveSubFolder;
  sArchivePath.AppendPath(sFileOrFolder);
  const plUInt32 uiEntryIndex = toc.FindEntry(sArchivePath);

  if (uiEntryIndex == plInvalidIndex)
    return PLASMA_FAILURE;

  const plArchiveEntry* pEntry = &toc.m_Entries[uiEntryIndex];

  const plStringView sPath = toc.GetEntryPathString(uiEntryIndex);

  out_Stats.m_bIsDirectory = false;
  out_Stats.m_LastModificationTime = m_LastModificationTime;
  out_Stats.m_uiFileSize = pEntry->m_uiUncompressedDataSize;
  out_Stats.m_sParentPath = sPath;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = plPathUtils::GetFileNameAndExtension(sPath);

  return PLASMA_SUCCESS;
}

plResult plDataDirectory::ArchiveType::InternalInitializeDataDirectory(plStringView sDirectory)
{
  plStringBuilder sRedirected;
  PLASMA_SUCCEED_OR_RETURN(plFileSystem::ResolveSpecialDirectory(sDirectory, sRedirected));

  sRedirected.MakeCleanPath();
  // remove trailing slashes
  sRedirected.Trim("", "/");
  m_sRedirectedDataDirPath = sRedirected;

  bool bSupported = false;
  plStringBuilder sArchivePath;

  plHybridArray<plString, 4, plStaticAllocatorWrapper> extensions = plArchiveUtils::GetAcceptedArchiveFileExtensions();


  for (const auto& ext : extensions)
  {
    const plUInt32 uiLength = ext.GetElementCount();
    if (sRedirected.HasExtension(ext))
    {
      sArchivePath = sRedirected;
      m_sArchiveSubFolder = "";
      bSupported = true;
      goto endloop;
    }
    const char* szFound = nullptr;
    do
    {
      szFound = sRedirected.FindLastSubString_NoCase(ext, szFound);
      if (szFound != nullptr && szFound[uiLength] == '/')
      {
        sArchivePath = plStringView(sRedirected.GetData(), szFound + uiLength);
        m_sArchiveSubFolder = szFound + uiLength + 1;
        bSupported = true;
        goto endloop;
      }

    } while (szFound != nullptr);
  }
endloop:
  if (!bSupported)
    return PLASMA_FAILURE;

  plFileStats stats;
  if (plOSFile::GetFileStats(sArchivePath, stats).Failed())
    return PLASMA_FAILURE;

  PLASMA_LOG_BLOCK("plArchiveDataDir", sDirectory);

  m_LastModificationTime = stats.m_LastModificationTime;

  PLASMA_SUCCEED_OR_RETURN(m_ArchiveReader.OpenArchive(sArchivePath));

  ReloadExternalConfigs();

  return PLASMA_SUCCESS;
}

void plDataDirectory::ArchiveType::OnReaderWriterClose(plDataDirectoryReaderWriterBase* pClosed)
{
  PLASMA_LOCK(m_ReaderMutex);

  if (pClosed->GetDataDirUserData() == 0)
  {
    m_FreeReadersUncompressed.PushBack(static_cast<ArchiveReaderUncompressed*>(pClosed));
    return;
  }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  if (pClosed->GetDataDirUserData() == 1)
  {
    m_FreeReadersZstd.PushBack(static_cast<ArchiveReaderZstd*>(pClosed));
    return;
  }
#endif


  PLASMA_ASSERT_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////////

plDataDirectory::ArchiveReaderUncompressed::ArchiveReaderUncompressed(plInt32 iDataDirUserData)
  : plDataDirectoryReader(iDataDirUserData)
{
}

plDataDirectory::ArchiveReaderUncompressed::~ArchiveReaderUncompressed() = default;

plUInt64 plDataDirectory::ArchiveReaderUncompressed::Read(void* pBuffer, plUInt64 uiBytes)
{
  return m_MemStreamReader.ReadBytes(pBuffer, uiBytes);
}

plUInt64 plDataDirectory::ArchiveReaderUncompressed::GetFileSize() const
{
  return m_uiUncompressedSize;
}

plResult plDataDirectory::ArchiveReaderUncompressed::InternalOpen(plFileShareMode::Enum FileShareMode)
{
  PLASMA_ASSERT_DEBUG(FileShareMode != plFileShareMode::Exclusive, "Archives only support shared reading of files. Exclusive access cannot be guaranteed.");

  // nothing to do
  return PLASMA_SUCCESS;
}

void plDataDirectory::ArchiveReaderUncompressed::InternalClose()
{
  // nothing to do
}

//////////////////////////////////////////////////////////////////////////

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

plDataDirectory::ArchiveReaderZstd::ArchiveReaderZstd(plInt32 iDataDirUserData)
  : ArchiveReaderUncompressed(iDataDirUserData)
{
}

plDataDirectory::ArchiveReaderZstd::~ArchiveReaderZstd() = default;

plUInt64 plDataDirectory::ArchiveReaderZstd::Read(void* pBuffer, plUInt64 uiBytes)
{
  return m_CompressedStreamReader.ReadBytes(pBuffer, uiBytes);
}

plResult plDataDirectory::ArchiveReaderZstd::InternalOpen(plFileShareMode::Enum FileShareMode)
{
  PLASMA_ASSERT_DEBUG(FileShareMode != plFileShareMode::Exclusive, "Archives only support shared reading of files. Exclusive access cannot be guaranteed.");

  m_CompressedStreamReader.SetInputStream(&m_MemStreamReader);
  return PLASMA_SUCCESS;
}

#endif

//////////////////////////////////////////////////////////////////////////



PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_DataDirTypeArchive);
