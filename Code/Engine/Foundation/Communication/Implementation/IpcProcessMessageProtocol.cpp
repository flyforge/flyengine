#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/IpcProcessMessageProtocol.h>
//#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
//#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

plIpcProcessMessageProtocol::plIpcProcessMessageProtocol(plIpcChannel* pChannel)
{
  m_pChannel = pChannel;
  m_pChannel->SetReceiveCallback(plMakeDelegate(&plIpcProcessMessageProtocol::ReceiveMessageData, this));
}

plIpcProcessMessageProtocol::~plIpcProcessMessageProtocol()
{
  m_pChannel->SetReceiveCallback({});

  plDeque<plUniquePtr<plProcessMessage>> messages;
  SwapWorkQueue(messages);
  messages.Clear();
}

bool plIpcProcessMessageProtocol::Send(plProcessMessage* pMsg)
{
  plContiguousMemoryStreamStorage storage;
  plMemoryStreamWriter writer(&storage);
  plReflectionSerializer::WriteObjectToBinary(writer, pMsg->GetDynamicRTTI(), pMsg);
  return m_pChannel->Send(plArrayPtr<const plUInt8>(storage.GetData(), storage.GetStorageSize32()));
}

bool plIpcProcessMessageProtocol::ProcessMessages()
{
  plDeque<plUniquePtr<plProcessMessage>> messages;
  SwapWorkQueue(messages);
  if (messages.IsEmpty())
  {
    return false;
  }

  while (!messages.IsEmpty())
  {
    plUniquePtr<plProcessMessage> msg = std::move(messages.PeekFront());
    messages.PopFront();
    m_MessageEvent.Broadcast(msg.Borrow());
  }

  return true;
}

plResult plIpcProcessMessageProtocol::WaitForMessages(plTime timeout)
{
  plResult res = m_pChannel->WaitForMessages(timeout);
  if (res.Succeeded())
  {
    ProcessMessages();
  }
  return res;
}

void plIpcProcessMessageProtocol::ReceiveMessageData(plArrayPtr<const plUInt8> data)
{
  // Message complete, de-serialize
  plRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  const plRTTI* pRtti = nullptr;

  plProcessMessage* pMsg = (plProcessMessage*)plReflectionSerializer::ReadObjectFromBinary(reader, pRtti);
  plUniquePtr<plProcessMessage> msg(pMsg, plFoundation::GetDefaultAllocator());
  if (msg != nullptr)
  {
    EnqueueMessage(std::move(msg));
  }
  else
  {
    plLog::Error("Channel received invalid Message!");
  }
}

void plIpcProcessMessageProtocol::EnqueueMessage(plUniquePtr<plProcessMessage>&& msg)
{
  PLASMA_LOCK(m_IncomingQueueMutex);
  m_IncomingQueue.PushBack(std::move(msg));
}

void plIpcProcessMessageProtocol::SwapWorkQueue(plDeque<plUniquePtr<plProcessMessage>>& messages)
{
  PLASMA_ASSERT_DEBUG(messages.IsEmpty(), "Swap target must be empty!");
  PLASMA_LOCK(m_IncomingQueueMutex);
  if (m_IncomingQueue.IsEmpty())
    return;
  messages.Swap(m_IncomingQueue);
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_IpcProcessMessageProtocol);
