#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveClient.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/CommandLineUtils.h>

PLASMA_IMPLEMENT_SINGLETON(plFileserver);

plFileserver::plFileserver()
  : m_SingletonRegistrar(this)
{
  // once a server exists, the client should stay inactive
  plFileserveClient::DisabledFileserveClient();

  // check whether the fileserve port was reconfigured through the command line
  m_uiPort = static_cast<plUInt16>(plCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_port", m_uiPort));
}

void plFileserver::StartServer()
{
  if (m_pNetwork)
    return;

  plStringBuilder tmp;

  m_pNetwork = plRemoteInterfaceEnet::Make();
  m_pNetwork->StartServer('PLFS', plConversionUtils::ToString(m_uiPort, tmp), false).IgnoreResult();
  m_pNetwork->SetMessageHandler('FSRV', plMakeDelegate(&plFileserver::NetworkMsgHandler, this));
  m_pNetwork->m_RemoteEvents.AddEventHandler(plMakeDelegate(&plFileserver::NetworkEventHandler, this));

  plFileserverEvent e;
  e.m_Type = plFileserverEvent::Type::ServerStarted;
  m_Events.Broadcast(e);
}

void plFileserver::StopServer()
{
  if (!m_pNetwork)
    return;

  m_pNetwork->ShutdownConnection();
  m_pNetwork.Clear();

  plFileserverEvent e;
  e.m_Type = plFileserverEvent::Type::ServerStopped;
  m_Events.Broadcast(e);
}

bool plFileserver::UpdateServer()
{
  if (!m_pNetwork)
    return false;

  m_pNetwork->UpdateRemoteInterface();
  return m_pNetwork->ExecuteAllMessageHandlers() > 0;
}

bool plFileserver::IsServerRunning() const
{
  return m_pNetwork != nullptr;
}

void plFileserver::SetPort(plUInt16 uiPort)
{
  PLASMA_ASSERT_DEV(m_pNetwork == nullptr, "The port cannot be changed after the server was started");
  m_uiPort = uiPort;
}


void plFileserver::BroadcastReloadResourcesCommand()
{
  if (!IsServerRunning())
    return;

  m_pNetwork->Send('FSRV', 'RLDR');
}

void plFileserver::NetworkMsgHandler(plRemoteMessage& msg)
{
  auto& client = DetermineClient(msg);

  if (msg.GetMessageID() == 'HELO')
    return;

  if (msg.GetMessageID() == 'RUTR')
  {
    // 'are you there' is used to check whether a certain address is a proper Fileserver
    m_pNetwork->Send('FSRV', ' YES');

    plFileserverEvent e;
    e.m_Type = plFileserverEvent::Type::AreYouThereRequest;
    m_Events.Broadcast(e);
    return;
  }

  if (msg.GetMessageID() == 'READ')
  {
    HandleFileRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLH')
  {
    HandleUploadFileHeader(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLD')
  {
    HandleUploadFileTransfer(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLF')
  {
    HandleUploadFileFinished(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'DELF')
  {
    HandleDeleteFileRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == ' MNT')
  {
    HandleMountRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UMNT')
  {
    HandleUnmountRequest(client, msg);
    return;
  }

  plLog::Error("Unknown FSRV message: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageData().GetCount());
}


void plFileserver::NetworkEventHandler(const plRemoteEvent& e)
{
  switch (e.m_Type)
  {
    case plRemoteEvent::DisconnectedFromClient:
    {
      if (m_Clients.Contains(e.m_uiOtherAppID))
      {
        plFileserverEvent se;
        se.m_Type = plFileserverEvent::Type::ClientDisconnected;
        se.m_uiClientID = e.m_uiOtherAppID;

        m_Events.Broadcast(se);

        m_Clients[e.m_uiOtherAppID].m_bLostConnection = true;
      }
    }
    break;

    default:
      break;
  }
}

plFileserveClientContext& plFileserver::DetermineClient(plRemoteMessage& msg)
{
  plFileserveClientContext& client = m_Clients[msg.GetApplicationID()];

  if (client.m_uiApplicationID != msg.GetApplicationID())
  {
    client.m_uiApplicationID = msg.GetApplicationID();

    plFileserverEvent e;
    e.m_Type = plFileserverEvent::Type::ClientConnected;
    e.m_uiClientID = client.m_uiApplicationID;
    m_Events.Broadcast(e);
  }
  else if (client.m_bLostConnection)
  {
    client.m_bLostConnection = false;

    plFileserverEvent e;
    e.m_Type = plFileserverEvent::Type::ClientReconnected;
    e.m_uiClientID = client.m_uiApplicationID;
    m_Events.Broadcast(e);
  }

  return client;
}

void plFileserver::HandleMountRequest(plFileserveClientContext& client, plRemoteMessage& msg)
{
  plStringBuilder sDataDir, sRootName, sMountPoint, sRedir;
  plUInt16 uiDataDirID = 0xffff;

  msg.GetReader() >> sDataDir;
  msg.GetReader() >> sRootName;
  msg.GetReader() >> sMountPoint;
  msg.GetReader() >> uiDataDirID;

  PLASMA_ASSERT_DEV(uiDataDirID >= client.m_MountedDataDirs.GetCount(), "Data dir ID should be larger than previous IDs");

  client.m_MountedDataDirs.SetCount(plMath::Max<plUInt32>(uiDataDirID + 1, client.m_MountedDataDirs.GetCount()));
  auto& dir = client.m_MountedDataDirs[uiDataDirID];
  dir.m_sPathOnClient = sDataDir;
  dir.m_sRootName = sRootName;
  dir.m_sMountPoint = sMountPoint;

  plFileserverEvent e;

  if (plFileSystem::ResolveSpecialDirectory(sDataDir, sRedir).Succeeded())
  {
    dir.m_bMounted = true;
    dir.m_sPathOnServer = sRedir;
    e.m_Type = plFileserverEvent::Type::MountDataDir;
  }
  else
  {
    dir.m_bMounted = false;
    e.m_Type = plFileserverEvent::Type::MountDataDirFailed;
  }

  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szName = sRootName;
  e.m_szPath = sDataDir;
  e.m_szRedirectedPath = sRedir;
  m_Events.Broadcast(e);
}


void plFileserver::HandleUnmountRequest(plFileserveClientContext& client, plRemoteMessage& msg)
{
  plUInt16 uiDataDirID = 0xffff;
  msg.GetReader() >> uiDataDirID;

  PLASMA_ASSERT_DEV(uiDataDirID < client.m_MountedDataDirs.GetCount(), "Invalid data dir ID to unmount");

  auto& dir = client.m_MountedDataDirs[uiDataDirID];
  dir.m_bMounted = false;

  plFileserverEvent e;
  e.m_Type = plFileserverEvent::Type::UnmountDataDir;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = dir.m_sPathOnClient;
  e.m_szName = dir.m_sRootName;
  m_Events.Broadcast(e);
}

void plFileserver::HandleFileRequest(plFileserveClientContext& client, plRemoteMessage& msg)
{
  plUInt16 uiDataDirID = 0;
  bool bForceThisDataDir = false;

  msg.GetReader() >> uiDataDirID;
  msg.GetReader() >> bForceThisDataDir;

  plStringBuilder sRequestedFile;
  msg.GetReader() >> sRequestedFile;

  plUuid downloadGuid;
  msg.GetReader() >> downloadGuid;

  plFileserveClientContext::FileStatus status;
  msg.GetReader() >> status.m_iTimestamp;
  msg.GetReader() >> status.m_uiHash;

  plFileserverEvent e;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = sRequestedFile;
  e.m_uiSentTotal = 0;

  const plFileserveFileState filestate = client.GetFileStatus(uiDataDirID, sRequestedFile, status, m_SendToClient, bForceThisDataDir);

  {
    e.m_Type = plFileserverEvent::Type::FileDownloadRequest;
    e.m_uiSizeTotal = m_SendToClient.GetCount();
    e.m_FileState = filestate;
    m_Events.Broadcast(e);
  }

  if (filestate == plFileserveFileState::Different)
  {
    plUInt32 uiNextByte = 0;
    const plUInt32 uiFileSize = m_SendToClient.GetCount();

    // send the file over in multiple packages of 1KB each
    // send at least one package, even for empty files
    do
    {
      const plUInt16 uiChunkSize = (plUInt16)plMath::Min<plUInt32>(1024, m_SendToClient.GetCount() - uiNextByte);

      plRemoteMessage ret;
      ret.GetWriter() << downloadGuid;
      ret.GetWriter() << uiChunkSize;
      ret.GetWriter() << uiFileSize;

      if (!m_SendToClient.IsEmpty())
        ret.GetWriter().WriteBytes(&m_SendToClient[uiNextByte], uiChunkSize).IgnoreResult();

      ret.SetMessageID('FSRV', 'DWNL');
      m_pNetwork->Send(plRemoteTransmitMode::Reliable, ret);

      uiNextByte += uiChunkSize;

      // reuse previous values
      {
        e.m_Type = plFileserverEvent::Type::FileDownloading;
        e.m_uiSentTotal = uiNextByte;
        m_Events.Broadcast(e);
      }
    } while (uiNextByte < m_SendToClient.GetCount());
  }

  // final answer to client
  {
    plRemoteMessage ret('FSRV', 'DWNF');
    ret.GetWriter() << downloadGuid;
    ret.GetWriter() << (plInt8)filestate;
    ret.GetWriter() << status.m_iTimestamp;
    ret.GetWriter() << status.m_uiHash;
    ret.GetWriter() << uiDataDirID;

    m_pNetwork->Send(plRemoteTransmitMode::Reliable, ret);
  }

  // reuse previous values
  {
    e.m_Type = plFileserverEvent::Type::FileDownloadFinished;
    m_Events.Broadcast(e);
  }
}

void plFileserver::HandleDeleteFileRequest(plFileserveClientContext& client, plRemoteMessage& msg)
{
  plUInt16 uiDataDirID = 0xffff;
  msg.GetReader() >> uiDataDirID;

  plStringBuilder sFile;
  msg.GetReader() >> sFile;

  PLASMA_ASSERT_DEV(uiDataDirID < client.m_MountedDataDirs.GetCount(), "Invalid data dir ID to unmount");

  plFileserverEvent e;
  e.m_Type = plFileserverEvent::Type::FileDeleteRequest;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = sFile;
  m_Events.Broadcast(e);

  const auto& dd = client.m_MountedDataDirs[uiDataDirID];

  plStringBuilder sAbsPath;
  sAbsPath = dd.m_sPathOnServer;
  sAbsPath.AppendPath(sFile);

  plOSFile::DeleteFile(sAbsPath).IgnoreResult();
}

void plFileserver::HandleUploadFileHeader(plFileserveClientContext& client, plRemoteMessage& msg)
{
  plUInt16 uiDataDirID = 0;

  msg.GetReader() >> m_FileUploadGuid;
  msg.GetReader() >> m_uiFileUploadSize;
  msg.GetReader() >> uiDataDirID;
  msg.GetReader() >> m_sCurFileUpload;

  m_SentFromClient.Clear();
  m_SentFromClient.Reserve(m_uiFileUploadSize);

  plFileserverEvent e;
  e.m_Type = plFileserverEvent::Type::FileUploadRequest;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = m_sCurFileUpload;
  e.m_uiSentTotal = 0;
  e.m_uiSizeTotal = m_uiFileUploadSize;

  m_Events.Broadcast(e);
}

void plFileserver::HandleUploadFileTransfer(plFileserveClientContext& client, plRemoteMessage& msg)
{
  plUuid transferGuid;
  msg.GetReader() >> transferGuid;

  if (transferGuid != m_FileUploadGuid)
    return;

  plUInt16 uiChunkSize = 0;
  msg.GetReader() >> uiChunkSize;

  const plUInt32 uiStartPos = m_SentFromClient.GetCount();
  m_SentFromClient.SetCountUninitialized(uiStartPos + uiChunkSize);
  msg.GetReader().ReadBytes(&m_SentFromClient[uiStartPos], uiChunkSize);

  plFileserverEvent e;
  e.m_Type = plFileserverEvent::Type::FileUploading;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = m_sCurFileUpload;
  e.m_uiSentTotal = m_SentFromClient.GetCount();
  e.m_uiSizeTotal = m_uiFileUploadSize;

  m_Events.Broadcast(e);
}

void plFileserver::HandleUploadFileFinished(plFileserveClientContext& client, plRemoteMessage& msg)
{
  plUuid transferGuid;
  msg.GetReader() >> transferGuid;

  if (transferGuid != m_FileUploadGuid)
    return;

  plUInt16 uiDataDirID = 0;
  msg.GetReader() >> uiDataDirID;

  plStringBuilder sFile;
  msg.GetReader() >> sFile;

  plStringBuilder sOutputFile;
  sOutputFile = client.m_MountedDataDirs[uiDataDirID].m_sPathOnServer;
  sOutputFile.AppendPath(sFile);

  {
    plOSFile file;
    if (file.Open(sOutputFile, plFileOpenMode::Write).Failed())
    {
      plLog::Error("Could not write uploaded file to '{0}'", sOutputFile);
      return;
    }

    if (!m_SentFromClient.IsEmpty())
    {
      file.Write(m_SentFromClient.GetData(), m_SentFromClient.GetCount()).IgnoreResult();
    }
  }

  plFileserverEvent e;
  e.m_Type = plFileserverEvent::Type::FileUploadFinished;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = sFile;
  e.m_uiSentTotal = m_SentFromClient.GetCount();
  e.m_uiSizeTotal = m_SentFromClient.GetCount();

  m_Events.Broadcast(e);

  // send a response when all data has been transmitted
  // this ensures the client side updates the network until all data has been fully transmitted
  m_pNetwork->Send('FSRV', 'UACK');
}


plResult plFileserver::SendConnectionInfo(const char* szClientAddress, plUInt16 uiMyPort, const plArrayPtr<plStringBuilder>& MyIPs, plTime timeout)
{
  plStringBuilder sAddress = szClientAddress;
  sAddress.Append(":2042"); // hard-coded port

  plUniquePtr<plRemoteInterfaceEnet> network = plRemoteInterfaceEnet::Make();
  PLASMA_SUCCEED_OR_RETURN(network->ConnectToServer('PLIP', sAddress, false));

  if (network->WaitForConnectionToServer(timeout).Failed())
  {
    network->ShutdownConnection();
    return PLASMA_FAILURE;
  }

  const plUInt8 uiCount = static_cast<plUInt8>(MyIPs.GetCount());

  plRemoteMessage msg('FSRV', 'MYIP');
  msg.GetWriter() << uiMyPort;
  msg.GetWriter() << uiCount;

  for (const auto& info : MyIPs)
  {
    msg.GetWriter() << info;
  }

  network->Send(plRemoteTransmitMode::Reliable, msg);

  // make sure the message is out, before we shut down
  for (plUInt32 i = 0; i < 10; ++i)
  {
    network->UpdateRemoteInterface();
    plThreadUtils::Sleep(plTime::Milliseconds(1));
  }

  network->ShutdownConnection();
  return PLASMA_SUCCESS;
}



PLASMA_STATICLINK_FILE(FileservePlugin, FileservePlugin_Fileserver_Fileserver);
