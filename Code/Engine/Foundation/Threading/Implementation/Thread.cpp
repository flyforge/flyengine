#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Thread.h>

plEvent<const plThreadEvent&, plMutex> plThread::s_ThreadEvents;

plThread::plThread(const char* szName /*= "plThread"*/, plUInt32 uiStackSize /*= 128 * 1024*/)
  : plOSThread(plThreadClassEntryPoint, this, szName, uiStackSize)
  , m_sName(szName)
{
  plThreadEvent e;
  e.m_pThread = this;
  e.m_Type = plThreadEvent::Type::ThreadCreated;
  plThread::s_ThreadEvents.Broadcast(e, 255);
}

plThread::~plThread()
{
  PL_ASSERT_DEV(!IsRunning(), "Thread deletion while still running detected!");

  plThreadEvent e;
  e.m_pThread = this;
  e.m_Type = plThreadEvent::Type::ThreadDestroyed;
  plThread::s_ThreadEvents.Broadcast(e, 255);
}

plUInt32 RunThread(plThread* pThread)
{
  if (pThread == nullptr)
    return 0;

  plProfilingSystem::SetThreadName(pThread->m_sName.GetData());

  {
    plThreadEvent e;
    e.m_pThread = pThread;
    e.m_Type = plThreadEvent::Type::StartingExecution;
    plThread::s_ThreadEvents.Broadcast(e, 255);
  }

  pThread->m_ThreadStatus = plThread::Running;

  // Run the worker thread function
  plUInt32 uiReturnCode = pThread->Run();

  {
    plThreadEvent e;
    e.m_pThread = pThread;
    e.m_Type = plThreadEvent::Type::FinishedExecution;
    plThread::s_ThreadEvents.Broadcast(e, 255);
  }

  pThread->m_ThreadStatus = plThread::Finished;

  plProfilingSystem::RemoveThread();

  return uiReturnCode;
}


