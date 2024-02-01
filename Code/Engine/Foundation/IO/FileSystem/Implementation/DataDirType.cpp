#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

plResult plDataDirectoryType::InitializeDataDirectory(plStringView sDataDirPath)
{
  plStringBuilder sPath = sDataDirPath;
  sPath.MakeCleanPath();

  PL_ASSERT_DEV(sPath.IsEmpty() || sPath.EndsWith("/"), "Data directory path must end with a slash.");

  m_sDataDirectoryPath = sPath;

  return InternalInitializeDataDirectory(m_sDataDirectoryPath.GetData());
}

bool plDataDirectoryType::ExistsFile(plStringView sFile, bool bOneSpecificDataDir)
{
  plStringBuilder sRedirectedAsset;
  ResolveAssetRedirection(sFile, sRedirectedAsset);

  plStringBuilder sPath = GetRedirectedDataDirectoryPath();
  sPath.AppendPath(sRedirectedAsset);
  return plOSFile::ExistsFile(sPath);
}

void plDataDirectoryReaderWriterBase::Close()
{
  InternalClose();

  plFileSystem::FileEvent fe;
  fe.m_EventType = plFileSystem::FileEventType::CloseFile;
  fe.m_sFileOrDirectory = GetFilePath();
  fe.m_pDataDir = m_pDataDirectory;
  plFileSystem::s_pData->m_Event.Broadcast(fe);

  m_pDataDirectory->OnReaderWriterClose(this);
}


