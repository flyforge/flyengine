#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/IpcChannelEnet.h>
#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Communication/Implementation/Win/PipeChannel_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
#  include <Foundation/Communication/Implementation/Linux/PipeChannel_linux.h>
#endif

plIpcChannel::plIpcChannel(plStringView sAddress, Mode::Enum mode)
  : m_Mode(mode)
  , m_pOwner(plMessageLoop::GetSingleton())
{
}

plIpcChannel::~plIpcChannel()
{
  plDeque<plUniquePtr<plProcessMessage>> messages;
  SwapWorkQueue(messages);
  messages.Clear();

  m_pOwner->RemoveChannel(this);
}

plIpcChannel* plIpcChannel::CreatePipeChannel(plStringView sAddress, Mode::Enum mode)
{
  if (sAddress.IsEmpty() || sAddress.GetElementCount() > 200)
  {
    plLog::Error("Failed co create pipe '{0}', name is not valid", sAddress);
    return nullptr;
  }

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  return PLASMA_DEFAULT_NEW(plPipeChannel_win, sAddress, mode);
#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
  return PLASMA_DEFAULT_NEW(plPipeChannel_linux, sAddress, mode);
#else
  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
#endif
}


plIpcChannel* plIpcChannel::CreateNetworkChannel(plStringView sAddress, Mode::Enum mode)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  return PLASMA_DEFAULT_NEW(plIpcChannelEnet, sAddress, mode);
#else
  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
#endif
}

void plIpcChannel::Connect()
{
  PLASMA_LOCK(m_pOwner->m_TasksMutex);
  m_pOwner->m_ConnectQueue.PushBack(this);
  m_pOwner->WakeUp();
}


void plIpcChannel::Disconnect()
{
  PLASMA_LOCK(m_pOwner->m_TasksMutex);
  m_pOwner->m_DisconnectQueue.PushBack(this);
  m_pOwner->WakeUp();
}

bool plIpcChannel::Send(plProcessMessage* pMsg)
{
  {
    PLASMA_LOCK(m_OutputQueueMutex);
    plMemoryStreamStorageInterface& storage = m_OutputQueue.ExpandAndGetRef();
    plMemoryStreamWriter writer(&storage);
    plUInt32 uiSize = 0;
    plUInt32 uiMagic = MAGIC_VALUE;
    writer << uiMagic;
    writer << uiSize;
    PLASMA_ASSERT_DEBUG(storage.GetStorageSize32() == HEADER_SIZE, "Magic value and size should have written HEADER_SIZE bytes.");
    plReflectionSerializer::WriteObjectToBinary(writer, pMsg->GetDynamicRTTI(), pMsg);

    // reset to the beginning and write the stored size again
    writer.SetWritePosition(4);
    writer << storage.GetStorageSize32();
  }
  if (m_bConnected)
  {
    if (NeedWakeup())
    {
      PLASMA_LOCK(m_pOwner->m_TasksMutex);
      if (!m_pOwner->m_SendQueue.Contains(this))
        m_pOwner->m_SendQueue.PushBack(this);
      m_pOwner->WakeUp();
      return true;
    }
  }
  return false;
}

bool plIpcChannel::ProcessMessages()
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

void plIpcChannel::WaitForMessages()
{
  if (m_bConnected)
  {
    m_IncomingMessages.WaitForSignal();
    ProcessMessages();
  }
}

plResult plIpcChannel::WaitForMessages(plTime timeout)
{
  if (m_bConnected)
  {
    if (m_IncomingMessages.WaitForSignal(timeout) == plThreadSignal::WaitResult::Timeout)
    {
      return PLASMA_FAILURE;
    }
    ProcessMessages();
  }

  return PLASMA_SUCCESS;
}

void plIpcChannel::ReceiveMessageData(plArrayPtr<const plUInt8> data)
{
  plArrayPtr<const plUInt8> remainingData = data;
  while (true)
  {
    if (m_MessageAccumulator.GetCount() < HEADER_SIZE)
    {
      if (remainingData.GetCount() + m_MessageAccumulator.GetCount() < HEADER_SIZE)
      {
        m_MessageAccumulator.PushBackRange(remainingData);
        return;
      }
      else
      {
        plUInt32 uiRemainingHeaderData = HEADER_SIZE - m_MessageAccumulator.GetCount();
        plArrayPtr<const plUInt8> headerData = remainingData.GetSubArray(0, uiRemainingHeaderData);
        m_MessageAccumulator.PushBackRange(headerData);
        PLASMA_ASSERT_DEBUG(m_MessageAccumulator.GetCount() == HEADER_SIZE, "We should have a full header now.");
        remainingData = remainingData.GetSubArray(uiRemainingHeaderData);
      }
    }

    PLASMA_ASSERT_DEBUG(m_MessageAccumulator.GetCount() >= HEADER_SIZE, "Header must be complete at this point.");
    if (remainingData.IsEmpty())
      return;

    // Read and verify header
    plUInt32 uiMagic = *reinterpret_cast<const plUInt32*>(m_MessageAccumulator.GetData());
    PLASMA_IGNORE_UNUSED(uiMagic);
    PLASMA_ASSERT_DEBUG(uiMagic == MAGIC_VALUE, "Message received with wrong magic value.");
    plUInt32 uiMessageSize = *reinterpret_cast<const plUInt32*>(m_MessageAccumulator.GetData() + 4);
    PLASMA_ASSERT_DEBUG(uiMessageSize < MAX_MESSAGE_SIZE, "Message too big: {0}! Either the stream got corrupted or you need to increase MAX_MESSAGE_SIZE.", uiMessageSize);
    if (uiMessageSize > remainingData.GetCount() + m_MessageAccumulator.GetCount())
    {
      m_MessageAccumulator.PushBackRange(remainingData);
      return;
    }

    // Write missing data into message accumulator
    plUInt32 remainingMessageData = uiMessageSize - m_MessageAccumulator.GetCount();
    plArrayPtr<const plUInt8> messageData = remainingData.GetSubArray(0, remainingMessageData);
    m_MessageAccumulator.PushBackRange(messageData);
    PLASMA_ASSERT_DEBUG(m_MessageAccumulator.GetCount() == uiMessageSize, "");
    remainingData = remainingData.GetSubArray(remainingMessageData);

    {
      // Message complete, de-serialize
      plRawMemoryStreamReader reader(m_MessageAccumulator.GetData() + HEADER_SIZE, uiMessageSize - HEADER_SIZE);
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
      m_MessageAccumulator.Clear();
    }
  }
}

void plIpcChannel::EnqueueMessage(plUniquePtr<plProcessMessage>&& msg)
{
  {
    PLASMA_LOCK(m_IncomingQueueMutex);
    m_IncomingQueue.PushBack(std::move(msg));
  }
  m_IncomingMessages.RaiseSignal();

  m_Events.Broadcast(plIpcChannelEvent(plIpcChannelEvent::NewMessages, this));
}

void plIpcChannel::SwapWorkQueue(plDeque<plUniquePtr<plProcessMessage>>& messages)
{
  PLASMA_ASSERT_DEBUG(messages.IsEmpty(), "Swap target must be empty!");
  PLASMA_LOCK(m_IncomingQueueMutex);
  if (m_IncomingQueue.IsEmpty())
    return;
  messages.Swap(m_IncomingQueue);
}

void plIpcChannel::FlushPendingOperations()
{
  m_pOwner->WaitForMessages(-1, this);
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_IpcChannel);
