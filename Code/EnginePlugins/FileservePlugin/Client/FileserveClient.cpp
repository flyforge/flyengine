#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveClient.h>
#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>

PLASMA_IMPLEMENT_SINGLETON(plFileserveClient);

bool plFileserveClient::s_bEnableFileserve = true;

plFileserveClient::plFileserveClient()
  : m_SingletonRegistrar(this)
{
  AddServerAddressToTry("localhost:1042");

  plStringBuilder sAddress, sSearch;

  // the app directory
  {
    sSearch = plOSFile::GetApplicationDirectory();
    sSearch.AppendPath("plFileserve.txt");

    if (TryReadFileserveConfig(sSearch, sAddress).Succeeded())
    {
      AddServerAddressToTry(sAddress);
    }
  }

  plStringBuilder tmp;
  // command line argument
  AddServerAddressToTry(plCommandLineUtils::GetGlobalInstance()->GetStringOption("-fs_server", 0, "").GetData(tmp));

  // last successful IP is stored in the user directory
  {
    sSearch = plOSFile::GetUserDataFolder("plFileserve.txt");

    if (TryReadFileserveConfig(sSearch, sAddress).Succeeded())
    {
      AddServerAddressToTry(sAddress);
    }
  }

  if (plCommandLineUtils::GetGlobalInstance()->GetBoolOption("-fs_off"))
    s_bEnableFileserve = false;

  m_CurrentTime = plTime::Now();
}

plFileserveClient::~plFileserveClient()
{
  ShutdownConnection();
}

void plFileserveClient::ShutdownConnection()
{
  if (m_pNetwork)
  {
    plLog::Dev("Shutting down fileserve client");

    m_pNetwork->ShutdownConnection();
    m_pNetwork = nullptr;
  }
}

void plFileserveClient::ClearState()
{
  m_bDownloading = false;
  m_bWaitingForUploadFinished = false;
  m_CurFileRequestGuid = plUuid();
  m_sCurFileRequest.Clear();
  m_Download.Clear();
}

plResult plFileserveClient::EnsureConnected(plTime timeout)
{
  PLASMA_LOCK(m_Mutex);
  if (!s_bEnableFileserve || m_bFailedToConnect)
    return PLASMA_FAILURE;

  if (m_pNetwork == nullptr)
  {
    m_pNetwork = plRemoteInterfaceEnet::Make(); /// \todo Somehow abstract this away ?

    m_sFileserveCacheFolder = plOSFile::GetUserDataFolder("plFileserve/Cache");
    m_sFileserveCacheMetaFolder = plOSFile::GetUserDataFolder("plFileserve/Meta");

    if (plOSFile::CreateDirectoryStructure(m_sFileserveCacheFolder).Failed())
    {
      plLog::Error("Could not create fileserve cache folder '{0}'", m_sFileserveCacheFolder);
      return PLASMA_FAILURE;
    }

    if (plOSFile::CreateDirectoryStructure(m_sFileserveCacheMetaFolder).Failed())
    {
      plLog::Error("Could not create fileserve cache folder '{0}'", m_sFileserveCacheMetaFolder);
      return PLASMA_FAILURE;
    }
  }

  if (!m_pNetwork->IsConnectedToServer())
  {
    ClearState();
    m_bFailedToConnect = true;

    if (m_pNetwork->ConnectToServer('PLFS', m_sServerConnectionAddress).Failed())
      return PLASMA_FAILURE;

    if (timeout.GetSeconds() < 0)
    {
      timeout = plTime::Seconds(plCommandLineUtils::GetGlobalInstance()->GetFloatOption("-fs_timeout", -timeout.GetSeconds()));
    }

    if (m_pNetwork->WaitForConnectionToServer(timeout).Failed())
    {
      m_pNetwork->ShutdownConnection();
      plLog::Error("Connection to plFileserver timed out");
      return PLASMA_FAILURE;
    }
    else
    {
      plLog::Success("Connected to plFileserver '{0}", m_sServerConnectionAddress);
      m_pNetwork->SetMessageHandler('FSRV', plMakeDelegate(&plFileserveClient::NetworkMsgHandler, this));

      m_pNetwork->Send('FSRV', 'HELO'); // be friendly
    }

    m_bFailedToConnect = false;
  }

  return PLASMA_SUCCESS;
}

void plFileserveClient::UpdateClient()
{
  PLASMA_LOCK(m_Mutex);
  if (m_pNetwork == nullptr || m_bFailedToConnect || !s_bEnableFileserve)
    return;

  if (!m_pNetwork->IsConnectedToServer())
  {
    if (EnsureConnected().Failed())
    {
      plLog::Error("Fileserve connection was lost and could not be re-established.");
      ShutdownConnection();
    }
    return;
  }

  m_CurrentTime = plTime::Now();

  m_pNetwork->ExecuteAllMessageHandlers();
}

void plFileserveClient::AddServerAddressToTry(const char* szAddress)
{
  PLASMA_LOCK(m_Mutex);
  if (plStringUtils::IsNullOrEmpty(szAddress))
    return;

  if (m_TryServerAddresses.Contains(szAddress))
    return;

  m_TryServerAddresses.PushBack(szAddress);

  // always set the most recent address as the default one
  m_sServerConnectionAddress = szAddress;
}

void plFileserveClient::UploadFile(plUInt16 uiDataDirID, const char* szFile, const plDynamicArray<plUInt8>& fileContent)
{
  PLASMA_LOCK(m_Mutex);

  if (m_pNetwork == nullptr)
    return;

  // update meta state and cache
  {
    const plString& sMountPoint = m_MountedDataDirs[uiDataDirID].m_sMountPoint;
    plStringBuilder sCachedMetaFile;
    BuildPathInCache(szFile, sMountPoint, nullptr, &sCachedMetaFile);

    plUInt64 uiHash = 1;

    if (!fileContent.IsEmpty())
    {
      uiHash = plHashingUtils::xxHash64(fileContent.GetData(), fileContent.GetCount(), uiHash);
    }

    WriteMetaFile(sCachedMetaFile, 0, uiHash);

    InvalidateFileCache(uiDataDirID, szFile, uiHash);
  }

  const plUInt32 uiFileSize = fileContent.GetCount();

  plUuid uploadGuid;
  uploadGuid.CreateNewUuid();

  {
    plRemoteMessage msg;
    msg.SetMessageID('FSRV', 'UPLH');
    msg.GetWriter() << uploadGuid;
    msg.GetWriter() << uiFileSize;
    msg.GetWriter() << uiDataDirID;
    msg.GetWriter() << szFile;
    m_pNetwork->Send(plRemoteTransmitMode::Reliable, msg);
  }

  plUInt32 uiNextByte = 0;

  // send the file over in multiple packages of 1KB each
  // send at least one package, even for empty files

  while (uiNextByte < fileContent.GetCount())
  {
    const plUInt16 uiChunkSize = (plUInt16)plMath::Min<plUInt32>(1024, fileContent.GetCount() - uiNextByte);

    plRemoteMessage msg;
    msg.GetWriter() << uploadGuid;
    msg.GetWriter() << uiChunkSize;
    msg.GetWriter().WriteBytes(&fileContent[uiNextByte], uiChunkSize).IgnoreResult();

    msg.SetMessageID('FSRV', 'UPLD');
    m_pNetwork->Send(plRemoteTransmitMode::Reliable, msg);

    uiNextByte += uiChunkSize;
  }

  // continuously update the network until we know the server has received the big chunk of data
  m_bWaitingForUploadFinished = true;

  // final message to server
  {
    plRemoteMessage msg('FSRV', 'UPLF');
    msg.GetWriter() << uploadGuid;
    msg.GetWriter() << uiDataDirID;
    msg.GetWriter() << szFile;

    m_pNetwork->Send(plRemoteTransmitMode::Reliable, msg);
  }

  while (m_bWaitingForUploadFinished)
  {
    UpdateClient();
  }
}


void plFileserveClient::InvalidateFileCache(plUInt16 uiDataDirID, plStringView sFile, plUInt64 uiHash)
{
  PLASMA_LOCK(m_Mutex);
  auto& cache = m_MountedDataDirs[uiDataDirID].m_CacheStatus[sFile];
  cache.m_FileHash = uiHash;
  cache.m_TimeStamp = 0;
  cache.m_LastCheck.SetZero(); // will trigger a server request and that in turn will update the file timestamp

  // redirect the next access to this cache entry
  // together with the zero LastCheck that will make sure the best match gets updated as well
  m_FileDataDir[sFile] = uiDataDirID;
}

void plFileserveClient::FillFileStatusCache(const char* szFile)
{
  PLASMA_LOCK(m_Mutex);
  auto it = m_FileDataDir.FindOrAdd(szFile);
  it.Value() = 0xffff; // does not exist

  for (plUInt16 i = static_cast<plUInt16>(m_MountedDataDirs.GetCount()); i > 0; --i)
  {
    const plUInt16 dd = i - 1;

    if (!m_MountedDataDirs[dd].m_bMounted)
      continue;

    auto& cache = m_MountedDataDirs[dd].m_CacheStatus[szFile];

    DetermineCacheStatus(dd, szFile, cache);
    cache.m_LastCheck.SetZero();

    if (cache.m_TimeStamp != 0 && cache.m_FileHash != 0) // file exists
    {
      // best possible candidate
      if (it.Value() == 0xffff)
        it.Value() = dd;
    }
  }

  if (it.Value() == 0xffff)
    it.Value() = 0; // fallback
}

void plFileserveClient::BuildPathInCache(const char* szFile, const char* szMountPoint, plStringBuilder* out_pAbsPath, plStringBuilder* out_pFullPathMeta) const
{
  PLASMA_ASSERT_DEV(!plPathUtils::IsAbsolutePath(szFile), "Invalid path");
  PLASMA_LOCK(m_Mutex);
  if (out_pAbsPath)
  {
    *out_pAbsPath = m_sFileserveCacheFolder;
    out_pAbsPath->AppendPath(szMountPoint, szFile);
    out_pAbsPath->MakeCleanPath();
  }
  if (out_pFullPathMeta)
  {
    *out_pFullPathMeta = m_sFileserveCacheMetaFolder;
    out_pFullPathMeta->AppendPath(szMountPoint, szFile);
    out_pFullPathMeta->MakeCleanPath();
  }
}

void plFileserveClient::ComputeDataDirMountPoint(plStringView sDataDir, plStringBuilder& out_sMountPoint)
{
  PLASMA_ASSERT_DEV(sDataDir.IsEmpty() || sDataDir.EndsWith("/"), "Invalid path");

  const plUInt32 uiMountPoint = plHashingUtils::xxHash32String(sDataDir);
  out_sMountPoint.Format("{0}", plArgU(uiMountPoint, 8, true, 16));
}

void plFileserveClient::GetFullDataDirCachePath(const char* szDataDir, plStringBuilder& out_sFullPath, plStringBuilder& out_sFullPathMeta) const
{
  PLASMA_LOCK(m_Mutex);
  plStringBuilder sMountPoint;
  ComputeDataDirMountPoint(szDataDir, sMountPoint);

  out_sFullPath = m_sFileserveCacheFolder;
  out_sFullPath.AppendPath(sMountPoint);

  out_sFullPathMeta = m_sFileserveCacheMetaFolder;
  out_sFullPathMeta.AppendPath(sMountPoint);
}

void plFileserveClient::NetworkMsgHandler(plRemoteMessage& msg)
{
  PLASMA_LOCK(m_Mutex);
  if (msg.GetMessageID() == 'DWNL')
  {
    HandleFileTransferMsg(msg);
    return;
  }

  if (msg.GetMessageID() == 'DWNF')
  {
    HandleFileTransferFinishedMsg(msg);
    return;
  }

  static bool s_bReloadResources = false;

  if (msg.GetMessageID() == 'RLDR')
  {
    s_bReloadResources = true;
  }

  if (!m_bDownloading && s_bReloadResources)
  {
    PLASMA_BROADCAST_EVENT(plResourceManager_ReloadAllResources);
    s_bReloadResources = false;
    return;
  }

  if (msg.GetMessageID() == 'RLDR')
    return;

  if (msg.GetMessageID() == 'UACK')
  {
    m_bWaitingForUploadFinished = false;
    return;
  }

  plLog::Error("Unknown FSRV message: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageData().GetCount());
}

plUInt16 plFileserveClient::MountDataDirectory(plStringView sDataDirectory, plStringView sRootName)
{
  PLASMA_LOCK(m_Mutex);
  if (!m_pNetwork->IsConnectedToServer())
    return 0xffff;

  plStringBuilder sRoot = sRootName;
  sRoot.Trim(":/");

  plStringBuilder sMountPoint;
  ComputeDataDirMountPoint(sDataDirectory, sMountPoint);

  const plUInt16 uiDataDirID = static_cast<plUInt16>(m_MountedDataDirs.GetCount());

  plRemoteMessage msg('FSRV', ' MNT');
  msg.GetWriter() << sDataDirectory;
  msg.GetWriter() << sRoot;
  msg.GetWriter() << sMountPoint;
  msg.GetWriter() << uiDataDirID;

  m_pNetwork->Send(plRemoteTransmitMode::Reliable, msg);

  auto& dd = m_MountedDataDirs.ExpandAndGetRef();
  // dd.m_sPathOnClient = sDataDirectory;
  // dd.m_sRootName = sRoot;
  dd.m_sMountPoint = sMountPoint;
  dd.m_bMounted = true;

  return uiDataDirID;
}


void plFileserveClient::UnmountDataDirectory(plUInt16 uiDataDir)
{
  PLASMA_LOCK(m_Mutex);
  if (!m_pNetwork->IsConnectedToServer())
    return;

  plRemoteMessage msg('FSRV', 'UMNT');
  msg.GetWriter() << uiDataDir;

  m_pNetwork->Send(plRemoteTransmitMode::Reliable, msg);

  auto& dd = m_MountedDataDirs[uiDataDir];
  dd.m_bMounted = false;
}

void plFileserveClient::DeleteFile(plUInt16 uiDataDir, plStringView sFile)
{
  PLASMA_LOCK(m_Mutex);
  if (!m_pNetwork->IsConnectedToServer())
    return;

  InvalidateFileCache(uiDataDir, sFile, 0);

  plRemoteMessage msg('FSRV', 'DELF');
  msg.GetWriter() << uiDataDir;
  msg.GetWriter() << sFile;

  m_pNetwork->Send(plRemoteTransmitMode::Reliable, msg);
}

void plFileserveClient::HandleFileTransferMsg(plRemoteMessage& msg)
{
  PLASMA_LOCK(m_Mutex);
  {
    plUuid fileRequestGuid;
    msg.GetReader() >> fileRequestGuid;

    if (fileRequestGuid != m_CurFileRequestGuid)
    {
      // plLog::Debug("Fileserver is answering someone else");
      return;
    }
  }

  plUInt16 uiChunkSize = 0;
  msg.GetReader() >> uiChunkSize;

  plUInt32 uiFileSize = 0;
  msg.GetReader() >> uiFileSize;

  // make sure we don't need to reallocate
  m_Download.Reserve(uiFileSize);

  if (uiChunkSize > 0)
  {
    const plUInt32 uiStartPos = m_Download.GetCount();
    m_Download.SetCountUninitialized(uiStartPos + uiChunkSize);
    msg.GetReader().ReadBytes(&m_Download[uiStartPos], uiChunkSize);
  }
}


void plFileserveClient::HandleFileTransferFinishedMsg(plRemoteMessage& msg)
{
  PLASMA_LOCK(m_Mutex);
  PLASMA_SCOPE_EXIT(m_bDownloading = false);

  {
    plUuid fileRequestGuid;
    msg.GetReader() >> fileRequestGuid;

    if (fileRequestGuid != m_CurFileRequestGuid)
    {
      // plLog::Debug("Fileserver is answering someone else");
      return;
    }
  }

  plFileserveFileState fileState;
  {
    plInt8 iFileStatus = 0;
    msg.GetReader() >> iFileStatus;
    fileState = (plFileserveFileState)iFileStatus;
  }

  plInt64 iFileTimeStamp = 0;
  msg.GetReader() >> iFileTimeStamp;

  plUInt64 uiFileHash = 0;
  msg.GetReader() >> uiFileHash;

  plUInt16 uiFoundInDataDir = 0;
  msg.GetReader() >> uiFoundInDataDir;

  if (uiFoundInDataDir == 0xffff) // file does not exist on server in any data dir
  {
    m_FileDataDir[m_sCurFileRequest] = 0; // placeholder

    for (plUInt32 i = 0; i < m_MountedDataDirs.GetCount(); ++i)
    {
      auto& ref = m_MountedDataDirs[i].m_CacheStatus[m_sCurFileRequest];
      ref.m_FileHash = 0;
      ref.m_TimeStamp = 0;
      ref.m_LastCheck = m_CurrentTime;
    }

    return;
  }
  else
  {
    m_FileDataDir[m_sCurFileRequest] = uiFoundInDataDir;

    auto& ref = m_MountedDataDirs[uiFoundInDataDir].m_CacheStatus[m_sCurFileRequest];
    ref.m_FileHash = uiFileHash;
    ref.m_TimeStamp = iFileTimeStamp;
    ref.m_LastCheck = m_CurrentTime;
  }

  // nothing changed
  if (fileState == plFileserveFileState::SameTimestamp || fileState == plFileserveFileState::NonExistantEither)
    return;

  const plString& sMountPoint = m_MountedDataDirs[uiFoundInDataDir].m_sMountPoint;
  plStringBuilder sCachedFile, sCachedMetaFile;
  BuildPathInCache(m_sCurFileRequest, sMountPoint, &sCachedFile, &sCachedMetaFile);

  if (fileState == plFileserveFileState::NonExistant)
  {
    // remove them from the cache as well, if they still exist there
    plOSFile::DeleteFile(sCachedFile).IgnoreResult();
    plOSFile::DeleteFile(sCachedMetaFile).IgnoreResult();
    return;
  }

  // timestamp changed, but hash is still the same -> update timestamp
  if (fileState == plFileserveFileState::SameHash)
  {
    WriteMetaFile(sCachedMetaFile, iFileTimeStamp, uiFileHash);
  }

  if (fileState == plFileserveFileState::Different)
  {
    WriteDownloadToDisk(sCachedFile);
    WriteMetaFile(sCachedMetaFile, iFileTimeStamp, uiFileHash);
  }
}


void plFileserveClient::WriteMetaFile(plStringBuilder sCachedMetaFile, plInt64 iFileTimeStamp, plUInt64 uiFileHash)
{
  plOSFile file;
  if (file.Open(sCachedMetaFile, plFileOpenMode::Write).Succeeded())
  {
    file.Write(&iFileTimeStamp, sizeof(plInt64)).IgnoreResult();
    file.Write(&uiFileHash, sizeof(plUInt64)).IgnoreResult();

    file.Close();
  }
  else
  {
    plLog::Error("Failed to write meta file to '{0}'", sCachedMetaFile);
  }
}

void plFileserveClient::WriteDownloadToDisk(plStringBuilder sCachedFile)
{
  PLASMA_LOCK(m_Mutex);
  plOSFile file;
  if (file.Open(sCachedFile, plFileOpenMode::Write).Succeeded())
  {
    if (!m_Download.IsEmpty())
      file.Write(m_Download.GetData(), m_Download.GetCount()).IgnoreResult();

    file.Close();
  }
  else
  {
    plLog::Error("Failed to write download to '{0}'", sCachedFile);
  }
}

plResult plFileserveClient::DownloadFile(plUInt16 uiDataDirID, const char* szFile, bool bForceThisDataDir, plStringBuilder* out_pFullPath)
{
  // bForceThisDataDir = true;
  PLASMA_LOCK(m_Mutex);
  if (m_bDownloading)
  {
    plLog::Warning("Trying to download a file over fileserve while another file is already downloading. Recursive download is ignored.");
    return PLASMA_FAILURE;
  }

  PLASMA_ASSERT_DEV(uiDataDirID < m_MountedDataDirs.GetCount(), "Invalid data dir index {0}", uiDataDirID);
  PLASMA_ASSERT_DEV(m_MountedDataDirs[uiDataDirID].m_bMounted, "Data directory {0} is not mounted", uiDataDirID);
  PLASMA_ASSERT_DEV(!m_bDownloading, "Cannot start a download, while one is still running");

  if (!m_pNetwork->IsConnectedToServer())
    return PLASMA_FAILURE;

  bool bCachedYet = false;
  auto itFileDataDir = m_FileDataDir.FindOrAdd(szFile, &bCachedYet);
  if (!bCachedYet)
  {
    FillFileStatusCache(szFile);
  }

  const plUInt16 uiUseDataDirCache = bForceThisDataDir ? uiDataDirID : itFileDataDir.Value();
  const FileCacheStatus& CacheStatus = m_MountedDataDirs[uiUseDataDirCache].m_CacheStatus[szFile];

  if (m_CurrentTime - CacheStatus.m_LastCheck < plTime::Seconds(5.0f))
  {
    if (CacheStatus.m_FileHash == 0) // file does not exist
      return PLASMA_FAILURE;

    if (out_pFullPath)
      BuildPathInCache(szFile, m_MountedDataDirs[uiUseDataDirCache].m_sMountPoint, out_pFullPath, nullptr);

    return PLASMA_SUCCESS;
  }

  m_Download.Clear();
  m_sCurFileRequest = szFile;
  m_CurFileRequestGuid.CreateNewUuid();
  m_bDownloading = true;

  plRemoteMessage msg('FSRV', 'READ');
  msg.GetWriter() << uiUseDataDirCache;
  msg.GetWriter() << bForceThisDataDir;
  msg.GetWriter() << szFile;
  msg.GetWriter() << m_CurFileRequestGuid;
  msg.GetWriter() << CacheStatus.m_TimeStamp;
  msg.GetWriter() << CacheStatus.m_FileHash;

  m_pNetwork->Send(plRemoteTransmitMode::Reliable, msg);

  while (m_bDownloading)
  {
    m_pNetwork->UpdateRemoteInterface();
    m_pNetwork->ExecuteAllMessageHandlers();
  }

  if (bForceThisDataDir)
  {
    if (m_MountedDataDirs[uiDataDirID].m_CacheStatus[m_sCurFileRequest].m_FileHash == 0)
      return PLASMA_FAILURE;

    if (out_pFullPath)
      BuildPathInCache(szFile, m_MountedDataDirs[uiDataDirID].m_sMountPoint, out_pFullPath, nullptr);

    return PLASMA_SUCCESS;
  }
  else
  {
    const plUInt16 uiBestDir = itFileDataDir.Value();
    if (uiBestDir == uiDataDirID) // best match is still this? -> success
    {
      // file does not exist
      if (m_MountedDataDirs[uiBestDir].m_CacheStatus[m_sCurFileRequest].m_FileHash == 0)
        return PLASMA_FAILURE;

      if (out_pFullPath)
        BuildPathInCache(szFile, m_MountedDataDirs[uiBestDir].m_sMountPoint, out_pFullPath, nullptr);

      return PLASMA_SUCCESS;
    }

    return PLASMA_FAILURE;
  }
}

void plFileserveClient::DetermineCacheStatus(plUInt16 uiDataDirID, const char* szFile, FileCacheStatus& out_Status) const
{
  PLASMA_LOCK(m_Mutex);
  plStringBuilder sAbsPathFile, sAbsPathMeta;
  const auto& dd = m_MountedDataDirs[uiDataDirID];

  PLASMA_ASSERT_DEV(dd.m_bMounted, "Data directory {0} is not mounted", uiDataDirID);

  BuildPathInCache(szFile, dd.m_sMountPoint, &sAbsPathFile, &sAbsPathMeta);

  if (plOSFile::ExistsFile(sAbsPathFile))
  {
    plOSFile meta;
    if (meta.Open(sAbsPathMeta, plFileOpenMode::Read).Failed())
    {
      // cleanup, when the meta file does not exist, the data file is useless
      plOSFile::DeleteFile(sAbsPathFile).IgnoreResult();
      return;
    }

    meta.Read(&out_Status.m_TimeStamp, sizeof(plInt64));
    meta.Read(&out_Status.m_FileHash, sizeof(plUInt64));
  }
}

plResult plFileserveClient::TryReadFileserveConfig(const char* szFile, plStringBuilder& out_Result)
{
  plOSFile file;
  if (file.Open(szFile, plFileOpenMode::Read).Succeeded())
  {
    plUInt8 data[64]; // an IP + port should not be longer than 22 characters

    plStringBuilder res;

    data[file.Read(data, 63)] = 0;
    res = (const char*)data;
    res.Trim(" \t\n\r");

    if (res.IsEmpty())
      return PLASMA_FAILURE;

    // has to contain a port number
    if (res.FindSubString(":") == nullptr)
      return PLASMA_FAILURE;

    // otherwise could be an arbitrary string
    out_Result = res;
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plFileserveClient::SearchForServerAddress(plTime timeout /*= plTime::Seconds(5)*/)
{
  PLASMA_LOCK(m_Mutex);
  if (!s_bEnableFileserve)
    return PLASMA_FAILURE;

  plStringBuilder sAddress;

  plStringBuilder tmp;

  // add the command line argument again, in case this was modified since the constructor ran
  // will not change anything, if this is a duplicate
  AddServerAddressToTry(plCommandLineUtils::GetGlobalInstance()->GetStringOption("-fs_server", 0, "").GetData(tmp));

  // go through the available options
  for (plInt32 idx = m_TryServerAddresses.GetCount() - 1; idx >= 0; --idx)
  {
    if (TryConnectWithFileserver(m_TryServerAddresses[idx], timeout).Succeeded())
      return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plFileserveClient::TryConnectWithFileserver(const char* szAddress, plTime timeout) const
{
  PLASMA_LOCK(m_Mutex);
  if (plStringUtils::IsNullOrEmpty(szAddress))
    return PLASMA_FAILURE;

  plLog::Info("File server address: '{0}' ({1} sec)", szAddress, timeout.GetSeconds());

  plUniquePtr<plRemoteInterfaceEnet> network = plRemoteInterfaceEnet::Make(); /// \todo Abstract this somehow ?
  if (network->ConnectToServer('PLFS', szAddress, false).Failed())
    return PLASMA_FAILURE;

  bool bServerFound = false;
  network->SetMessageHandler('FSRV', [&bServerFound](plRemoteMessage& msg) {
      switch (msg.GetMessageID())
      {
        case ' YES':
          bServerFound = true;
          break;
      } });

  if (network->WaitForConnectionToServer(timeout).Succeeded())
  {
    // wait for a proper response
    plTime tStart = plTime::Now();
    while (plTime::Now() - tStart < timeout && !bServerFound)
    {
      network->Send('FSRV', 'RUTR');

      plThreadUtils::Sleep(plTime::Milliseconds(100));

      network->UpdateRemoteInterface();
      network->ExecuteAllMessageHandlers();
    }
  }

  network->ShutdownConnection();

  if (!bServerFound)
    return PLASMA_FAILURE;

  m_sServerConnectionAddress = szAddress;

  // always store the IP that was successful in the user directory
  SaveCurrentConnectionInfoToDisk().IgnoreResult();
  return PLASMA_SUCCESS;
}

plResult plFileserveClient::WaitForServerInfo(plTime timeout /*= plTime::Seconds(60.0 * 5)*/)
{
  PLASMA_LOCK(m_Mutex);
  if (!s_bEnableFileserve)
    return PLASMA_FAILURE;

  plUInt16 uiPort = 1042;
  plHybridArray<plStringBuilder, 4> sServerIPs;

  {
    plUniquePtr<plRemoteInterfaceEnet> network = plRemoteInterfaceEnet::Make(); /// \todo Abstract this somehow ?
    network->SetMessageHandler('FSRV', [&sServerIPs, &uiPort](plRemoteMessage& msg)

      {
        switch (msg.GetMessageID())
        {
          case 'MYIP':
            msg.GetReader() >> uiPort;

            plUInt8 uiCount = 0;
            msg.GetReader() >> uiCount;

            sServerIPs.SetCount(uiCount);
            for (plUInt32 i = 0; i < uiCount; ++i)
            {
              msg.GetReader() >> sServerIPs[i];
            }

            break;
        } });

    PLASMA_SUCCEED_OR_RETURN(network->StartServer('PLIP', "2042", false));

    plTime tStart = plTime::Now();
    while (plTime::Now() - tStart < timeout && sServerIPs.IsEmpty())
    {
      plThreadUtils::Sleep(plTime::Milliseconds(1));

      network->UpdateRemoteInterface();
      network->ExecuteAllMessageHandlers();
    }

    network->ShutdownConnection();
  }

  if (sServerIPs.IsEmpty())
    return PLASMA_FAILURE;

  // network connections are unreliable and surprisingly slow sometimes
  // we just got an IP from a server, so we know it's there and we should be able to connect to it
  // still this often fails the first few times
  // so we try this several times and waste some time in between and hope that at some point the connection succeeds
  for (plUInt32 i = 0; i < 8; ++i)
  {
    plStringBuilder sAddress;
    for (auto& ip : sServerIPs)
    {
      sAddress.Format("{0}:{1}", ip, uiPort);

      plThreadUtils::Sleep(plTime::Milliseconds(500));

      if (TryConnectWithFileserver(sAddress, plTime::Seconds(3)).Succeeded())
        return PLASMA_SUCCESS;
    }

    plThreadUtils::Sleep(plTime::Milliseconds(1000));
  }

  return PLASMA_FAILURE;
}

plResult plFileserveClient::SaveCurrentConnectionInfoToDisk() const
{
  PLASMA_LOCK(m_Mutex);
  plStringBuilder sFile = plOSFile::GetUserDataFolder("plFileserve.txt");
  plOSFile file;
  PLASMA_SUCCEED_OR_RETURN(file.Open(sFile, plFileOpenMode::Write));

  PLASMA_SUCCEED_OR_RETURN(file.Write(m_sServerConnectionAddress.GetData(), m_sServerConnectionAddress.GetElementCount()));
  file.Close();

  return PLASMA_SUCCESS;
}

PLASMA_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  if (plFileserveClient::GetSingleton())
  {
    plFileserveClient::GetSingleton()->UpdateClient();
  }
}



PLASMA_STATICLINK_FILE(FileservePlugin, FileservePlugin_Client_FileserveClient);