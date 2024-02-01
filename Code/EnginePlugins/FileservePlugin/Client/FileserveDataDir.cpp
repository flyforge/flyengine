#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveDataDir.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/Logging/Log.h>

void plDataDirectory::FileserveType::ReloadExternalConfigs()
{
  PL_LOCK(m_RedirectionMutex);
  m_FileRedirection.Clear();

  if (!s_sRedirectionFile.IsEmpty())
  {
    plFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, s_sRedirectionFile, true, nullptr).IgnoreResult();
  }

  FolderType::ReloadExternalConfigs();
}

plDataDirectoryReader* plDataDirectory::FileserveType::OpenFileToRead(plStringView sFile, plFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir)
{
  // fileserve cannot handle absolute paths, which is actually already ruled out at creation time, so this is just an optimization
  if (plPathUtils::IsAbsolutePath(sFile))
    return nullptr;

  plStringBuilder sRedirected;
  if (ResolveAssetRedirection(sFile, sRedirected))
    bSpecificallyThisDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (plConversionUtils::IsStringUuid(sRedirected))
    return nullptr;

  plStringBuilder sFullPath;
  if (plFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bSpecificallyThisDataDir, &sFullPath).Failed())
    return nullptr;

  // It's fine to use the base class here as it will resurface in CreateFolderReader which gives us control of the important part.
  return FolderType::OpenFileToRead(sFullPath, FileShareMode, bSpecificallyThisDataDir);
}

plDataDirectoryWriter* plDataDirectory::FileserveType::OpenFileToWrite(plStringView sFile, plFileShareMode::Enum FileShareMode)
{
  // fileserve cannot handle absolute paths, which is actually already ruled out at creation time, so this is just an optimization
  if (plPathUtils::IsAbsolutePath(sFile))
    return nullptr;

  return FolderType::OpenFileToWrite(sFile, FileShareMode);
}

plResult plDataDirectory::FileserveType::InternalInitializeDataDirectory(plStringView sDirectory)
{
  plStringBuilder sDataDir = sDirectory;
  sDataDir.MakeCleanPath();

  plStringBuilder sCacheFolder, sCacheMetaFolder;
  plFileserveClient::GetSingleton()->GetFullDataDirCachePath(sDataDir, sCacheFolder, sCacheMetaFolder);
  m_sRedirectedDataDirPath = sCacheFolder;
  m_sFileserveCacheMetaFolder = sCacheMetaFolder;

  ReloadExternalConfigs();
  return PL_SUCCESS;
}

void plDataDirectory::FileserveType::RemoveDataDirectory()
{
  if (plFileserveClient::GetSingleton())
  {
    plFileserveClient::GetSingleton()->UnmountDataDirectory(m_uiDataDirID);
  }

  FolderType::RemoveDataDirectory();
}

void plDataDirectory::FileserveType::DeleteFile(plStringView sFile)
{
  if (plFileserveClient::GetSingleton())
  {
    plFileserveClient::GetSingleton()->DeleteFile(m_uiDataDirID, sFile);
  }

  FolderType::DeleteFile(sFile);
}

plDataDirectory::FolderReader* plDataDirectory::FileserveType::CreateFolderReader() const
{
  return PL_DEFAULT_NEW(FileserveDataDirectoryReader, 0);
}

plDataDirectory::FolderWriter* plDataDirectory::FileserveType::CreateFolderWriter() const
{
  return PL_DEFAULT_NEW(FileserveDataDirectoryWriter);
}

plResult plDataDirectory::FileserveType::GetFileStats(plStringView sFileOrFolder, bool bOneSpecificDataDir, plFileStats& out_Stats)
{
  plStringBuilder sRedirected;
  if (ResolveAssetRedirection(sFileOrFolder, sRedirected))
    bOneSpecificDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (plConversionUtils::IsStringUuid(sRedirected))
    return PL_FAILURE;

  plStringBuilder sFullPath;
  PL_SUCCEED_OR_RETURN(plFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bOneSpecificDataDir, &sFullPath));
  return plOSFile::GetFileStats(sFullPath, out_Stats);
}

bool plDataDirectory::FileserveType::ExistsFile(plStringView sFile, bool bOneSpecificDataDir)
{
  plStringBuilder sRedirected;
  if (ResolveAssetRedirection(sFile, sRedirected))
    bOneSpecificDataDir = true; // If this data dir can resolve the guid, only this should load it as well.

  // we know that the server cannot resolve asset GUIDs, so don't even ask
  if (plConversionUtils::IsStringUuid(sRedirected))
    return false;

  return plFileserveClient::GetSingleton()->DownloadFile(m_uiDataDirID, sRedirected, bOneSpecificDataDir, nullptr).Succeeded();
}

plDataDirectoryType* plDataDirectory::FileserveType::Factory(plStringView sDataDirectory, plStringView sGroup, plStringView sRootName, plFileSystem::DataDirUsage usage)
{
  if (!plFileserveClient::s_bEnableFileserve || plFileserveClient::GetSingleton() == nullptr)
    return nullptr; // this would only happen if the functionality is switched off, but not before the factory was added

  // ignore the empty data dir, which handles absolute paths, as we cannot translate these paths to the fileserve host OS
  if (sDataDirectory.IsEmpty())
    return nullptr;

  // Fileserve can only translate paths on the server that start with a 'Special Directory' (e.g. ">sdk/" or ">project/")
  // ignore everything else
  if (!sDataDirectory.StartsWith(">"))
    return nullptr;

  if (plFileserveClient::GetSingleton()->EnsureConnected().Failed())
    return nullptr;

  plDataDirectory::FileserveType* pDataDir = PL_DEFAULT_NEW(plDataDirectory::FileserveType);
  pDataDir->m_uiDataDirID = plFileserveClient::GetSingleton()->MountDataDirectory(sDataDirectory, sRootName);

  if (pDataDir->m_uiDataDirID < 0xffff && pDataDir->InitializeDataDirectory(sDataDirectory) == PL_SUCCESS)
    return pDataDir;

  PL_DEFAULT_DELETE(pDataDir);
  return nullptr;
}

plDataDirectory::FileserveDataDirectoryReader::FileserveDataDirectoryReader(plInt32 iDataDirUserData)
  : FolderReader(iDataDirUserData)
{
}

plResult plDataDirectory::FileserveDataDirectoryReader::InternalOpen(plFileShareMode::Enum FileShareMode)
{
  return m_File.Open(GetFilePath().GetData(), plFileOpenMode::Read, FileShareMode);
}

void plDataDirectory::FileserveDataDirectoryWriter::InternalClose()
{
  FolderWriter::InternalClose();

  static_cast<FileserveType*>(GetDataDirectory())->FinishedWriting(this);
}

void plDataDirectory::FileserveType::FinishedWriting(FolderWriter* pWriter)
{
  if (plFileserveClient::GetSingleton() == nullptr)
    return;

  plStringBuilder sAbsPath = pWriter->GetDataDirectory()->GetRedirectedDataDirectoryPath();
  sAbsPath.AppendPath(pWriter->GetFilePath());

  plOSFile file;
  if (file.Open(sAbsPath, plFileOpenMode::Read).Failed())
  {
    plLog::Error("Could not read file for upload: '{0}'", sAbsPath);
    return;
  }

  plDynamicArray<plUInt8> content;
  file.ReadAll(content);
  file.Close();

  plFileserveClient::GetSingleton()->UploadFile(m_uiDataDirID, pWriter->GetFilePath(), content);
}



PL_STATICLINK_FILE(FileservePlugin, FileservePlugin_Client_FileserveDataDir);
