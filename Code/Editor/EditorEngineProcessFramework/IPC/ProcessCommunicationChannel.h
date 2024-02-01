#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

class plIpcChannel;
class plProcessMessage;
class plIpcProcessMessageProtocol;

class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plProcessCommunicationChannel
{
public:
  plProcessCommunicationChannel();
  ~plProcessCommunicationChannel();

  bool SendMessage(plProcessMessage* pMessage);

  /// /brief Callback for 'wait for...' functions. If true is returned, the message is accepted to match the wait criteria and
  ///        the waiting ends. If false is returned the wait for the message continues.
  using WaitForMessageCallback = plDelegate<bool(plProcessMessage*)>;
  plResult WaitForMessage(const plRTTI* pMessageType, plTime timeout, WaitForMessageCallback* pMessageCallack = nullptr);
  plResult WaitForConnection(plTime timeout);
  bool IsConnected() const;

  /// \brief Returns true if any message was processed
  bool ProcessMessages();
  void WaitForMessages();

  struct Event
  {
    const plProcessMessage* m_pMessage;
  };

  plEvent<const Event&> m_Events;

  void MessageFunc(const plProcessMessage* pMsg);

protected:
  plUniquePtr<plIpcProcessMessageProtocol> m_pProtocol;
  plUniquePtr<plIpcChannel> m_pChannel;
  const plRTTI* m_pFirstAllowedMessageType = nullptr;

private:
  WaitForMessageCallback m_WaitForMessageCallback;
  const plRTTI* m_pWaitForMessageType = nullptr;
};
