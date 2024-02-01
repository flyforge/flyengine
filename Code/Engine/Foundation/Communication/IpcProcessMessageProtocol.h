#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Types/UniquePtr.h>

class plIpcChannel;
class plMessageLoop;


/// \brief A protocol around plIpcChannel to send reflected messages instead of byte array messages between client and server.
///
/// This wrapper class hooks into an existing plIpcChannel. The plIpcChannel is still responsible for all connection logic. This class merely provides a high-level messaging protocol via reflected messages derived from plProcessMessage.
/// Note that if this class is used, plIpcChannel::Send must not be called manually anymore, only use plIpcProcessMessageProtocol::Send.
/// Received messages are stored in a queue and must be flushed via calling ProcessMessages or WaitForMessages.
class PL_FOUNDATION_DLL plIpcProcessMessageProtocol
{
public:
  plIpcProcessMessageProtocol(plIpcChannel* pChannel);
  ~plIpcProcessMessageProtocol();

  /// \brief Sends a message. pMsg can be destroyed after the call.
  bool Send(plProcessMessage* pMsg);


  /// \brief Processes all pending messages by broadcasting m_MessageEvent. Not re-entrant.
  bool ProcessMessages();
  /// \brief Block and wait for new messages and call ProcessMessages.
  plResult WaitForMessages(plTime timeout = plTime::MakeZero());

public:
  plEvent<const plProcessMessage*> m_MessageEvent; ///< Will be sent from thread calling ProcessMessages or WaitForMessages.

private:
  void EnqueueMessage(plUniquePtr<plProcessMessage>&& msg);
  void SwapWorkQueue(plDeque<plUniquePtr<plProcessMessage>>& messages);
  void ReceiveMessageData(plArrayPtr<const plUInt8> data);

private:
  plIpcChannel* m_pChannel = nullptr;

  plMutex m_IncomingQueueMutex;
  plDeque<plUniquePtr<plProcessMessage>> m_IncomingQueue;
};
