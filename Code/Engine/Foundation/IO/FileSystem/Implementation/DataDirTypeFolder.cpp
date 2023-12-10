#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/Log.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FolderDataDirectory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "FileSystem"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plFileSystem::RegisterDataDirectoryFactory(plDataDirectory::FolderType::Factory);
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace plDataDirectory
{
  plString FolderType::s_sRedirectionFile;
  plString FolderType::s_sRedirectionPrefix;

  plResult FolderReader::InternalOpen(plFileShareMode::Enum FileShareMode)
  {
    plStringBuilder sPath = ((plDataDirectory::FolderType*)GetDataDirectory())->GetRedirectedDataDirectoryPath();
    sPath.AppendPath(GetFilePath());

    return m_File.Open(sPath.GetData(), plFileOpenMode::Read, FileShareMode);
  }

  void FolderReader::InternalClose()
  {
    m_File.Close();
  }

  plUInt64 FolderReader::Read(void* pBuffer, plUInt64 uiBytes)
  {
    return m_File.Read(pBuffer, uiBytes);
  }

  plUInt64 FolderReader::GetFileSize() const
  {
    return m_File.GetFileSize();
  }

  plResult FolderWriter::InternalOpen(plFileShareMode::Enum FileShareMode)
  {
    plStringBuilder sPath = ((plDataDirectory::FolderType*)GetDataDirectory())->GetRedirectedDataDirectoryPath();
    sPath.AppendPath(GetFilePath());

    return m_File.Open(sPath.GetData(), plFileOpenMode::Write, FileShareMode);
  }

  void FolderWriter::InternalClose()
  {
    m_File.Close();
  }

  plResult FolderWriter::Write(const void* pBuffer, plUInt64 uiBytes)
  {
    return m_File.Write(pBuffer, uiBytes);
  }

  plUInt64 FolderWriter::GetFileSize() const
  {
    return m_File.GetFileSize();
  }

  plDataDirectoryType* FolderType::Factory(plStringView sDataDirectory, plStringView sGroup, plStringView sRootName, plFileSystem::DataDirUsage usage)
  {
    FolderType* pDataDir = PLASMA_DEFAULT_NEW(FolderType);

    if (pDataDir->InitializeDataDirectory(sDataDirectory) == PLASMA_SUCCESS)
      return pDataDir;

    PLASMA_DEFAULT_DELETE(pDataDir);
    return nullptr;
  }

  void FolderType::RemoveDataDirectory()
  {
    {
      PLASMA_LOCK(m_ReaderWriterMutex);
      for (plUInt32 i = 0; i < m_Readers.GetCount(); ++i)
      {
        PLASMA_ASSERT_DEV(!m_Readers[i]->m_bIsInUse, "Cannot remove a data directory while there are still files open in it.");
      }

      for (plUInt32 i = 0; i < m_Writers.GetCount(); ++i)
      {
        PLASMA_ASSERT_DEV(!m_Writers[i]->m_bIsInUse, "Cannot remove a data directory while there are still files open in it.");
      }
    }
    FolderType* pThis = this;
    PLASMA_DEFAULT_DELETE(pThis);
  }

  void FolderType::DeleteFile(plStringView sFile)
  {
    plStringBuilder sPath = GetRedirectedDataDirectoryPath();
    sPath.AppendPath(sFile);

    plOSFile::DeleteFile(sPath.GetData()).IgnoreResult();
  }

  FolderType::~FolderType()
  {
    PLASMA_LOCK(m_ReaderWriterMutex);
    for (plUInt32 i = 0; i < m_Readers.GetCount(); ++i)
      PLASMA_DEFAULT_DELETE(m_Readers[i]);

    for (plUInt32 i = 0; i < m_Writers.GetCount(); ++i)
      PLASMA_DEFAULT_DELETE(m_Writers[i]);
  }

  void FolderType::ReloadExternalConfigs()
  {
    LoadRedirectionFile();
  }

  void FolderType::LoadRedirectionFile()
  {
    PLASMA_LOCK(m_RedirectionMutex);
    m_FileRedirection.Clear();

    if (!s_sRedirectionFile.IsEmpty())
    {
      plStringBuilder sRedirectionFile(GetRedirectedDataDirectoryPath(), "/", s_sRedirectionFile);
      sRedirectionFile.MakeCleanPath();

      PLASMA_LOG_BLOCK("LoadRedirectionFile", sRedirectionFile.GetData());

      plOSFile file;
      if (file.Open(sRedirectionFile, plFileOpenMode::Read).Succeeded())
      {
        plHybridArray<char, 1024 * 10> content;
        char uiTemp[4096];

        plUInt64 uiRead = 0;

        do
        {
          uiRead = file.Read(uiTemp, PLASMA_ARRAY_SIZE(uiTemp));
          content.PushBackRange(plArrayPtr<char>(uiTemp, (plUInt32)uiRead));
        } while (uiRead == PLASMA_ARRAY_SIZE(uiTemp));

        content.PushBack(0); // make sure the string is terminated

        const char* szLineStart = content.GetData();
        const char* szSeparator = nullptr;
        const char* szLineEnd = nullptr;

        plStringBuilder sFileToRedirect, sRedirection;

        while (true)
        {
          szSeparator = plStringUtils::FindSubString(szLineStart, ";");
          szLineEnd = plStringUtils::FindSubString(szSeparator, "\n");

          if (szLineStart == nullptr || szSeparator == nullptr || szLineEnd == nullptr)
            break;

          sFileToRedirect.SetSubString_FromTo(szLineStart, szSeparator);
          sRedirection.SetSubString_FromTo(szSeparator + 1, szLineEnd);

          m_FileRedirection[sFileToRedirect] = sRedirection;

          szLineStart = szLineEnd + 1;
        }

        // plLog::Debug("Redirection file contains {0} entries", m_FileRedirection.GetCount());
      }
      // else
      // plLog::Debug("No Redirection file found in: '{0}'", sRedirectionFile);
    }
  }


  bool FolderType::ExistsFile(plStringView sFile, bool bOneSpecificDataDir)
  {
    plStringBuilder sRedirectedAsset;
    ResolveAssetRedirection(sFile, sRedirectedAsset);

    plStringBuilder sPath = GetRedirectedDataDirectoryPath();
    sPath.AppendPath(sRedirectedAsset);
    return plOSFile::ExistsFile(sPath);
  }

  plResult FolderType::GetFileStats(plStringView sFileOrFolder, bool bOneSpecificDataDir, plFileStats& out_Stats)
  {
    plStringBuilder sRedirectedAsset;
    ResolveAssetRedirection(sFileOrFolder, sRedirectedAsset);

    plStringBuilder sPath = GetRedirectedDataDirectoryPath();

    if (plPathUtils::IsAbsolutePath(sRedirectedAsset))
    {
      if (!sRedirectedAsset.StartsWith_NoCase(sPath))
        return PLASMA_FAILURE;

      sPath.Clear();
    }

    sPath.AppendPath(sRedirectedAsset);

    if (!plPathUtils::IsAbsolutePath(sPath))
      return PLASMA_FAILURE;

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)
    return plOSFile::GetFileStats(sPath, out_Stats);
#else
    return PLASMA_FAILURE;
#endif
  }

  plResult FolderType::InternalInitializeDataDirectory(plStringView sDirectory)
  {
    // allow to set the 'empty' directory to handle all absolute paths
    if (sDirectory.IsEmpty())
      return PLASMA_SUCCESS;

    plStringBuilder sRedirected;
    if (plFileSystem::ResolveSpecialDirectory(sDirectory, sRedirected).Succeeded())
    {
      m_sRedirectedDataDirPath = sRedirected;
    }
    else
    {
      m_sRedirectedDataDirPath = sDirectory;
    }

    if (!plOSFile::ExistsDirectory(m_sRedirectedDataDirPath))
      return PLASMA_FAILURE;

    ReloadExternalConfigs();

    return PLASMA_SUCCESS;
  }

  void FolderType::OnReaderWriterClose(plDataDirectoryReaderWriterBase* pClosed)
  {
    PLASMA_LOCK(m_ReaderWriterMutex);
    if (pClosed->IsReader())
    {
      FolderReader* pReader = (FolderReader*)pClosed;
      pReader->m_bIsInUse = false;
    }
    else
    {
      FolderWriter* pWriter = (FolderWriter*)pClosed;
      pWriter->m_bIsInUse = false;
    }
  }

  plDataDirectory::FolderReader* FolderType::CreateFolderReader() const
  {
    return PLASMA_DEFAULT_NEW(FolderReader, 0);
  }

  plDataDirectory::FolderWriter* FolderType::CreateFolderWriter() const
  {
    return PLASMA_DEFAULT_NEW(FolderWriter, 0);
  }

  plDataDirectoryReader* FolderType::OpenFileToRead(plStringView sFile, plFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir)
  {
    plStringBuilder sFileToOpen;
    ResolveAssetRedirection(sFile, sFileToOpen);

    // we know that these files cannot be opened, so don't even try
    if (plConversionUtils::IsStringUuid(sFileToOpen))
      return nullptr;

    FolderReader* pReader = nullptr;
    {
      PLASMA_LOCK(m_ReaderWriterMutex);
      for (plUInt32 i = 0; i < m_Readers.GetCount(); ++i)
      {
        if (!m_Readers[i]->m_bIsInUse)
          pReader = m_Readers[i];
      }

      if (pReader == nullptr)
      {
        m_Readers.PushBack(CreateFolderReader());
        pReader = m_Readers.PeekBack();
      }
      pReader->m_bIsInUse = true;
    }

    // if opening the file fails, the reader's m_bIsInUse needs to be reset.
    if (pReader->Open(sFileToOpen, this, FileShareMode) == PLASMA_FAILURE)
    {
      PLASMA_LOCK(m_ReaderWriterMutex);
      pReader->m_bIsInUse = false;
      return nullptr;
    }

    // if it succeeds, we return the reader
    return pReader;
  }


  bool FolderType::ResolveAssetRedirection(plStringView sFile, plStringBuilder& out_sRedirection)
  {
    PLASMA_LOCK(m_RedirectionMutex);
    // Check if we know about a file redirection for this
    auto it = m_FileRedirection.Find(sFile);

    // if available, open the file that is mentioned in the redirection file instead
    if (it.IsValid())
    {

      if (it.Value().StartsWith("?"))
      {
        // ? is an option to tell the system to skip the redirection prefix and use the path as is
        out_sRedirection = &it.Value().GetData()[1];
      }
      else
      {
        out_sRedirection.Set(s_sRedirectionPrefix, it.Value());
      }
      return true;
    }
    else
    {
      out_sRedirection = sFile;
      return false;
    }
  }

  plDataDirectoryWriter* FolderType::OpenFileToWrite(plStringView sFile, plFileShareMode::Enum FileShareMode)
  {
    FolderWriter* pWriter = nullptr;

    {
      PLASMA_LOCK(m_ReaderWriterMutex);
      for (plUInt32 i = 0; i < m_Writers.GetCount(); ++i)
      {
        if (!m_Writers[i]->m_bIsInUse)
          pWriter = m_Writers[i];
      }

      if (pWriter == nullptr)
      {
        m_Writers.PushBack(CreateFolderWriter());
        pWriter = m_Writers.PeekBack();
      }
      pWriter->m_bIsInUse = true;
    }
    // if opening the file fails, the writer's m_bIsInUse needs to be reset.
    if (pWriter->Open(sFile, this, FileShareMode) == PLASMA_FAILURE)
    {
      PLASMA_LOCK(m_ReaderWriterMutex);
      pWriter->m_bIsInUse = false;
      return nullptr;
    }

    // if it succeeds, we return the reader
    return pWriter;
  }
} // namespace plDataDirectory



PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_DataDirTypeFolder);
