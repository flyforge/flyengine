#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Profiling/Profiling.h>

void plTelemetry::QueueOutgoingMessage(TransmitMode tm, plUInt32 uiSystemID, plUInt32 uiMsgID, const void* pData, plUInt32 uiDataBytes)
{
  // unreliable packages can just be dropped
  if (tm == plTelemetry::Unreliable)
    return;

  PL_LOCK(GetTelemetryMutex());

  // add a new message to the queue
  MessageQueue& Queue = s_SystemMessages[uiSystemID];
  Queue.m_OutgoingQueue.PushBack();

  // and fill it out properly
  plTelemetryMessage& msg = Queue.m_OutgoingQueue.PeekBack();
  msg.SetMessageID(uiSystemID, uiMsgID);

  if (uiDataBytes > 0)
  {
    msg.GetWriter().WriteBytes(pData, uiDataBytes).IgnoreResult();
  }

  // if our outgoing queue has grown too large, dismiss older messages
  if (Queue.m_OutgoingQueue.GetCount() > Queue.m_uiMaxQueuedOutgoing)
    Queue.m_OutgoingQueue.PopFront(Queue.m_OutgoingQueue.GetCount() - Queue.m_uiMaxQueuedOutgoing);
}

void plTelemetry::FlushOutgoingQueues()
{
  static bool bRecursion = false;

  if (bRecursion)
    return;

  // if there is no connection to anyone (yet), don't do anything
  if (!IsConnectedToOther())
    return;

  bRecursion = true;

  PL_LOCK(GetTelemetryMutex());

  // go through all system types
  for (auto it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_OutgoingQueue.IsEmpty())
      continue;

    const plUInt32 uiCurCount = it.Value().m_OutgoingQueue.GetCount();

    // send all messages that are queued for this system
    for (plUInt32 i = 0; i < uiCurCount; ++i)
      Send(plTelemetry::Reliable, it.Value().m_OutgoingQueue[i]); // Send() will already update the network

    // check that they have not been queue again
    PL_ASSERT_DEV(it.Value().m_OutgoingQueue.GetCount() == uiCurCount, "Implementation Error: When queued messages are flushed, they should not get queued again.");

    it.Value().m_OutgoingQueue.Clear();
  }

  bRecursion = false;
}


plResult plTelemetry::ConnectToServer(plStringView sConnectTo)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  return OpenConnection(Client, sConnectTo);
#else
  plLog::SeriousWarning("Enet is not compiled into this build, plTelemetry::ConnectToServer() will be ignored.");
  return PL_FAILURE;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void plTelemetry::CreateServer()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (OpenConnection(Server).Failed())
  {
    plLog::Error("plTelemetry: Failed to open a connection as a server.");
    s_ConnectionMode = ConnectionMode::None;
  }
#else
  plLog::SeriousWarning("Enet is not compiled into this build, plTelemetry::CreateServer() will be ignored.");
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void plTelemetry::AcceptMessagesForSystem(plUInt32 uiSystemID, bool bAccept, ProcessMessagesCallback callback, void* pPassThrough)
{
  PL_LOCK(GetTelemetryMutex());

  s_SystemMessages[uiSystemID].m_bAcceptMessages = bAccept;
  s_SystemMessages[uiSystemID].m_Callback = callback;
  s_SystemMessages[uiSystemID].m_pPassThrough = pPassThrough;
}

void plTelemetry::PerFrameUpdate()
{
  PL_PROFILE_SCOPE("Telemetry.PerFrameUpdate");
  PL_LOCK(GetTelemetryMutex());

  // Call each callback to process the incoming messages
  for (auto it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    if (!it.Value().m_IncomingQueue.IsEmpty() && it.Value().m_Callback)
      it.Value().m_Callback(it.Value().m_pPassThrough);
  }

  TelemetryEventData e;
  e.m_EventType = TelemetryEventData::PerFrameUpdate;

  const bool bAllowUpdate = s_bAllowNetworkUpdate;
  s_bAllowNetworkUpdate = false;
  s_TelemetryEvents.Broadcast(e);
  s_bAllowNetworkUpdate = bAllowUpdate;
}

void plTelemetry::SetOutgoingQueueSize(plUInt32 uiSystemID, plUInt16 uiMaxQueued)
{
  PL_LOCK(GetTelemetryMutex());

  s_SystemMessages[uiSystemID].m_uiMaxQueuedOutgoing = uiMaxQueued;
}


bool plTelemetry::IsConnectedToOther()
{
  return ((s_ConnectionMode == Client && IsConnectedToServer()) || (s_ConnectionMode == Server && IsConnectedToClient()));
}

void plTelemetry::Broadcast(TransmitMode tm, plUInt32 uiSystemID, plUInt32 uiMsgID, const void* pData, plUInt32 uiDataBytes)
{
  if (s_ConnectionMode != plTelemetry::Server)
    return;

  Send(tm, uiSystemID, uiMsgID, pData, uiDataBytes);
}

void plTelemetry::Broadcast(TransmitMode tm, plUInt32 uiSystemID, plUInt32 uiMsgID, plStreamReader& inout_stream, plInt32 iDataBytes)
{
  if (s_ConnectionMode != plTelemetry::Server)
    return;

  Send(tm, uiSystemID, uiMsgID, inout_stream, iDataBytes);
}

void plTelemetry::Broadcast(TransmitMode tm, plTelemetryMessage& ref_msg)
{
  if (s_ConnectionMode != plTelemetry::Server)
    return;

  Send(tm, ref_msg);
}

void plTelemetry::SendToServer(plUInt32 uiSystemID, plUInt32 uiMsgID, const void* pData, plUInt32 uiDataBytes)
{
  if (s_ConnectionMode != plTelemetry::Client)
    return;

  Send(plTelemetry::Reliable, uiSystemID, uiMsgID, pData, uiDataBytes);
}

void plTelemetry::SendToServer(plUInt32 uiSystemID, plUInt32 uiMsgID, plStreamReader& inout_stream, plInt32 iDataBytes)
{
  if (s_ConnectionMode != plTelemetry::Client)
    return;

  Send(plTelemetry::Reliable, uiSystemID, uiMsgID, inout_stream, iDataBytes);
}

void plTelemetry::SendToServer(plTelemetryMessage& ref_msg)
{
  if (s_ConnectionMode != plTelemetry::Client)
    return;

  Send(plTelemetry::Reliable, ref_msg);
}

void plTelemetry::Send(TransmitMode tm, plTelemetryMessage& msg)
{
  Send(tm, msg.GetSystemID(), msg.GetMessageID(), msg.GetReader(), (plInt32)msg.m_Storage.GetStorageSize32());
}


