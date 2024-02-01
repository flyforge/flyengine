#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Configuration/Startup.h>

PL_IMPLEMENT_SINGLETON(plMessageLoop);

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Platform/Win/MessageLoop_Win.h>
#elif PL_ENABLED(PL_PLATFORM_LINUX)
#  include <Foundation/Platform/Linux/MessageLoop_Linux.h>
#else
#  include <Foundation/Communication/Implementation/MessageLoop_Fallback.h>
#endif

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(Foundation, MessageLoop)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "TaskSystem",
    "ThreadUtils"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    if (plStartup::HasApplicationTag("NoMessageLoop"))
      return;

    #if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
      PL_DEFAULT_NEW(plMessageLoop_win);
    #elif PL_ENABLED(PL_PLATFORM_LINUX)
      PL_DEFAULT_NEW(plMessageLoop_linux);
    #else
      PL_DEFAULT_NEW(plMessageLoop_Fallback);
    #endif
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plMessageLoop* pDummy = plMessageLoop::GetSingleton();
    PL_DEFAULT_DELETE(pDummy);
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

class plLoopThread : public plThread
{
public:
  plLoopThread()
    : plThread("plMessageLoopThread")
  {
  }
  plMessageLoop* m_pRemoteInterface = nullptr;
  virtual plUInt32 Run() override
  {
    m_pRemoteInterface->RunLoop();
    return 0;
  }
};

plMessageLoop::plMessageLoop()
  : m_SingletonRegistrar(this)
{
}

void plMessageLoop::StartUpdateThread()
{
  PL_LOCK(m_Mutex);
  if (m_pUpdateThread == nullptr)
  {
    m_pUpdateThread = PL_DEFAULT_NEW(plLoopThread);
    m_pUpdateThread->m_pRemoteInterface = this;
    m_pUpdateThread->Start();
  }
}

void plMessageLoop::StopUpdateThread()
{
  PL_LOCK(m_Mutex);
  if (m_pUpdateThread != nullptr)
  {
    m_bShouldQuit = true;
    WakeUp();
    m_pUpdateThread->Join();

    PL_DEFAULT_DELETE(m_pUpdateThread);
  }
}

void plMessageLoop::RunLoop()
{
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  m_ThreadId = plThreadUtils::GetCurrentThreadID();
#endif

  while (true)
  {
    if (m_bCallTickFunction)
    {
      for (plIpcChannel* pChannel : m_AllAddedChannels)
      {
        if (pChannel->RequiresRegularTick())
        {
          pChannel->Tick();
        }
      }
    }

    // process all available data until all is processed and we wait for new messages.
    bool didwork = ProcessTasks();
    if (m_bShouldQuit)
      break;

    if (didwork)
      continue;

    didwork |= WaitForMessages(0, nullptr);
    if (m_bShouldQuit)
      break;

    if (didwork)
      continue;

    // wait until we have work again
    WaitForMessages(m_bCallTickFunction ? 50 : -1, nullptr); // timeout 20 times per second, if we need to call the tick function
  }
}

bool plMessageLoop::ProcessTasks()
{
  {
    PL_LOCK(m_TasksMutex);
    // Swap out the queues under the lock so we can process them without holding the lock
    m_ConnectQueueTask.Swap(m_ConnectQueue);
    m_SendQueueTask.Swap(m_SendQueue);
    m_DisconnectQueueTask.Swap(m_DisconnectQueue);
  }

  for (plIpcChannel* pChannel : m_ConnectQueueTask)
  {
    pChannel->InternalConnect();
  }
  for (plIpcChannel* pChannel : m_SendQueueTask)
  {
    pChannel->InternalSend();
  }
  for (plIpcChannel* pChannel : m_DisconnectQueueTask)
  {
    pChannel->InternalDisconnect();
  }

  bool bDidWork = !m_ConnectQueueTask.IsEmpty() || !m_SendQueueTask.IsEmpty() || !m_DisconnectQueueTask.IsEmpty();
  m_ConnectQueueTask.Clear();
  m_SendQueueTask.Clear();
  m_DisconnectQueueTask.Clear();
  return bDidWork;
}

void plMessageLoop::Quit()
{
  m_bShouldQuit = true;
}

void plMessageLoop::AddChannel(plIpcChannel* pChannel)
{
  {
    PL_LOCK(m_TasksMutex);
    m_AllAddedChannels.PushBack(pChannel);

    m_bCallTickFunction = false;
    for (auto pThisChannel : m_AllAddedChannels)
    {
      if (pThisChannel->RequiresRegularTick())
      {
        m_bCallTickFunction = true;
        break;
      }
    }
  }

  StartUpdateThread();
  pChannel->m_pOwner = this;
}

void plMessageLoop::RemoveChannel(plIpcChannel* pChannel)
{
  PL_LOCK(m_TasksMutex);

  m_AllAddedChannels.RemoveAndSwap(pChannel);
  m_ConnectQueue.RemoveAndSwap(pChannel);
  m_DisconnectQueue.RemoveAndSwap(pChannel);
  m_SendQueue.RemoveAndSwap(pChannel);
}

PL_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_MessageLoop);
