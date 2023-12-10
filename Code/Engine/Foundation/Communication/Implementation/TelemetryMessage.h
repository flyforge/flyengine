#pragma once

#include <Foundation/IO/MemoryStream.h>

class PLASMA_FOUNDATION_DLL plTelemetryMessage
{
public:
  plTelemetryMessage();
  plTelemetryMessage(const plTelemetryMessage& rhs);
  ~plTelemetryMessage();

  void operator=(const plTelemetryMessage& rhs);

  PLASMA_ALWAYS_INLINE plStreamReader& GetReader() { return m_Reader; }
  PLASMA_ALWAYS_INLINE plStreamWriter& GetWriter() { return m_Writer; }

  PLASMA_ALWAYS_INLINE plUInt32 GetSystemID() const { return m_uiSystemID; }
  PLASMA_ALWAYS_INLINE plUInt32 GetMessageID() const { return m_uiMsgID; }

  PLASMA_ALWAYS_INLINE void SetMessageID(plUInt32 uiSystemID, plUInt32 uiMessageID)
  {
    m_uiSystemID = uiSystemID;
    m_uiMsgID = uiMessageID;
  }

  //plUInt64 GetMessageSize() const { return m_Storage.GetStorageSize64(); }

private:
  friend class plTelemetry;

  plUInt32 m_uiSystemID;
  plUInt32 m_uiMsgID;

  plContiguousMemoryStreamStorage m_Storage;
  plMemoryStreamReader m_Reader;
  plMemoryStreamWriter m_Writer;
};
