#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

plMutex plTaskSystem::s_TaskSystemMutex;
plUniquePtr<plTaskSystemState> plTaskSystem::s_pState;
plUniquePtr<plTaskSystemThreadState> plTaskSystem::s_pThreadState;

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Foundation, TaskSystem)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ThreadUtils",
    "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plTaskSystem::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plTaskSystem::Shutdown();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

void plTaskSystem::Startup()
{
  s_pThreadState = PLASMA_DEFAULT_NEW(plTaskSystemThreadState);
  s_pState = PLASMA_DEFAULT_NEW(plTaskSystemState);

  tl_TaskWorkerInfo.m_WorkerType = plWorkerThreadType::MainThread;
  tl_TaskWorkerInfo.m_iWorkerIndex = 0;

  // initialize with the default number of worker threads
  SetWorkerThreadCount();
}

void plTaskSystem::Shutdown()
{
  StopWorkerThreads();

  s_pState.Clear();
  s_pThreadState.Clear();
}

void plTaskSystem::SetTargetFrameTime(plTime targetFrameTime)
{
  s_pState->m_TargetFrameTime = targetFrameTime;
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystem);
