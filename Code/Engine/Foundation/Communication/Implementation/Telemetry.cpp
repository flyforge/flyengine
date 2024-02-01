#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/ThreadUtils.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
#  include <enet/enet.h>
#endif

class plTelemetryThread;

plTelemetry::plEventTelemetry plTelemetry::s_TelemetryEvents;
plUInt32 plTelemetry::s_uiApplicationID = 0;
plUInt32 plTelemetry::s_uiServerID = 0;
plUInt16 plTelemetry::s_uiPort = 1040;
bool plTelemetry::s_bConnectedToServer = false;
bool plTelemetry::s_bConnectedToClient = false;
bool plTelemetry::s_bAllowNetworkUpdate = true;
plTime plTelemetry::s_PingToServer;
plString plTelemetry::s_sServerName;
plString plTelemetry::s_sServerIP;
static bool g_bInitialized = false;
plTelemetry::ConnectionMode plTelemetry::s_ConnectionMode = plTelemetry::None;
plMap<plUInt64, plTelemetry::MessageQueue> plTelemetry::s_SystemMessages;

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
static ENetAddress g_pServerAddress;
static ENetHost* g_pHost = nullptr;
static ENetPeer* g_pConnectionToServer = nullptr;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT

void plTelemetry::UpdateServerPing()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  enet_peer_ping(g_pConnectionToServer);
  plTelemetry::s_PingToServer = plTime::MakeFromMilliseconds(g_pConnectionToServer->lastRoundTripTime);
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void plTelemetry::UpdateNetwork()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  if (!s_bAllowNetworkUpdate)
    return;

  s_bAllowNetworkUpdate = false;

  ENetEvent NetworkEvent;

  while (true)
  {
    PL_LOCK(GetTelemetryMutex());

    const plInt32 iStatus = enet_host_service(g_pHost, &NetworkEvent, 0);

    if (iStatus <= 0)
    {
      s_bAllowNetworkUpdate = true;
      return;
    }

    switch (NetworkEvent.type)
    {
      case ENET_EVENT_TYPE_CONNECT:
      {
        if ((plTelemetry::s_ConnectionMode == plTelemetry::Server) && (NetworkEvent.peer->eventData != 'PLBC'))
        {
          enet_peer_disconnect(NetworkEvent.peer, 0);
          break;
        }

        if (s_ConnectionMode == Client)
        {
          char szHostIP[64] = "<unknown>";
          // char szHostName[64] = "<unknown>";

          enet_address_get_host_ip(&NetworkEvent.peer->address, szHostIP, 63);

          // Querying host IP and name can take a lot of time which can lead to timeouts
          // enet_address_get_host(&NetworkEvent.peer->address, szHostName, 63);

          plTelemetry::s_sServerIP = szHostIP;
          // plTelemetry::s_ServerName = szHostName;

          // now we are waiting for the server to send its ID
        }
        else
        {
          // got a new client, send the server ID to it
          s_bConnectedToClient = true; // we need this fake state, otherwise Broadcast will queue the message instead of sending it
          Broadcast(plTelemetry::Reliable, 'PLBC', 'PLID', &s_uiApplicationID, sizeof(plUInt32));
          s_bConnectedToClient = false;

          // then wait for its acknowledgment message
        }
      }
      break;

      case ENET_EVENT_TYPE_DISCONNECT:
      {
        if (s_ConnectionMode == Client)
        {
          s_bConnectedToServer = false;

          // First wait a bit to ensure that the Server could shut down, if this was a legitimate disconnect
          plThreadUtils::Sleep(plTime::MakeFromSeconds(1));

          // Now try to reconnect. If the Server still exists, fine, connect to that.
          // If it does not exist anymore, this will connect to the next best Server that can be found.
          g_pConnectionToServer = enet_host_connect(g_pHost, &g_pServerAddress, 2, 'PLBC');

          TelemetryEventData e;
          e.m_EventType = TelemetryEventData::DisconnectedFromServer;

          s_TelemetryEvents.Broadcast(e);
        }
        else
        {
          /// \todo This assumes we only connect to a single client ...
          s_bConnectedToClient = false;

          TelemetryEventData e;
          e.m_EventType = TelemetryEventData::DisconnectedFromClient;

          s_TelemetryEvents.Broadcast(e);
        }
      }
      break;

      case ENET_EVENT_TYPE_RECEIVE:
      {
        const plUInt32 uiSystemID = *((plUInt32*)&NetworkEvent.packet->data[0]);
        const plUInt32 uiMsgID = *((plUInt32*)&NetworkEvent.packet->data[4]);
        const plUInt8* pData = &NetworkEvent.packet->data[8];

        if (uiSystemID == 'PLBC')
        {
          switch (uiMsgID)
          {
            case 'PLID':
            {
              s_uiServerID = *((plUInt32*)pData);

              // connection to server is finalized
              s_bConnectedToServer = true;

              // acknowledge that the ID has been received
              SendToServer('PLBC', 'AKID', nullptr, 0);

              // go tell the others about it
              TelemetryEventData e;
              e.m_EventType = TelemetryEventData::ConnectedToServer;

              s_TelemetryEvents.Broadcast(e);

              FlushOutgoingQueues();
            }
            break;
            case 'AKID':
            {
              // the client received the server ID -> the connection has been established properly

              /// \todo This assumes we only connect to a single client ...
              s_bConnectedToClient = true;

              // go tell the others about it
              TelemetryEventData e;
              e.m_EventType = TelemetryEventData::ConnectedToClient;

              s_TelemetryEvents.Broadcast(e);

              SendServerName();
              FlushOutgoingQueues();
            }
            break;

            case 'NAME':
            {
              s_sServerName = reinterpret_cast<const char*>(pData);
            }
            break;
          }
        }
        else
        {
          MessageQueue& Queue = s_SystemMessages[uiSystemID];

          if (Queue.m_bAcceptMessages)
          {
            Queue.m_IncomingQueue.PushBack();
            plTelemetryMessage& Msg = Queue.m_IncomingQueue.PeekBack();

            Msg.SetMessageID(uiSystemID, uiMsgID);

            PL_ASSERT_DEV((plUInt32)NetworkEvent.packet->dataLength >= 8, "Message Length Invalid: {0}", (plUInt32)NetworkEvent.packet->dataLength);

            Msg.GetWriter().WriteBytes(pData, NetworkEvent.packet->dataLength - 8).IgnoreResult();
          }
        }

        enet_packet_destroy(NetworkEvent.packet);
      }
      break;

      default:
        break;
    }
  }

  s_bAllowNetworkUpdate = true;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void plTelemetry::SetServerName(plStringView sName)
{
  if (s_ConnectionMode == ConnectionMode::Client)
    return;

  if (s_sServerName == sName)
    return;

  s_sServerName = sName;

  SendServerName();
}

void plTelemetry::SendServerName()
{
  if (!IsConnectedToOther())
    return;

  char data[48];
  plStringUtils::Copy(data, PL_ARRAY_SIZE(data), s_sServerName.GetData());

  Broadcast(plTelemetry::Reliable, 'PLBC', 'NAME', data, PL_ARRAY_SIZE(data));
}

plResult plTelemetry::RetrieveMessage(plUInt32 uiSystemID, plTelemetryMessage& out_message)
{
  if (s_SystemMessages[uiSystemID].m_IncomingQueue.IsEmpty())
    return PL_FAILURE;

  PL_LOCK(GetTelemetryMutex());

  // check again while inside the lock
  if (s_SystemMessages[uiSystemID].m_IncomingQueue.IsEmpty())
    return PL_FAILURE;

  out_message = s_SystemMessages[uiSystemID].m_IncomingQueue.PeekFront();
  s_SystemMessages[uiSystemID].m_IncomingQueue.PopFront();

  return PL_SUCCESS;
}

void plTelemetry::InitializeAsServer()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  g_pServerAddress.host = ENET_HOST_ANY;
  g_pServerAddress.port = s_uiPort;

  g_pHost = enet_host_create(&g_pServerAddress, 32, 2, 0, 0);
#else
  plLog::SeriousWarning("Enet is not compiled into this build, plTelemetry::InitializeAsServer() will be ignored.");
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

plResult plTelemetry::InitializeAsClient(plStringView sConnectTo0)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  g_pHost = enet_host_create(nullptr, 1, 2, 0, 0);

  plStringBuilder sConnectTo = sConnectTo0;

  const char* szColon = sConnectTo.FindLastSubString(":");
  if (szColon != nullptr)
  {
    sConnectTo.Shrink(0, plStringUtils::GetStringElementCount(szColon));

    plStringBuilder sPort = szColon + 1;
    s_uiPort = static_cast<plUInt16>(atoi(sPort.GetData()));
  }

  if (sConnectTo.IsEmpty() || sConnectTo.IsEqual_NoCase("localhost"))
    enet_address_set_host(&g_pServerAddress, "localhost");
  else if (sConnectTo.FindSubString(".") != nullptr)
  {
    plHybridArray<plString, 8> IP;
    sConnectTo.Split(false, IP, ".");

    if (IP.GetCount() != 4)
      return PL_FAILURE;

    const plUInt32 ip1 = atoi(IP[0].GetData()) & 0xFF;
    const plUInt32 ip2 = atoi(IP[1].GetData()) & 0xFF;
    const plUInt32 ip3 = atoi(IP[2].GetData()) & 0xFF;
    const plUInt32 ip4 = atoi(IP[3].GetData()) & 0xFF;

    const plUInt32 uiIP = (ip1 | ip2 << 8 | ip3 << 16 | ip4 << 24);

    g_pServerAddress.host = uiIP;
  }
  else
    enet_address_set_host(&g_pServerAddress, sConnectTo.GetData());

  g_pServerAddress.port = s_uiPort;

  g_pConnectionToServer = nullptr;
  g_pConnectionToServer = enet_host_connect(g_pHost, &g_pServerAddress, 2, 'PLBC');

  if (g_pConnectionToServer)
    return PL_SUCCESS;
#else
  plLog::SeriousWarning("Enet is not compiled into this build, plTelemetry::InitializeAsClient() will be ignored.");
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT

  return PL_FAILURE;
}

plResult plTelemetry::OpenConnection(ConnectionMode Mode, plStringView sConnectTo)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  CloseConnection();

  if (!g_bInitialized)
  {
    if (enet_initialize() != 0)
    {
      plLog::Error("Enet could not be initialized.");
      return PL_FAILURE;
    }

    g_bInitialized = true;
  }

  s_uiApplicationID = (plUInt32)plTime::Now().GetSeconds();

  switch (Mode)
  {
    case plTelemetry::Server:
      InitializeAsServer();
      break;
    case plTelemetry::Client:
      if (InitializeAsClient(sConnectTo) == PL_FAILURE)
      {
        CloseConnection();
        return PL_FAILURE;
      }
      break;
    default:
      break;
  }

  s_ConnectionMode = Mode;

  plTelemetry::UpdateNetwork();

  StartTelemetryThread();

  return PL_SUCCESS;
#else
  plLog::SeriousWarning("Enet is not compiled into this build, plTelemetry::OpenConnection() will be ignored.");
  return PL_FAILURE;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void plTelemetry::Transmit(TransmitMode tm, const void* pData, plUInt32 uiDataBytes)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  PL_LOCK(GetTelemetryMutex());

  ENetPacket* pPacket = enet_packet_create(pData, uiDataBytes, (tm == Reliable) ? ENET_PACKET_FLAG_RELIABLE : 0);
  enet_host_broadcast(g_pHost, 0, pPacket);

  // make sure the message is processed immediately
  plTelemetry::UpdateNetwork();
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void plTelemetry::Send(TransmitMode tm, plUInt32 uiSystemID, plUInt32 uiMsgID, const void* pData, plUInt32 uiDataBytes)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  // in case we have no connection to a peer, queue the message
  if (!IsConnectedToOther())
    QueueOutgoingMessage(tm, uiSystemID, uiMsgID, pData, uiDataBytes);
  else
  {
    // when we do have a connection, just send the message out

    plHybridArray<plUInt8, 64> TempData;
    TempData.SetCountUninitialized(8 + uiDataBytes);
    *((plUInt32*)&TempData[0]) = uiSystemID;
    *((plUInt32*)&TempData[4]) = uiMsgID;

    if (pData && uiDataBytes > 0)
      plMemoryUtils::Copy((plUInt8*)&TempData[8], (plUInt8*)pData, uiDataBytes);

    Transmit(tm, &TempData[0], TempData.GetCount());
  }
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void plTelemetry::Send(TransmitMode tm, plUInt32 uiSystemID, plUInt32 uiMsgID, plStreamReader& Stream, plInt32 iDataBytes)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  const plUInt32 uiStackSize = 1024;

  plHybridArray<plUInt8, uiStackSize + 8> TempData;
  TempData.SetCountUninitialized(8);
  *((plUInt32*)&TempData[0]) = uiSystemID;
  *((plUInt32*)&TempData[4]) = uiMsgID;

  // if we don't know how much to take out of the stream, read the data piece by piece from the input stream
  if (iDataBytes < 0)
  {
    while (true)
    {
      const plUInt32 uiOffset = TempData.GetCount();
      TempData.SetCountUninitialized(uiOffset + uiStackSize); // no allocation the first time

      const plUInt32 uiRead = static_cast<plUInt32>(Stream.ReadBytes(&TempData[uiOffset], uiStackSize));

      if (uiRead < uiStackSize)
      {
        // resize the array down to its actual size
        TempData.SetCountUninitialized(uiOffset + uiRead);
        break;
      }
    }
  }
  else
  {
    TempData.SetCountUninitialized(8 + iDataBytes);

    if (iDataBytes > 0)
      Stream.ReadBytes(&TempData[8], iDataBytes);
  }

  // in case we have no connection to a peer, queue the message
  if (!IsConnectedToOther())
  {
    if (TempData.GetCount() > 8)
      QueueOutgoingMessage(tm, uiSystemID, uiMsgID, &TempData[8], TempData.GetCount() - 8);
    else
      QueueOutgoingMessage(tm, uiSystemID, uiMsgID, nullptr, 0);
  }
  else
  {
    // when we do have a connection, just send the message out
    Transmit(tm, &TempData[0], TempData.GetCount());
  }
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void plTelemetry::CloseConnection()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  s_bConnectedToServer = false;
  s_bConnectedToClient = false;
  s_ConnectionMode = None;
  s_uiServerID = 0;
  g_pConnectionToServer = nullptr;

  StopTelemetryThread();

  // prevent other threads from interfering
  PL_LOCK(GetTelemetryMutex());

  UpdateNetwork();
  plThreadUtils::Sleep(plTime::MakeFromMilliseconds(10));

  if (g_pHost)
  {
    // send all peers that we are disconnecting
    for (plUInt32 i = (plUInt32)g_pHost->connectedPeers; i > 0; --i)
      enet_peer_disconnect(&g_pHost->peers[i - 1], 0);

    // process the network messages (e.g. send the disconnect messages)
    UpdateNetwork();
    plThreadUtils::Sleep(plTime::MakeFromMilliseconds(10));
  }

  // finally close the network connection
  if (g_pHost)
  {
    enet_host_destroy(g_pHost);
    g_pHost = nullptr;
  }

  if (g_bInitialized)
  {
    enet_deinitialize();
    g_bInitialized = false;
  }

  // if there are any queued messages, throw them away
  for (auto it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_IncomingQueue.Clear();
    it.Value().m_OutgoingQueue.Clear();
  }
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}


