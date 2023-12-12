#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Utilities/ConversionUtils.h>

plRemoteInterface::~plRemoteInterface()
{
  // unfortunately we cannot do that ourselves here, because ShutdownConnection() calls virtual functions
  // and this object is already partially destructed here (derived class is already shut down)
  PLASMA_ASSERT_DEV(m_RemoteMode == plRemoteMode::None, "plRemoteInterface::ShutdownConnection() has to be called before destroying the interface");
}

plResult plRemoteInterface::CreateConnection(plUInt32 uiConnectionToken, plRemoteMode mode, plStringView sServerAddress, bool bStartUpdateThread)
{
  plUInt32 uiPrevID = m_uiApplicationID;
  ShutdownConnection();
  m_uiApplicationID = uiPrevID;

  PLASMA_LOCK(GetMutex());

  m_uiConnectionToken = uiConnectionToken;
  m_sServerAddress = sServerAddress;

  if (m_uiApplicationID == 0)
  {
    // create a 'unique' ID to identify this application
    m_uiApplicationID = (plUInt32)plTime::Now().GetSeconds();
  }

  if (InternalCreateConnection(mode, sServerAddress).Failed())
  {
    ShutdownConnection();
    return PLASMA_FAILURE;
  }

  m_RemoteMode = mode;

  UpdateRemoteInterface();

  if (bStartUpdateThread)
  {
    StartUpdateThread();
  }

  return PLASMA_SUCCESS;
}

plResult plRemoteInterface::StartServer(plUInt32 uiConnectionToken, plStringView sAddress, bool bStartUpdateThread /*= true*/)
{
  return CreateConnection(uiConnectionToken, plRemoteMode::Server, sAddress, bStartUpdateThread);
}

plResult plRemoteInterface::ConnectToServer(plUInt32 uiConnectionToken, plStringView sAddress, bool bStartUpdateThread /*= true*/)
{
  return CreateConnection(uiConnectionToken, plRemoteMode::Client, sAddress, bStartUpdateThread);
}

plResult plRemoteInterface::WaitForConnectionToServer(plTime timeout /*= plTime::Seconds(10)*/)
{
  if (m_RemoteMode != plRemoteMode::Client)
    return PLASMA_FAILURE;

  const plTime tStart = plTime::Now();

  while (true)
  {
    UpdateRemoteInterface();

    if (IsConnectedToServer())
      return PLASMA_SUCCESS;

    if (timeout.GetSeconds() != 0)
    {
      if (plTime::Now() - tStart > timeout)
        return PLASMA_FAILURE;
    }

    plThreadUtils::Sleep(plTime::Milliseconds(10));
  }
}

void plRemoteInterface::ShutdownConnection()
{
  StopUpdateThread();

  PLASMA_LOCK(GetMutex());

  if (m_RemoteMode != plRemoteMode::None)
  {
    InternalShutdownConnection();

    m_RemoteMode = plRemoteMode::None;
    m_uiApplicationID = 0;
    m_uiConnectionToken = 0;
  }
}

void plRemoteInterface::UpdatePingToServer()
{
  if (m_RemoteMode == plRemoteMode::Server)
  {
    PLASMA_LOCK(GetMutex());
    m_PingToServer = InternalGetPingToServer();
  }
}

void plRemoteInterface::UpdateRemoteInterface()
{
  PLASMA_LOCK(GetMutex());

  InternalUpdateRemoteInterface();
}

plResult plRemoteInterface::Transmit(plRemoteTransmitMode tm, const plArrayPtr<const plUInt8>& data)
{
  if (m_RemoteMode == plRemoteMode::None)
    return PLASMA_FAILURE;

  PLASMA_LOCK(GetMutex());

  if (InternalTransmit(tm, data).Failed())
    return PLASMA_FAILURE;

  // make sure the message is processed immediately
  UpdateRemoteInterface();

  return PLASMA_SUCCESS;
}


void plRemoteInterface::Send(plUInt32 uiSystemID, plUInt32 uiMsgID)
{
  Send(plRemoteTransmitMode::Reliable, uiSystemID, uiMsgID, plArrayPtr<const plUInt8>());
}

void plRemoteInterface::Send(plRemoteTransmitMode tm, plUInt32 uiSystemID, plUInt32 uiMsgID, const plArrayPtr<const plUInt8>& data)
{
  if (m_RemoteMode == plRemoteMode::None)
    return;

  // if (!IsConnectedToOther())
  //  return;

  m_TempSendBuffer.SetCountUninitialized(12 + data.GetCount());
  *((plUInt32*)&m_TempSendBuffer[0]) = m_uiApplicationID;
  *((plUInt32*)&m_TempSendBuffer[4]) = uiSystemID;
  *((plUInt32*)&m_TempSendBuffer[8]) = uiMsgID;

  if (!data.IsEmpty())
  {
    plUInt8* pCopyDst = &m_TempSendBuffer[12];
    plMemoryUtils::Copy(pCopyDst, data.GetPtr(), data.GetCount());
  }

  Transmit(tm, m_TempSendBuffer).IgnoreResult();
}

void plRemoteInterface::Send(plRemoteTransmitMode tm, plUInt32 uiSystemID, plUInt32 uiMsgID, const void* pData /*= nullptr*/, plUInt32 uiDataBytes /*= 0*/)
{
  Send(tm, uiSystemID, uiMsgID, plArrayPtr<const plUInt8>(reinterpret_cast<const plUInt8*>(pData), uiDataBytes));
}

void plRemoteInterface::Send(plRemoteTransmitMode tm, plRemoteMessage& ref_msg)
{
  Send(tm, ref_msg.GetSystemID(), ref_msg.GetMessageID(), ref_msg.m_Storage);
}

void plRemoteInterface::Send(plRemoteTransmitMode tm, plUInt32 uiSystemID, plUInt32 uiMsgID, const plContiguousMemoryStreamStorage& data)
{
  if (m_RemoteMode == plRemoteMode::None)
    return;

  // if (!IsConnectedToOther())
  //  return;

  plArrayPtr<const plUInt8> range = {data.GetData(), data.GetStorageSize32()};

  m_TempSendBuffer.SetCountUninitialized(12 + range.GetCount());
  *((plUInt32*)&m_TempSendBuffer[0]) = m_uiApplicationID;
  *((plUInt32*)&m_TempSendBuffer[4]) = uiSystemID;
  *((plUInt32*)&m_TempSendBuffer[8]) = uiMsgID;

  if (!range.IsEmpty())
  {
    plUInt8* pCopyDst = &m_TempSendBuffer[12];
    plMemoryUtils::Copy(pCopyDst, range.GetPtr(), range.GetCount());
  }

  Transmit(tm, m_TempSendBuffer).IgnoreResult();
}

void plRemoteInterface::SetMessageHandler(plUInt32 uiSystemID, plRemoteMessageHandler messageHandler)
{
  m_MessageQueues[uiSystemID].m_MessageHandler = messageHandler;
}

plUInt32 plRemoteInterface::ExecuteMessageHandlers(plUInt32 uiSystem)
{
  PLASMA_LOCK(m_Mutex);

  return ExecuteMessageHandlersForQueue(m_MessageQueues[uiSystem]);
}

plUInt32 plRemoteInterface::ExecuteAllMessageHandlers()
{
  PLASMA_LOCK(m_Mutex);

  plUInt32 ret = 0;
  for (auto it = m_MessageQueues.GetIterator(); it.IsValid(); ++it)
  {
    ret += ExecuteMessageHandlersForQueue(it.Value());
  }

  return ret;
}

plUInt32 plRemoteInterface::ExecuteMessageHandlersForQueue(plRemoteMessageQueue& queue)
{
  queue.m_MessageQueueIn.Swap(queue.m_MessageQueueOut);
  const plUInt32 ret = queue.m_MessageQueueOut.GetCount();


  if (queue.m_MessageHandler.IsValid())
  {
    for (auto& msg : queue.m_MessageQueueOut)
    {
      queue.m_MessageHandler(msg);
    }
  }

  queue.m_MessageQueueOut.Clear();

  return ret;
}

void plRemoteInterface::StartUpdateThread()
{
  StopUpdateThread();

  if (m_pUpdateThread == nullptr)
  {
    PLASMA_LOCK(m_Mutex);

    m_pUpdateThread = PLASMA_DEFAULT_NEW(plRemoteThread);
    m_pUpdateThread->m_pRemoteInterface = this;
    m_pUpdateThread->Start();
  }
}

void plRemoteInterface::StopUpdateThread()
{
  if (m_pUpdateThread != nullptr)
  {
    m_pUpdateThread->m_bKeepRunning = false;
    m_pUpdateThread->Join();

    PLASMA_LOCK(m_Mutex);
    PLASMA_DEFAULT_DELETE(m_pUpdateThread);
  }
}


void plRemoteInterface::ReportConnectionToServer(plUInt32 uiServerID)
{
  if (m_uiConnectedToServerWithID == uiServerID)
    return;

  m_uiConnectedToServerWithID = uiServerID;

  plRemoteEvent e;
  e.m_Type = plRemoteEvent::ConnectedToServer;
  e.m_uiOtherAppID = uiServerID;
  m_RemoteEvents.Broadcast(e);
}


void plRemoteInterface::ReportConnectionToClient(plUInt32 uiApplicationID)
{
  m_iConnectionsToClients++;

  plRemoteEvent e;
  e.m_Type = plRemoteEvent::ConnectedToClient;
  e.m_uiOtherAppID = uiApplicationID;
  m_RemoteEvents.Broadcast(e);
}

void plRemoteInterface::ReportDisconnectedFromServer()
{
  m_uiConnectedToServerWithID = 0;

  plRemoteEvent e;
  e.m_Type = plRemoteEvent::DisconnectedFromServer;
  e.m_uiOtherAppID = m_uiConnectedToServerWithID;
  m_RemoteEvents.Broadcast(e);
}

void plRemoteInterface::ReportDisconnectedFromClient(plUInt32 uiApplicationID)
{
  m_iConnectionsToClients--;

  plRemoteEvent e;
  e.m_Type = plRemoteEvent::DisconnectedFromClient;
  e.m_uiOtherAppID = uiApplicationID;
  m_RemoteEvents.Broadcast(e);
}


void plRemoteInterface::ReportMessage(plUInt32 uiApplicationID, plUInt32 uiSystemID, plUInt32 uiMsgID, const plArrayPtr<const plUInt8>& data)
{
  PLASMA_LOCK(m_Mutex);

  auto& queue = m_MessageQueues[uiSystemID];

  // discard messages for which we have no message handler
  if (!queue.m_MessageHandler.IsValid())
    return;

  // store the data for later
  auto& msg = queue.m_MessageQueueIn.ExpandAndGetRef();
  msg.m_uiApplicationID = uiApplicationID;
  msg.SetMessageID(uiSystemID, uiMsgID);
  msg.GetWriter().WriteBytes(data.GetPtr(), data.GetCount()).IgnoreResult();
}

plResult plRemoteInterface::DetermineTargetAddress(plStringView sConnectTo0, plUInt32& out_IP, plUInt16& out_Port)
{
  out_IP = 0;
  out_Port = 0;

  plStringBuilder sConnectTo = sConnectTo0;

  const char* szColon = sConnectTo.FindLastSubString(":");
  if (szColon != nullptr)
  {
    sConnectTo.Shrink(0, plStringUtils::GetStringElementCount(szColon));

    plStringBuilder sPort = szColon + 1;

    plInt32 tmp;
    if (plConversionUtils::StringToInt(sPort, tmp).Succeeded())
      out_Port = static_cast<plUInt16>(tmp);
  }

  plInt32 ip1 = 0;
  plInt32 ip2 = 0;
  plInt32 ip3 = 0;
  plInt32 ip4 = 0;

  if (sConnectTo.IsEmpty() || sConnectTo.IsEqual_NoCase("localhost"))
  {
    ip1 = 127;
    ip2 = 0;
    ip3 = 0;
    ip4 = 1;
  }
  else if (sConnectTo.FindSubString(".") != nullptr)
  {
    plHybridArray<plString, 8> IP;
    sConnectTo.Split(false, IP, ".");

    if (IP.GetCount() != 4)
      return PLASMA_FAILURE;

    if (plConversionUtils::StringToInt(IP[0], ip1).Failed())
      return PLASMA_FAILURE;
    if (plConversionUtils::StringToInt(IP[1], ip2).Failed())
      return PLASMA_FAILURE;
    if (plConversionUtils::StringToInt(IP[2], ip3).Failed())
      return PLASMA_FAILURE;
    if (plConversionUtils::StringToInt(IP[3], ip4).Failed())
      return PLASMA_FAILURE;
  }
  else
  {
    return PLASMA_FAILURE;
  }

  out_IP = ((ip1 & 0xFF) | (ip2 & 0xFF) << 8 | (ip3 & 0xFF) << 16 | (ip4 & 0xFF) << 24);
  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plRemoteThread::plRemoteThread()
  : plThread("plRemoteThread")
{
}

plUInt32 plRemoteThread::Run()
{
  plTime lastPing;

  while (m_bKeepRunning && m_pRemoteInterface)
  {
    m_pRemoteInterface->UpdateRemoteInterface();

    // Send a Ping every once in a while
    if (m_pRemoteInterface->GetRemoteMode() == plRemoteMode::Client)
    {
      plTime tNow = plTime::Now();

      if (tNow - lastPing > plTime::Milliseconds(500))
      {
        lastPing = tNow;

        m_pRemoteInterface->UpdatePingToServer();
      }
    }

    plThreadUtils::Sleep(plTime::Milliseconds(10));
  }

  return 0;
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_RemoteInterface);
