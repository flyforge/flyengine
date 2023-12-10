#include <Core/CorePCH.h>

#include <Core/ActorSystem/ActorManager.h>
#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Input/InputManager.h>
#include <Core/Interfaces/FrameCaptureInterface.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Time/Timestamp.h>
#include <Texture/Image/Image.h>

plGameApplicationBase* plGameApplicationBase::s_pGameApplicationBaseInstance = nullptr;

plGameApplicationBase::plGameApplicationBase(plStringView sAppName)
  : plApplication(sAppName)
  , m_ConFunc_TakeScreenshot("TakeScreenshot", "()", plMakeDelegate(&plGameApplicationBase::TakeScreenshot, this))
  , m_ConFunc_CaptureFrame("CaptureFrame", "()", plMakeDelegate(&plGameApplicationBase::CaptureFrame, this))
{
  s_pGameApplicationBaseInstance = this;
}

plGameApplicationBase::~plGameApplicationBase()
{
  s_pGameApplicationBaseInstance = nullptr;
}

void AppendCurrentTimestamp(plStringBuilder& out_sString)
{
  const plDateTime dt = plDateTime::MakeFromTimestamp(plTimestamp::CurrentTimestamp());

  out_sString.AppendFormat("_{0}-{1}-{2}_{3}-{4}-{5}-{6}", dt.GetYear(), plArgU(dt.GetMonth(), 2, true), plArgU(dt.GetDay(), 2, true), plArgU(dt.GetHour(), 2, true), plArgU(dt.GetMinute(), 2, true), plArgU(dt.GetSecond(), 2, true), plArgU(dt.GetMicroseconds() / 1000, 3, true));
}

void plGameApplicationBase::TakeProfilingCapture()
{
  class WriteProfilingDataTask final : public plTask
  {
  public:
    plProfilingSystem::ProfilingData m_profilingData;

    WriteProfilingDataTask() = default;
    ~WriteProfilingDataTask() = default;

  private:
    virtual void Execute() override
    {
      plStringBuilder sPath(":appdata/Profiling/", plApplication::GetApplicationInstance()->GetApplicationName());
      AppendCurrentTimestamp(sPath);
      sPath.Append(".json");

      plFileWriter fileWriter;
      if (fileWriter.Open(sPath) == PLASMA_SUCCESS)
      {
        m_profilingData.Write(fileWriter).IgnoreResult();
        plLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
      }
      else
      {
        plLog::Error("Could not write profiling capture to '{0}'.", sPath);
      }
    }
  };

  plSharedPtr<WriteProfilingDataTask> pWriteProfilingDataTask = PLASMA_DEFAULT_NEW(WriteProfilingDataTask);
  pWriteProfilingDataTask->ConfigureTask("Write Profiling Data", plTaskNesting::Never);
  plProfilingSystem::Capture(pWriteProfilingDataTask->m_profilingData);

  plTaskSystem::StartSingleTask(pWriteProfilingDataTask, plTaskPriority::LongRunning);
}

//////////////////////////////////////////////////////////////////////////

void plGameApplicationBase::TakeScreenshot()
{
  m_bTakeScreenshot = true;
}

void plGameApplicationBase::StoreScreenshot(plImage&& image, plStringView sContext /*= {} */)
{
  class WriteFileTask final : public plTask
  {
  public:
    plImage m_Image;
    plStringBuilder m_sPath;

    WriteFileTask() = default;
    ~WriteFileTask() = default;

  private:
    virtual void Execute() override
    {
      // get rid of Alpha channel before saving
      m_Image.Convert(plImageFormat::R8G8B8_UNORM_SRGB).IgnoreResult();

      if (m_Image.SaveTo(m_sPath).Succeeded())
      {
        plLog::Info("Screenshot: '{0}'", m_sPath);
      }
    }
  };

  plSharedPtr<WriteFileTask> pWriteTask = PLASMA_DEFAULT_NEW(WriteFileTask);
  pWriteTask->ConfigureTask("Write Screenshot", plTaskNesting::Never);
  pWriteTask->m_Image.ResetAndMove(std::move(image));

  pWriteTask->m_sPath.Format(":appdata/Screenshots/{0}", plApplication::GetApplicationInstance()->GetApplicationName());
  AppendCurrentTimestamp(pWriteTask->m_sPath);
  pWriteTask->m_sPath.Append(sContext);
  pWriteTask->m_sPath.Append(".png");

  // we move the file writing off to another thread to save some time
  // if we moved it to the 'FileAccess' thread, writing a screenshot would block resource loading, which can reduce game performance
  // 'LongRunning' will give it even less priority and let the task system do them in parallel to other things
  plTaskSystem::StartSingleTask(pWriteTask, plTaskPriority::LongRunning);
}

void plGameApplicationBase::ExecuteTakeScreenshot(plWindowOutputTargetBase* pOutputTarget, plStringView sContext /* = {} */)
{
  if (m_bTakeScreenshot)
  {
    PLASMA_PROFILE_SCOPE("ExecuteTakeScreenshot");
    plImage img;
    if (pOutputTarget->CaptureImage(img).Succeeded())
    {
      StoreScreenshot(std::move(img), sContext);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

void plGameApplicationBase::CaptureFrame()
{
  m_bCaptureFrame = true;
}

void plGameApplicationBase::SetContinuousFrameCapture(bool bEnable)
{
  m_bContinuousFrameCapture = bEnable;
}

bool plGameApplicationBase::GetContinousFrameCapture() const
{
  return m_bContinuousFrameCapture;
}


plResult plGameApplicationBase::GetAbsFrameCaptureOutputPath(plStringBuilder& ref_sOutputPath)
{
  plStringBuilder sPath = ":appdata/FrameCaptures/Capture_";
  AppendCurrentTimestamp(sPath);
  return plFileSystem::ResolvePath(sPath, &ref_sOutputPath, nullptr);
}

void plGameApplicationBase::ExecuteFrameCapture(plWindowHandle targetWindowHandle, plStringView sContext /*= {} */)
{
  plFrameCaptureInterface* pCaptureInterface = plSingletonRegistry::GetSingletonInstance<plFrameCaptureInterface>();
  if (!pCaptureInterface)
  {
    return;
  }

  PLASMA_PROFILE_SCOPE("ExecuteFrameCapture");
  // If we still have a running capture (i.e., if no one else has taken the capture so far), finish it
  if (pCaptureInterface->IsFrameCapturing())
  {
    if (m_bCaptureFrame)
    {
      plStringBuilder sOutputPath;
      if (GetAbsFrameCaptureOutputPath(sOutputPath).Succeeded())
      {
        sOutputPath.Append(sContext);
        pCaptureInterface->SetAbsCaptureFilePathTemplate(sOutputPath);
      }

      pCaptureInterface->EndFrameCaptureAndWriteOutput(targetWindowHandle);

      plStringBuilder stringBuilder;
      if (pCaptureInterface->GetLastAbsCaptureFileName(stringBuilder).Succeeded())
      {
        plLog::Info("Frame captured: '{}'", stringBuilder);
      }
      else
      {
        plLog::Warning("Frame capture failed!");
      }
      m_bCaptureFrame = false;
    }
    else
    {
      pCaptureInterface->EndFrameCaptureAndDiscardResult(targetWindowHandle);
    }
  }

  // Start capturing the next frame if
  // (a) we want to capture the very next frame, or
  // (b) we capture every frame and later decide if we want to persist or discard it.
  if (m_bCaptureFrame || m_bContinuousFrameCapture)
  {
    pCaptureInterface->StartFrameCapture(targetWindowHandle);
  }
}

//////////////////////////////////////////////////////////////////////////

plResult plGameApplicationBase::ActivateGameState(plWorld* pWorld /*= nullptr*/, const plTransform* pStartPosition /*= nullptr*/)
{
  PLASMA_ASSERT_DEBUG(m_pGameState == nullptr, "ActivateGameState cannot be called when another GameState is already active");

  m_pGameState = CreateGameState(pWorld);

  if (m_pGameState == nullptr)
    return PLASMA_FAILURE;

  m_pWorldLinkedWithGameState = pWorld;
  m_pGameState->OnActivation(pWorld, pStartPosition);

  plGameApplicationStaticEvent e;
  e.m_Type = plGameApplicationStaticEvent::Type::AfterGameStateActivated;
  m_StaticEvents.Broadcast(e);

  PLASMA_BROADCAST_EVENT(AfterGameStateActivation, m_pGameState.Borrow());

  return PLASMA_SUCCESS;
}

void plGameApplicationBase::DeactivateGameState()
{
  if (m_pGameState == nullptr)
    return;

  PLASMA_BROADCAST_EVENT(BeforeGameStateDeactivation, m_pGameState.Borrow());

  plGameApplicationStaticEvent e;
  e.m_Type = plGameApplicationStaticEvent::Type::BeforeGameStateDeactivated;
  m_StaticEvents.Broadcast(e);

  m_pGameState->OnDeactivation();

  plActorManager::GetSingleton()->DestroyAllActors(m_pGameState.Borrow());

  m_pGameState = nullptr;
}

plGameStateBase* plGameApplicationBase::GetActiveGameStateLinkedToWorld(const plWorld* pWorld) const
{
  if (m_pWorldLinkedWithGameState == pWorld)
    return m_pGameState.Borrow();

  return nullptr;
}

plUniquePtr<plGameStateBase> plGameApplicationBase::CreateGameState(plWorld* pWorld)
{
  PLASMA_LOG_BLOCK("Create Game State");

  plUniquePtr<plGameStateBase> pCurState;

  {
    plInt32 iBestPriority = -1;

    plRTTI::ForEachDerivedType<plGameStateBase>(
      [&](const plRTTI* pRtti) {
        plUniquePtr<plGameStateBase> pState = pRtti->GetAllocator()->Allocate<plGameStateBase>();

        const plInt32 iPriority = (plInt32)pState->DeterminePriority(pWorld);
        if (iPriority > iBestPriority)
        {
          iBestPriority = iPriority;

          pCurState = std::move(pState);
        }
      },
      plRTTI::ForEachOptions::ExcludeNonAllocatable);
  }

  return pCurState;
}

void plGameApplicationBase::ActivateGameStateAtStartup()
{
  ActivateGameState().IgnoreResult();
}

plResult plGameApplicationBase::BeforeCoreSystemsStartup()
{
  plStartup::AddApplicationTag("runtime");

  ExecuteBaseInitFunctions();

  return SUPER::BeforeCoreSystemsStartup();
}

void plGameApplicationBase::AfterCoreSystemsStartup()
{
  SUPER::AfterCoreSystemsStartup();

  ExecuteInitFunctions();

  // If one of the init functions already requested the application to quit,
  // something must have gone wrong. Don't continue initialization and let the
  // application exit.
  if (WasQuitRequested())
  {
    return;
  }

  plStartup::StartupHighLevelSystems();

  ActivateGameStateAtStartup();
}

void plGameApplicationBase::ExecuteBaseInitFunctions()
{
  BaseInit_ConfigureLogging();
}

void plGameApplicationBase::BeforeHighLevelSystemsShutdown()
{
  DeactivateGameState();

  {
    // make sure that no resources continue to be streamed in, while the engine shuts down
    plResourceManager::EngineAboutToShutdown();
    plResourceManager::ExecuteAllResourceCleanupCallbacks();
    plResourceManager::FreeAllUnusedResources();
  }
}

void plGameApplicationBase::BeforeCoreSystemsShutdown()
{
  // shut down all actors and APIs that may have been in use
  if (plActorManager::GetSingleton() != nullptr)
  {
    plActorManager::GetSingleton()->Shutdown();
  }

  {
    plFrameAllocator::Reset();
    plResourceManager::FreeAllUnusedResources();
  }

  {
    Deinit_ShutdownGraphicsDevice();
    plResourceManager::FreeAllUnusedResources();
  }

  Deinit_UnloadPlugins();

  // shut down telemetry if it was set up
  {
    plTelemetry::CloseConnection();
  }

  Deinit_ShutdownLogging();

  SUPER::BeforeCoreSystemsShutdown();
}

static bool s_bUpdatePluginsExecuted = false;

PLASMA_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  s_bUpdatePluginsExecuted = true;
}

plApplication::Execution plGameApplicationBase::Run()
{
  if (m_bWasQuitRequested)
    return plApplication::Execution::Quit;

  RunOneFrame();
  return plApplication::Execution::Continue;
}

void plGameApplicationBase::RunOneFrame()
{
  PLASMA_PROFILE_SCOPE("Run");
  s_bUpdatePluginsExecuted = false;

  plActorManager::GetSingleton()->Update();

  if (!IsGameUpdateEnabled())
    return;

  {
    // for plugins that need to hook into this without a link dependency on this lib
    PLASMA_PROFILE_SCOPE("GameApp_BeginAppTick");
    PLASMA_BROADCAST_EVENT(GameApp_BeginAppTick);
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::BeginAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  Run_InputUpdate();

  Run_WorldUpdateAndRender();

  if (!s_bUpdatePluginsExecuted)
  {
    Run_UpdatePlugins();

    PLASMA_ASSERT_DEV(s_bUpdatePluginsExecuted, "plGameApplicationBase::Run_UpdatePlugins has been overridden, but it does not broadcast the "
                                            "global event 'GameApp_UpdatePlugins' anymore.");
  }

  {
    // for plugins that need to hook into this without a link dependency on this lib
    PLASMA_PROFILE_SCOPE("GameApp_EndAppTick");
    PLASMA_BROADCAST_EVENT(GameApp_EndAppTick);

    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::EndAppTick;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    PLASMA_PROFILE_SCOPE("BeforePresent");
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::BeforePresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    PLASMA_PROFILE_SCOPE("Run_Present");
    Run_Present();
  }
  plClock::GetGlobalClock()->Update();
  UpdateFrameTime();

  {
    PLASMA_PROFILE_SCOPE("AfterPresent");
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::AfterPresent;
    m_ExecutionEvents.Broadcast(e);
  }

  {
    PLASMA_PROFILE_SCOPE("Run_FinishFrame");
    Run_FinishFrame();
  }
}

void plGameApplicationBase::Run_InputUpdate()
{
  PLASMA_PROFILE_SCOPE("Run_InputUpdate");
  plInputManager::Update(plClock::GetGlobalClock()->GetTimeDiff());

  if (!Run_ProcessApplicationInput())
    return;

  if (m_pGameState)
  {
    m_pGameState->ProcessInput();
  }
}

bool plGameApplicationBase::Run_ProcessApplicationInput()
{
  return true;
}

void plGameApplicationBase::Run_BeforeWorldUpdate()
{
  PLASMA_PROFILE_SCOPE("GameApplication.BeforeWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->BeforeWorldUpdate();
  }

  {
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::BeforeWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void plGameApplicationBase::Run_AfterWorldUpdate()
{
  PLASMA_PROFILE_SCOPE("GameApplication.AfterWorldUpdate");

  if (m_pGameState)
  {
    m_pGameState->AfterWorldUpdate();
  }

  {
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::AfterWorldUpdates;
    m_ExecutionEvents.Broadcast(e);
  }
}

void plGameApplicationBase::Run_UpdatePlugins()
{
  PLASMA_PROFILE_SCOPE("Run_UpdatePlugins");
  {
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::BeforeUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }

  // for plugins that need to hook into this without a link dependency on this lib
  PLASMA_BROADCAST_EVENT(GameApp_UpdatePlugins);

  {
    plGameApplicationExecutionEvent e;
    e.m_Type = plGameApplicationExecutionEvent::Type::AfterUpdatePlugins;
    m_ExecutionEvents.Broadcast(e);
  }
}

void plGameApplicationBase::Run_Present() {}

void plGameApplicationBase::Run_FinishFrame()
{
  plTelemetry::PerFrameUpdate();
  plResourceManager::PerFrameUpdate();
  plTaskSystem::FinishFrameTasks();
  plFrameAllocator::Swap();
  plProfilingSystem::StartNewFrame();

  // if many messages have been logged, make sure they get written to disk
  plLog::Flush(100, plTime::MakeFromSeconds(10));

  // reset this state
  m_bTakeScreenshot = false;
}

void plGameApplicationBase::UpdateFrameTime()
{
  // Do not use plClock for this, it smooths and clamps the timestep
  const plTime tNow = plTime::Now();

  static plTime tLast = tNow;
  m_FrameTime = tNow - tLast;
  tLast = tNow;
}

PLASMA_STATICLINK_FILE(Core, Core_GameApplication_Implementation_GameApplicationBase);
