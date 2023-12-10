#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/RemoteMessage.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcessMessage, 1, plRTTIDefaultAllocator<plProcessMessage>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRemoteMessage::plRemoteMessage()
  : m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
}

plRemoteMessage::plRemoteMessage(const plRemoteMessage& rhs)
  : m_Storage(rhs.m_Storage)
  , m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID = rhs.m_uiMsgID;
}


plRemoteMessage::plRemoteMessage(plUInt32 uiSystemID, plUInt32 uiMessageID)
  : m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
  m_uiSystemID = uiSystemID;
  m_uiMsgID = uiMessageID;
}

void plRemoteMessage::operator=(const plRemoteMessage& rhs)
{
  m_Storage = rhs.m_Storage;
  m_uiApplicationID = rhs.m_uiApplicationID;
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID = rhs.m_uiMsgID;
  m_Reader.SetStorage(&m_Storage);
  m_Writer.SetStorage(&m_Storage);
}

plRemoteMessage::~plRemoteMessage()
{
  m_Reader.SetStorage(nullptr);
  m_Writer.SetStorage(nullptr);
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_RemoteMessage);
