#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/ThreadWithDispatcher.h>

plThreadWithDispatcher::plThreadWithDispatcher(const char* szName /*= "plThreadWithDispatcher"*/, plUInt32 uiStackSize /*= 128 * 1024*/)
  : plThread(szName, uiStackSize)
{
}

plThreadWithDispatcher::~plThreadWithDispatcher() = default;

void plThreadWithDispatcher::Dispatch(DispatchFunction&& delegate)
{
  PL_LOCK(m_QueueMutex);
  m_ActiveQueue.PushBack(std::move(delegate));
}

void plThreadWithDispatcher::DispatchQueue()
{
  {
    PL_LOCK(m_QueueMutex);
    std::swap(m_ActiveQueue, m_CurrentlyBeingDispatchedQueue);
  }

  for (const auto& pDelegate : m_CurrentlyBeingDispatchedQueue)
  {
    pDelegate();
  }

  m_CurrentlyBeingDispatchedQueue.Clear();
}


