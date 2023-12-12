#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/IpcChannelEnet.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

#  include <Foundation/Communication/Implementation/MessageLoop.h>
#  include <Foundation/Communication/RemoteInterfaceEnet.h>
#  include <Foundation/Communication/RemoteMessage.h>
#  include <Foundation/Logging/Log.h>

plIpcChannelEnet::plIpcChannelEnet(plStringView sAddress, Mode::Enum mode)
  : plIpcChannel(sAddress, mode)
  , m_sAddress(sAddress)
{
  m_pNetwork = plRemoteInterfaceEnet::Make();
  m_pNetwork->SetMessageHandler(0, plMakeDelegate(&plIpcChannelEnet::NetworkMessageHandler, this));
  m_pNetwork->m_RemoteEvents.AddEventHandler(plMakeDelegate(&plIpcChannelEnet::EnetEventHandler, this));

  m_pOwner->AddChannel(this);
}

plIpcChannelEnet::~plIpcChannelEnet()
{
  m_pNetwork->ShutdownConnection();

  m_pOwner->RemoveChannel(this);
}

void plIpcChannelEnet::InternalConnect()
{
  if (m_Mode == Mode::Server)
  {
    m_pNetwork->StartServer('RMOT', m_sAddress, false).IgnoreResult();
  }
  else
  {
    if ((m_sLastAddress != m_sAddress) || (plTime::Now() - m_LastConnectAttempt > plTime::Seconds(10)))
    {
      m_sLastAddress = m_sAddress;
      m_LastConnectAttempt = plTime::Now();
      m_pNetwork->ConnectToServer('RMOT', m_sAddress, false).IgnoreResult();
    }

    m_pNetwork->WaitForConnectionToServer(plTime::Milliseconds(10.0)).IgnoreResult();
  }

  m_bConnected = m_pNetwork->IsConnectedToOther() ? 1 : 0;
}

void plIpcChannelEnet::InternalDisconnect()
{
  m_pNetwork->ShutdownConnection();
  m_pNetwork->m_RemoteEvents.RemoveEventHandler(plMakeDelegate(&plIpcChannelEnet::EnetEventHandler, this));

  m_bConnected = 0;
}

void plIpcChannelEnet::InternalSend()
{
  {
    PLASMA_LOCK(m_OutputQueueMutex);

    while (!m_OutputQueue.IsEmpty())
    {
      plContiguousMemoryStreamStorage& storage = m_OutputQueue.PeekFront();

      m_pNetwork->Send(plRemoteTransmitMode::Reliable, 0, 0, storage);

      m_OutputQueue.PopFront();
    }
  }

  m_pNetwork->UpdateRemoteInterface();
}

bool plIpcChannelEnet::NeedWakeup() const
{
  return true;
}

void plIpcChannelEnet::Tick()
{
  m_pNetwork->UpdateRemoteInterface();

  m_bConnected = m_pNetwork->IsConnectedToOther() ? 1 : 0;

  m_pNetwork->ExecuteAllMessageHandlers();
}

void plIpcChannelEnet::NetworkMessageHandler(plRemoteMessage& msg)
{
  ReceiveMessageData(msg.GetMessageData());
}

void plIpcChannelEnet::EnetEventHandler(const plRemoteEvent& e)
{
  if (e.m_Type == plRemoteEvent::DisconnectedFromServer)
  {
    plLog::Info("Disconnected from remote engine process.");
    Disconnect();
  }

  if (e.m_Type == plRemoteEvent::ConnectedToServer)
  {
    plLog::Info("Connected to remote engine process.");
  }
}

#endif



PLASMA_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_IpcChannelEnet);
