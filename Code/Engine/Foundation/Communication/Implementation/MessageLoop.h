#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>

class plProcessMessage;
class plIpcChannel;
class plLoopThread;

/// \brief Internal sub-system used by plIpcChannel.
///
/// This sub-system creates a background thread as soon as the first plIpcChannel
/// is added to it. This class should never be needed to be accessed outside
/// of plIpcChannel implementations.
class PL_FOUNDATION_DLL plMessageLoop
{
  PL_DECLARE_SINGLETON(plMessageLoop);

public:
  plMessageLoop();
  virtual ~plMessageLoop() = default;
  ;

  /// \brief Needs to be called by newly created channels' constructors.
  void AddChannel(plIpcChannel* pChannel);

  void RemoveChannel(plIpcChannel* pChannel);

protected:
  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, MessageLoop);
  friend class plLoopThread;
  friend class plIpcChannel;

  void StartUpdateThread();
  void StopUpdateThread();
  void RunLoop();
  bool ProcessTasks();
  void Quit();

  /// \brief Wake up the message loop when new work comes in.
  virtual void WakeUp() = 0;
  /// \brief Waits until a new message has been processed (sent, received).
  /// \param timeout If negative, wait indefinitely.
  /// \param pFilter If not null, wait for a message for the specific channel.
  /// \return Returns whether a message was received or the timeout was reached.
  virtual bool WaitForMessages(plInt32 iTimeout, plIpcChannel* pFilter) = 0;

  plThreadID m_ThreadId = 0;
  mutable plMutex m_Mutex;
  bool m_bShouldQuit = false;
  bool m_bCallTickFunction = false;
  class plLoopThread* m_pUpdateThread = nullptr;

  plMutex m_TasksMutex;
  plDynamicArray<plIpcChannel*> m_ConnectQueue;
  plDynamicArray<plIpcChannel*> m_DisconnectQueue;
  plDynamicArray<plIpcChannel*> m_SendQueue;

  // Thread local copies of the different queues for the ProcessTasks method
  plDynamicArray<plIpcChannel*> m_ConnectQueueTask;
  plDynamicArray<plIpcChannel*> m_DisconnectQueueTask;
  plDynamicArray<plIpcChannel*> m_SendQueueTask;

  plDynamicArray<plIpcChannel*> m_AllAddedChannels;
};
