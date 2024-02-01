#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/Reflection.h>

/// \todo Add move semantics for plRemoteMessage

/// \brief Encapsulates all the data that is transmitted when sending or receiving a message with plRemoteInterface
class PL_FOUNDATION_DLL plRemoteMessage
{
public:
  plRemoteMessage();
  plRemoteMessage(plUInt32 uiSystemID, plUInt32 uiMessageID);
  plRemoteMessage(const plRemoteMessage& rhs);
  ~plRemoteMessage();
  void operator=(const plRemoteMessage& rhs);

  /// \name Sending
  ///@{

  /// \brief For setting the message IDs before sending it
  PL_ALWAYS_INLINE void SetMessageID(plUInt32 uiSystemID, plUInt32 uiMessageID)
  {
    m_uiSystemID = uiSystemID;
    m_uiMsgID = uiMessageID;
  }

  /// \brief Returns a stream writer to append data to the message
  PL_ALWAYS_INLINE plStreamWriter& GetWriter() { return m_Writer; }


  ///@}

  /// \name Receiving
  ///@{

  /// \brief Returns a stream reader for reading the message data
  PL_ALWAYS_INLINE plStreamReader& GetReader() { return m_Reader; }
  PL_ALWAYS_INLINE plUInt32 GetApplicationID() const { return m_uiApplicationID; }
  PL_ALWAYS_INLINE plUInt32 GetSystemID() const { return m_uiSystemID; }
  PL_ALWAYS_INLINE plUInt32 GetMessageID() const { return m_uiMsgID; }
  PL_ALWAYS_INLINE plArrayPtr<const plUInt8> GetMessageData() const
  {
    return {m_Storage.GetData(), m_Storage.GetStorageSize32()};
  }

  ///@}

private:
  friend class plRemoteInterface;

  plUInt32 m_uiApplicationID = 0;
  plUInt32 m_uiSystemID = 0;
  plUInt32 m_uiMsgID = 0;

  plContiguousMemoryStreamStorage m_Storage;
  plMemoryStreamReader m_Reader;
  plMemoryStreamWriter m_Writer;
};

/// \brief Base class for IPC messages transmitted by plIpcChannel.
class PL_FOUNDATION_DLL plProcessMessage : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plProcessMessage, plReflectedClass);

public:
  plProcessMessage() = default;
};
