#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/RemoteInterfaceEnet.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

#  include <Foundation/Logging/Log.h>
#  include <Foundation/Threading/ThreadUtils.h>
#  include <Foundation/Types/ScopeExit.h>
#  include <enet/enet.h>

class plRemoteInterfaceEnetImpl : public plRemoteInterfaceEnet
{

protected:
  virtual void InternalUpdateRemoteInterface() override;
  virtual plResult InternalCreateConnection(plRemoteMode mode, plStringView sServerAddress) override;
  virtual void InternalShutdownConnection() override;
  virtual plTime InternalGetPingToServer() override;
  virtual plResult InternalTransmit(plRemoteTransmitMode tm, const plArrayPtr<const plUInt8>& data) override;

private:
  ENetAddress m_EnetServerAddress;
  ENetHost* m_pEnetHost = nullptr;
  ENetPeer* m_pEnetConnectionToServer = nullptr;
  bool m_bAllowNetworkUpdates = true;
  plMap<void*, plUInt32> m_EnetPeerToClientID;

  static bool s_bEnetInitialized;
};

plInternal::NewInstance<plRemoteInterfaceEnet> plRemoteInterfaceEnet::Make(plAllocator* pAllocator /*= plFoundation::GetDefaultAllocator()*/)
{
  return PL_NEW(pAllocator, plRemoteInterfaceEnetImpl);
}

plRemoteInterfaceEnet::plRemoteInterfaceEnet() = default;
plRemoteInterfaceEnet::~plRemoteInterfaceEnet() = default;

bool plRemoteInterfaceEnetImpl::s_bEnetInitialized = false;

plResult plRemoteInterfaceEnetImpl::InternalCreateConnection(plRemoteMode mode, plStringView sServerAddress)
{
  if (!s_bEnetInitialized)
  {
    if (enet_initialize() != 0)
    {
      plLog::Error("Failed to initialize Enet");
      return PL_FAILURE;
    }

    s_bEnetInitialized = true;
  }

  {
    // Extract port from address
    const char* szPortStart = sServerAddress.FindLastSubString(":");
    plStringView sPort = (szPortStart != nullptr) ? plStringView(szPortStart + 1, sServerAddress.GetEndPointer()) : sServerAddress;
    plInt32 iPort = 0;
    if (plConversionUtils::StringToInt(sPort, iPort).Failed())
    {
      plLog::Error("Failed to extract port from server address: {0}", sServerAddress);
      return PL_FAILURE;
    }
    m_uiPort = static_cast<plUInt16>(iPort);
  }

  m_pEnetConnectionToServer = nullptr;

  ENetAddress* pServerAddress = nullptr;
  size_t maxPeerCount = 1;
  const size_t maxChannels = 2;
  const enet_uint32 incomingBandwidth = 0; // unlimited
  const enet_uint32 outgoingBandwidth = 0; // unlimited

  if (mode == plRemoteMode::Server)
  {
    m_EnetServerAddress.host = ENET_HOST_ANY;
    m_EnetServerAddress.port = m_uiPort;

    maxPeerCount = 8;
    pServerAddress = &m_EnetServerAddress;
  }
  else
  {
    if (DetermineTargetAddress(sServerAddress, m_EnetServerAddress.host, m_EnetServerAddress.port).Failed())
    {
      plStringBuilder tmp;
      enet_address_set_host(&m_EnetServerAddress, sServerAddress.GetData(tmp));
    }

    // use default settings for enet_host_create
  }

  m_pEnetHost = enet_host_create(pServerAddress, maxPeerCount, maxChannels, incomingBandwidth, outgoingBandwidth);

  if (m_pEnetHost == nullptr)
  {
    plLog::Error("Failed to create an Enet server");
    return PL_FAILURE;
  }

  if (mode == plRemoteMode::Client)
  {
    m_pEnetConnectionToServer = enet_host_connect(m_pEnetHost, &m_EnetServerAddress, maxChannels, GetConnectionToken());

    if (m_pEnetConnectionToServer == nullptr)
      return PL_FAILURE;
  }

  return PL_SUCCESS;
}

void plRemoteInterfaceEnetImpl::InternalShutdownConnection()
{
  m_uiPort = 0;

  if (m_pEnetHost)
  {
    // send all peers that we are disconnecting
    for (plUInt32 i = (plUInt32)m_pEnetHost->connectedPeers; i > 0; --i)
      enet_peer_disconnect(&m_pEnetHost->peers[i - 1], 0);

    // process the network messages (e.g. send the disconnect messages)
    UpdateRemoteInterface();
    plThreadUtils::Sleep(plTime::MakeFromMilliseconds(10));
  }

  // finally close the network connection
  if (m_pEnetHost)
  {
    enet_host_destroy(m_pEnetHost);
    m_pEnetHost = nullptr;
  }

  // enet_deinitialize();
  m_pEnetConnectionToServer = nullptr;
}

plTime plRemoteInterfaceEnetImpl::InternalGetPingToServer()
{
  PL_ASSERT_DEV(m_pEnetConnectionToServer != nullptr, "Client has not connected to server");

  enet_peer_ping(m_pEnetConnectionToServer);
  return plTime::MakeFromMilliseconds(m_pEnetConnectionToServer->lastRoundTripTime);
}

plResult plRemoteInterfaceEnetImpl::InternalTransmit(plRemoteTransmitMode tm, const plArrayPtr<const plUInt8>& data)
{
  if (m_pEnetHost == nullptr)
    return PL_FAILURE;

  ENetPacket* pPacket = enet_packet_create(data.GetPtr(), data.GetCount(), (tm == plRemoteTransmitMode::Reliable) ? ENET_PACKET_FLAG_RELIABLE : 0);
  enet_host_broadcast(m_pEnetHost, 0, pPacket);

  return PL_SUCCESS;
}

void plRemoteInterfaceEnetImpl::InternalUpdateRemoteInterface()
{
  if (!m_pEnetHost)
    return;

  if (!m_bAllowNetworkUpdates)
    return;

  m_bAllowNetworkUpdates = false;
  PL_SCOPE_EXIT(m_bAllowNetworkUpdates = true);

  ENetEvent NetworkEvent;

  while (true)
  {
    const plInt32 iStatus = enet_host_service(m_pEnetHost, &NetworkEvent, 0);

    if (iStatus <= 0)
      return;

    switch (NetworkEvent.type)
    {
      case ENET_EVENT_TYPE_CONNECT:
      {
        if ((GetRemoteMode() == plRemoteMode::Server) && (NetworkEvent.peer->eventData != GetConnectionToken()))
        {
          // do not accept connections that don't have the correct password
          enet_peer_disconnect(NetworkEvent.peer, 0);
          break;
        }

        // Uncomment this to allow stepping through enet code without loosing the connection.
        // enet_peer_timeout(NetworkEvent.peer, 0xFFFFFF, 32000, 0xFFFFFF);


        if (GetRemoteMode() == plRemoteMode::Client)
        {
          // Querying host IP and name can take a lot of time
          // Do not do this in the other case, as it may result in timeouts while establishing the connection.
          char szHostIP[64] = "<unknown>";
          // char szHostName[64] = "<unknown>";

          enet_address_get_host_ip(&NetworkEvent.peer->address, szHostIP, 63);
          // enet_address_get_host(&NetworkEvent.peer->address, szHostName, 63);

          m_sServerInfoIP = szHostIP;
          // m_ServerInfoName = szHostName;

          // now we are waiting for the server to send its ID
        }
        else
        {
          const plUInt32 uiAppID = GetApplicationID();
          Send(plRemoteTransmitMode::Reliable, GetConnectionToken(), 'PLID',
            plArrayPtr<const plUInt8>(reinterpret_cast<const plUInt8*>(&uiAppID), sizeof(plUInt32)));

          // then wait for its acknowledgment message
        }
      }
      break;

      case ENET_EVENT_TYPE_DISCONNECT:
      {
        if (GetRemoteMode() == plRemoteMode::Client)
        {
          ReportDisconnectedFromServer();
        }
        else
        {
          auto it = m_EnetPeerToClientID.Find(NetworkEvent.peer);
          if (it.IsValid())
          {
            ReportDisconnectedFromClient(it.Value());
            m_EnetPeerToClientID.Remove(it);
          }
        }
      }
      break;

      case ENET_EVENT_TYPE_RECEIVE:
      {
        const plUInt32 uiApplicationID = *((plUInt32*)&NetworkEvent.packet->data[0]);
        const plUInt32 uiSystemID = *((plUInt32*)&NetworkEvent.packet->data[4]);
        const plUInt32 uiMsgID = *((plUInt32*)&NetworkEvent.packet->data[8]);
        const plUInt8* pData = &NetworkEvent.packet->data[12];

        if (uiSystemID == GetConnectionToken())
        {
          switch (uiMsgID)
          {
            case 'PLID':
            {
              // acknowledge that the ID has been received
              Send(GetConnectionToken(), 'AKID');

              // go tell the others about it
              plUInt32 uiServerID = *((plUInt32*)pData);
              ReportConnectionToServer(uiServerID);
            }
            break;

            case 'AKID':
            {
              if (m_EnetPeerToClientID[NetworkEvent.peer] != uiApplicationID)
              {
                m_EnetPeerToClientID[NetworkEvent.peer] = uiApplicationID;

                // the client received the server ID -> the connection has been established properly
                ReportConnectionToClient(uiApplicationID);
              }
            }
            break;
          }
        }
        else
        {
          ReportMessage(uiApplicationID, uiSystemID, uiMsgID, plArrayPtr<const plUInt8>(pData, (plUInt32)NetworkEvent.packet->dataLength - 12));
        }

        enet_packet_destroy(NetworkEvent.packet);
      }
      break;

      default:
        break;
    }
  }
}

#endif


