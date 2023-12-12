#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>

plProcessCommunicationChannel::plProcessCommunicationChannel() {}

plProcessCommunicationChannel::~plProcessCommunicationChannel()
{
  if (m_pChannel)
  {
    PLASMA_DEFAULT_DELETE(m_pChannel);
  }
}

void plProcessCommunicationChannel::SendMessage(plProcessMessage* pMessage)
{
  if (m_pFirstAllowedMessageType != nullptr)
  {
    // ignore all messages that are not the first allowed message
    // this is necessary to make sure that during an engine restart we don't accidentally send stray messages while
    // the engine is not yet correctly set up
    if (!pMessage->GetDynamicRTTI()->IsDerivedFrom(m_pFirstAllowedMessageType))
      return;

    m_pFirstAllowedMessageType = nullptr;
  }

  {
    if (m_pChannel == nullptr)
      return;

    m_pChannel->Send(pMessage);
  }
}

bool plProcessCommunicationChannel::ProcessMessages()
{
  if (!m_pChannel)
    return false;

  return m_pChannel->ProcessMessages();
}


void plProcessCommunicationChannel::WaitForMessages()
{
  if (!m_pChannel)
    return;

  m_pChannel->WaitForMessages();
}

void plProcessCommunicationChannel::MessageFunc(const plProcessMessage* msg)
{
  const plRTTI* pRtti = msg->GetDynamicRTTI();

  if (m_pWaitForMessageType != nullptr && msg->GetDynamicRTTI()->IsDerivedFrom(m_pWaitForMessageType))
  {
    if (m_WaitForMessageCallback.IsValid())
    {
      if (m_WaitForMessageCallback(const_cast<plProcessMessage*>(msg)))
      {
        m_WaitForMessageCallback = WaitForMessageCallback();
        m_pWaitForMessageType = nullptr;
      }
    }
    else
    {
      m_pWaitForMessageType = nullptr;
    }
  }

  PLASMA_ASSERT_DEV(pRtti != nullptr, "Message Type unknown");
  PLASMA_ASSERT_DEV(msg != nullptr, "Object could not be allocated");
  PLASMA_ASSERT_DEV(pRtti->IsDerivedFrom<plProcessMessage>(), "Msg base type is invalid");

  Event e;
  e.m_pMessage = msg;
  m_Events.Broadcast(e);
}

plResult plProcessCommunicationChannel::WaitForMessage(const plRTTI* pMessageType, plTime tTimeout, WaitForMessageCallback* pMessageCallack)
{
  PLASMA_ASSERT_DEV(m_pChannel != nullptr, "Need to connect first before waiting for a message.");
  // PLASMA_ASSERT_DEV(plThreadUtils::IsMainThread(), "This function is not thread safe");
  PLASMA_ASSERT_DEV(m_pWaitForMessageType == nullptr, "Already waiting for another message!");

  m_pWaitForMessageType = pMessageType;
  if (pMessageCallack)
  {
    m_WaitForMessageCallback = *pMessageCallack;
  }
  else
  {
    m_WaitForMessageCallback = WaitForMessageCallback();
  }

  PLASMA_SCOPE_EXIT(m_WaitForMessageCallback = WaitForMessageCallback(););

  const plTime tStart = plTime::Now();

  while (m_pWaitForMessageType != nullptr)
  {
    if (tTimeout == plTime())
    {
      m_pChannel->WaitForMessages();
    }
    else
    {
      plTime tTimeLeft = tTimeout - (plTime::Now() - tStart);

      if (tTimeLeft < plTime::Zero())
      {
        m_pWaitForMessageType = nullptr;
        plLog::Dev("Reached time-out of {0} seconds while waiting for {1}", plArgF(tTimeout.GetSeconds(), 1), pMessageType->GetTypeName());
        return PLASMA_FAILURE;
      }

      m_pChannel->WaitForMessages(tTimeLeft).IgnoreResult();
    }

    if (!m_pChannel->IsConnected())
    {
      m_pWaitForMessageType = nullptr;
      plLog::Dev("Lost connection while waiting for {}", pMessageType->GetTypeName());
      return PLASMA_FAILURE;
    }
  }

  return PLASMA_SUCCESS;
}

plResult plProcessCommunicationChannel::WaitForConnection(plTime tTimeout)
{
  if (m_pChannel->IsConnected())
  {
    return PLASMA_SUCCESS;
  }

  plThreadSignal waitForConnectionSignal;

  plEventSubscriptionID eventSubscriptionId = m_pChannel->m_Events.AddEventHandler([&](const plIpcChannelEvent& event) {
    switch (event.m_Type)
    {
      case plIpcChannelEvent::ConnectedToClient:
      case plIpcChannelEvent::ConnectedToServer:
      case plIpcChannelEvent::DisconnectedFromClient:
      case plIpcChannelEvent::DisconnectedFromServer:
        waitForConnectionSignal.RaiseSignal();
        break;
      default:
        break;
    }
  });

  PLASMA_SCOPE_EXIT(m_pChannel->m_Events.RemoveEventHandler(eventSubscriptionId));

  if (m_pChannel->IsConnected())
  {
    return PLASMA_SUCCESS;
  }

  if (tTimeout == plTime())
  {
    waitForConnectionSignal.WaitForSignal();
  }
  else
  {
    if (waitForConnectionSignal.WaitForSignal(tTimeout) == plThreadSignal::WaitResult::Timeout)
    {
      return PLASMA_FAILURE;
    }
  }

  return m_pChannel->IsConnected() ? PLASMA_SUCCESS : PLASMA_FAILURE;
}