#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Delegate.h>

class plIpcChannel;
class plProcessMessage;

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plProcessCommunicationChannel
{
public:
  plProcessCommunicationChannel();
  ~plProcessCommunicationChannel();

  void SendMessage(plProcessMessage* pMessage);

  /// /brief Callback for 'wait for...' functions. If true is returned, the message is accepted to match the wait criteria and
  ///        the waiting ends. If false is returned the wait for the message continues.
  typedef plDelegate<bool(plProcessMessage*)> WaitForMessageCallback;
  plResult WaitForMessage(const plRTTI* pMessageType, plTime tTimeout, WaitForMessageCallback* pMessageCallack = nullptr);
  plResult WaitForConnection(plTime tTimeout);

  /// \brief Returns true if any message was processed
  bool ProcessMessages();
  void WaitForMessages();

  struct Event
  {
    const plProcessMessage* m_pMessage;
  };

  plEvent<const Event&> m_Events;

  void MessageFunc(const plProcessMessage* msg);

protected:
  plIpcChannel* m_pChannel = nullptr;
  const plRTTI* m_pFirstAllowedMessageType = nullptr;

private:
  WaitForMessageCallback m_WaitForMessageCallback;
  const plRTTI* m_pWaitForMessageType = nullptr;
};
