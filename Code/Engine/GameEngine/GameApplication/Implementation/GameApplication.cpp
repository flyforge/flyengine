#include "Foundation/Utilities/Stats.h"

#include <GameEngine/GameEnginePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/ActorSystem/ActorPluginWindow.h>
#include <Core/Console/QuakeConsole.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/TgaFileFormat.h>
#include <Texture/Image/Image.h>

plGameApplication* plGameApplication::s_pGameApplicationInstance = nullptr;
plDelegate<plGALDevice*(const plGALDeviceCreationDescription&)> plGameApplication::s_DefaultDeviceCreator;

plCVarBool plGameApplication::cvar_AppVSync("App.VSync", false, plCVarFlags::Save, "Enables V-Sync");
plCVarBool plGameApplication::cvar_AppShowFPS("App.ShowFPS", true, plCVarFlags::Save, "Show frames per second counter");
plCVarBool plGameApplication::cvar_AppShowInfo("App.ShowInfo", true, plCVarFlags::Save, "Show info about the application");

plGameApplication::plGameApplication(const char* szAppName, const char* szProjectPath /*= nullptr*/)
  : plGameApplicationBase(szAppName)
  , m_sAppProjectPath(szProjectPath)
{
  m_pUpdateTask = PLASMA_DEFAULT_NEW(plDelegateTask<void>, "", plMakeDelegate(&plGameApplication::UpdateWorldsAndExtractViews, this));
  m_pUpdateTask->ConfigureTask("GameApplication.Update", plTaskNesting::Maybe);

  s_pGameApplicationInstance = this;
  m_bWasQuitRequested = false;

  m_pConsole = PLASMA_DEFAULT_NEW(plQuakeConsole);
  plConsole::SetMainConsole(m_pConsole.Borrow());
}

plGameApplication::~plGameApplication()
{
  s_pGameApplicationInstance = nullptr;
}

// static
void plGameApplication::SetOverrideDefaultDeviceCreator(plDelegate<plGALDevice*(const plGALDeviceCreationDescription&)> creator)
{
  s_DefaultDeviceCreator = creator;
}

void plGameApplication::ReinitializeInputConfig()
{
  Init_ConfigureInput();
}

plString plGameApplication::FindProjectDirectory() const
{
  PLASMA_ASSERT_RELEASE(!m_sAppProjectPath.IsEmpty(), "Either the project must have a built-in project directory passed to the plGameApplication constructor, or m_sAppProjectPath must be set manually before doing project setup, or plGameApplication::FindProjectDirectory() must be overridden.");

  if (plPathUtils::IsAbsolutePath(m_sAppProjectPath))
    return m_sAppProjectPath;

  // first check if the path is relative to the SDK special directory
  {
    plStringBuilder relToSdk(">sdk/", m_sAppProjectPath);
    plStringBuilder absToSdk;
    if (plFileSystem::ResolveSpecialDirectory(relToSdk, absToSdk).Succeeded())
    {
      if (plOSFile::ExistsDirectory(absToSdk))
        return absToSdk;
    }
  }

  plStringBuilder result;
  if (plFileSystem::FindFolderWithSubPath(result, plOSFile::GetApplicationDirectory(), m_sAppProjectPath).Failed())
  {
    plLog::Error("Could not find the project directory.");
  }

  return result;
}

bool plGameApplication::IsGameUpdateEnabled() const
{
  return plRenderWorld::GetMainViews().GetCount() > 0;
}

void plGameApplication::Run_WorldUpdateAndRender()
{
  PLASMA_PROFILE_SCOPE("Run_WorldUpdateAndRender");
  // If multi-threaded rendering is disabled, the same content is updated/extracted and rendered in the same frame.
  // As plRenderWorld::BeginFrame applies the render pipeline properties that were set during the update phase, it needs to be done after update/extraction but before rendering.
  if (!plRenderWorld::GetUseMultithreadedRendering())
  {
    UpdateWorldsAndExtractViews();
  }

  plRenderWorld::BeginFrame();

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // On most platforms it doesn't matter that much how early this happens.
  // But on HoloLens this executes something that needs to be done at the right time,
  // for the reprojection to work properly.
  const plUInt64 uiRenderFrame = plRenderWorld::GetUseMultithreadedRendering() ? plRenderWorld::GetFrameCounter() - 1 : plRenderWorld::GetFrameCounter();
  pDevice->BeginFrame(uiRenderFrame);

  plTaskGroupID updateTaskID;
  if (plRenderWorld::GetUseMultithreadedRendering())
  {
    updateTaskID = plTaskSystem::StartSingleTask(m_pUpdateTask, plTaskPriority::EarlyThisFrame);
  }

  RenderFps();
  RenderConsole();

  plRenderWorld::Render(plRenderContext::GetDefaultInstance());

  if (plRenderWorld::GetUseMultithreadedRendering())
  {
    PLASMA_PROFILE_SCOPE("Wait for UpdateWorldsAndExtractViews");
    plTaskSystem::WaitForGroup(updateTaskID);
  }
}

void plGameApplication::Run_Present()
{
  plHybridArray<plActor*, 8> allActors;
  plActorManager::GetSingleton()->GetAllActors(allActors);

  for (plActor* pActor : allActors)
  {
    PLASMA_PROFILE_SCOPE(pActor->GetName());

    plActorPluginWindow* pWindowPlugin = pActor->GetPlugin<plActorPluginWindow>();

    if (pWindowPlugin == nullptr)
      continue;

    // Ignore actors without an output target
    if (auto pOutput = pWindowPlugin->GetOutputTarget())
    {
      // if we have multiple actors, append the actor name to each screenshot
      plStringBuilder ctxt;
      if (allActors.GetCount() > 1)
      {
        ctxt.Append(" - ", pActor->GetName());
      }

      ExecuteTakeScreenshot(pOutput, ctxt);

      if (pWindowPlugin->GetWindow())
      {
        ExecuteFrameCapture(pWindowPlugin->GetWindow()->GetNativeWindowHandle(), ctxt);
      }

      PLASMA_PROFILE_SCOPE("Present");
      pOutput->Present(cvar_AppVSync);
    }
  }
}

void plGameApplication::Run_FinishFrame()
{
  plGALDevice::GetDefaultDevice()->EndFrame();
  plRenderWorld::EndFrame();

  SUPER::Run_FinishFrame();
}

void plGameApplication::UpdateWorldsAndExtractViews()
{
  plStringBuilder sb;
  sb.Format("FRAME {}", plRenderWorld::GetFrameCounter());
  PLASMA_PROFILE_SCOPE(sb.GetData());

  Run_BeforeWorldUpdate();

  static plHybridArray<plWorld*, 16> worldsToUpdate;
  worldsToUpdate.Clear();

  auto mainViews = plRenderWorld::GetMainViews();
  for (auto hView : mainViews)
  {
    plView* pView = nullptr;
    if (plRenderWorld::TryGetView(hView, pView))
    {
      plWorld* pWorld = pView->GetWorld();

      if (pWorld != nullptr && !worldsToUpdate.Contains(pWorld))
      {
        worldsToUpdate.PushBack(pWorld);
      }
    }
  }

  if (plRenderWorld::GetUseMultithreadedRendering())
  {
    plTaskGroupID updateWorldsTaskID = plTaskSystem::CreateTaskGroup(plTaskPriority::EarlyThisFrame);
    for (plUInt32 i = 0; i < worldsToUpdate.GetCount(); ++i)
    {
      plTaskSystem::AddTaskToGroup(updateWorldsTaskID, worldsToUpdate[i]->GetUpdateTask());
    }
    plTaskSystem::StartTaskGroup(updateWorldsTaskID);
    plTaskSystem::WaitForGroup(updateWorldsTaskID);
  }
  else
  {
    for (plUInt32 i = 0; i < worldsToUpdate.GetCount(); ++i)
    {
      plWorld* pWorld = worldsToUpdate[i];
      PLASMA_LOCK(pWorld->GetWriteMarker());

      pWorld->Update();
    }
  }

  Run_AfterWorldUpdate();

  // do this now, in parallel to the view extraction
  Run_UpdatePlugins();

  plRenderWorld::ExtractMainViews();
}

void plGameApplication::RenderFps()
{
  PLASMA_PROFILE_SCOPE("RenderFps");
  // Do not use plClock for this, it smooths and clamps the timestep

  static plTime tAccumTime;
  static plTime tDisplayedFrameTime = m_FrameTime;
  static plUInt32 uiFrames = 0;
  static plUInt32 uiFPS = 0;

  ++uiFrames;
  tAccumTime += m_FrameTime;

  if (tAccumTime >= plTime::Seconds(0.5))
  {
    tAccumTime -= plTime::Seconds(0.5);
    tDisplayedFrameTime = m_FrameTime;

    uiFPS = uiFrames * 2;
    uiFrames = 0;
  }

  if (cvar_AppShowFPS)
  {
    if (const plView* pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView))
    {
      plColor color = plColor::LightGreen;
      if(uiFPS < 30)
        color = plColor::Red;
      else if(uiFPS < 60)
        color = plColor::Orange;


      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugTextPlacement::TopRight, "AAAFPS", plFmt("{0} fps, {1} ms", uiFPS, plArgF(tDisplayedFrameTime.GetMilliseconds(), 1, false, 4)), color);
    }
  }

  if (cvar_AppShowInfo)
  {
    if (const plView* pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView))
    {
      const plSystemInformation sysInfo = plSystemInformation::Get();

      plStringView rednererName = plCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, "DX11");
      plStringBuilder tmp;
      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugTextPlacement::TopRight, "Platform", plFmt("Renderer: {0}", rednererName.GetData(tmp)));

      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugTextPlacement::TopRight, "Platform", plFmt("Platform: {0}", sysInfo.GetPlatformName()));
      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugTextPlacement::TopRight, "Platform", plFmt("Architecture: {0}", sysInfo.Is64BitOS() ? "x64" : "x86"));
      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugTextPlacement::TopRight, "Platform", plFmt("Build Type: {0}", sysInfo.GetBuildConfiguration()));

      // Move this to another section (not needed in default view)
//      float memory = plMath::RoundUp(plMemoryUtils::ConvertToGigaBytes(sysInfo.GetInstalledMainMemory()), 1);
//      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugRenderer::ScreenPlacement::TopLeft, "Hardware", plFmt("Threads: {0}, Memory: {1} (Gb)", sysInfo.GetCPUCoreCount(), memory));
//
//      plString gpuMemory = plStats::GetStat("GPU Resource Pool/Memory Consumption").Get<plString>();
//      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugRenderer::ScreenPlacement::TopLeft, "Usage", plFmt("GPU Memory Used: {0}", gpuMemory));
//
//      float availableMemory = plMath::RoundUp(plMemoryUtils::ConvertToGigaBytes(sysInfo.GetAvailableMainMemory()), 1);
//      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugRenderer::ScreenPlacement::TopLeft, "Usage", plFmt("System Memory Remaining: {0} (Gb)", availableMemory));

      plString profiling = plStats::GetStat("Features/Profiling").Get<plString>();
      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugTextPlacement::TopRight, "Features", plFmt("Profiling: {0}", profiling), (profiling == "Enabled" ? plColor::LightGreen : plColor::Red));

      plString allocTracking = plStats::GetStat("Features/Allocation Tracking").Get<plString>();
      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugTextPlacement::TopRight, "Features", plFmt("Allocation Tracking: {0}", allocTracking), (allocTracking == "Enabled" ? plColor::LightGreen : plColor::Red));
    }
  }
}

void plGameApplication::RenderConsole()
{
  PLASMA_PROFILE_SCOPE("RenderConsole");

  if (!m_bShowConsole || !m_pConsole)
    return;

  const plView* pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView);
  if (pView == nullptr)
    return;

  plViewHandle hView = pView->GetHandle();

  const float fViewWidth = pView->GetViewport().width;
  const float fViewHeight = pView->GetViewport().height;
  const float fTextHeight = 20.0f;
  const float fConsoleHeight = (fViewHeight / 2.0f);
  const float fBorderWidth = 3.0f;
  const float fConsoleTextAreaHeight = fConsoleHeight - fTextHeight - (2.0f * fBorderWidth);

  const plInt32 iTextHeight = (plInt32)fTextHeight;
  const plInt32 iTextLeft = (plInt32)(fBorderWidth);

  {
    plColor backgroundColor(0.3f, 0.3f, 0.3f, 0.7f);
    plDebugRenderer::Draw2DRectangle(hView, plRectFloat(0.0f, 0.0f, fViewWidth, fConsoleHeight), 0.0f, backgroundColor);

    plColor foregroundColor(0.0f, 0.0f, 0.0f, 0.8f);
    plDebugRenderer::Draw2DRectangle(hView, plRectFloat(fBorderWidth, 0.0f, fViewWidth - (2.0f * fBorderWidth), fConsoleTextAreaHeight), 0.0f, foregroundColor);
    plDebugRenderer::Draw2DRectangle(hView, plRectFloat(fBorderWidth, fConsoleTextAreaHeight + fBorderWidth, fViewWidth - (2.0f * fBorderWidth), fTextHeight), 0.0f, foregroundColor);
  }

  {
    PLASMA_LOCK(m_pConsole->GetMutex());

    auto& consoleStrings = m_pConsole->GetConsoleStrings();

    plUInt32 uiNumConsoleLines = (plUInt32)(plMath::Ceil(fConsoleTextAreaHeight / fTextHeight));
    plInt32 iFirstLinePos = (plInt32)fConsoleTextAreaHeight - uiNumConsoleLines * iTextHeight;
    plInt32 uiFirstLine = m_pConsole->GetScrollPosition() + uiNumConsoleLines - 1;
    plInt32 uiSkippedLines = plMath::Max(uiFirstLine - (plInt32)consoleStrings.GetCount() + 1, 0);

    for (plUInt32 i = uiSkippedLines; i < uiNumConsoleLines; ++i)
    {
      auto& consoleString = consoleStrings[uiFirstLine - i];
      plDebugRenderer::Draw2DText(hView, consoleString.m_sText.GetData(), plVec2I32(iTextLeft, iFirstLinePos + i * iTextHeight), consoleString.GetColor());
    }

    plStringBuilder tmp;
    plDebugRenderer::Draw2DText(hView, m_pConsole->GetInputLine().GetData(tmp), plVec2I32(iTextLeft, (plInt32)(fConsoleTextAreaHeight + fBorderWidth)), plColor::White);

    if (plMath::Fraction(plClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds()) > 0.5)
    {
      float fCaretPosition = (float)m_pConsole->GetCaretPosition();
      plColor caretColor(1.0f, 1.0f, 1.0f, 0.5f);
      plDebugRenderer::Draw2DRectangle(hView, plRectFloat(fBorderWidth + fCaretPosition * 8.0f + 2.0f, fConsoleTextAreaHeight + fBorderWidth + 1.0f, 2.0f, fTextHeight - 2.0f), 0.0f, caretColor);
    }
  }
}

namespace
{
  const char* s_szInputSet = "GameApp";
  const char* s_szCloseAppAction = "CloseApp";
  const char* s_szShowConsole = "ShowConsole";
  const char* s_szShowFpsAction = "ShowFps";
  const char* s_szReloadResourcesAction = "ReloadResources";
  const char* s_szCaptureProfilingAction = "CaptureProfiling";
  const char* s_szCaptureFrame = "CaptureFrame";
  const char* s_szTakeScreenshot = "TakeScreenshot";
} // namespace

void plGameApplication::Init_ConfigureInput()
{
  plInputActionConfig config;

  config.m_sInputSlotTrigger[0] = plInputSlot_KeyEscape;
  plInputManager::SetInputActionConfig(s_szInputSet, s_szCloseAppAction, config, true);

  // the tilde has problematic behavior on keyboards where it is a hat (^)
  config.m_sInputSlotTrigger[0] = plInputSlot_KeyF1;
  plInputManager::SetInputActionConfig("Console", s_szShowConsole, config, true);

  // in the editor we cannot use F5, because that is already 'run application'
  // so we use F4 there, and it should be consistent here
  config.m_sInputSlotTrigger[0] = plInputSlot_KeyF4;
  plInputManager::SetInputActionConfig(s_szInputSet, s_szReloadResourcesAction, config, true);

  config.m_sInputSlotTrigger[0] = plInputSlot_KeyF5;
  plInputManager::SetInputActionConfig(s_szInputSet, s_szShowFpsAction, config, true);

  config.m_sInputSlotTrigger[0] = plInputSlot_KeyF8;
  plInputManager::SetInputActionConfig(s_szInputSet, s_szCaptureProfilingAction, config, true);

  config.m_sInputSlotTrigger[0] = plInputSlot_KeyF11;
  plInputManager::SetInputActionConfig(s_szInputSet, s_szCaptureFrame, config, true);

  config.m_sInputSlotTrigger[0] = plInputSlot_KeyF12;
  plInputManager::SetInputActionConfig(s_szInputSet, s_szTakeScreenshot, config, true);

  {
    plStringView sConfigFile = plGameAppInputConfig::s_sConfigFile;

#if PLASMA_ENABLED(PLASMA_MIGRATE_RUNTIMECONFIGS)
    sConfigFile = plFileSystem::MigrateFileLocation(":project/InputConfig.ddl", sConfigFile);
#endif

    plFileReader file;
    if (file.Open(sConfigFile).Succeeded())
    {
      plHybridArray<plGameAppInputConfig, 32> InputActions;

      plGameAppInputConfig::ReadFromDDL(file, InputActions);
      plGameAppInputConfig::ApplyAll(InputActions);
    }
  }

  if (m_pConsole)
  {
    m_pConsole->LoadInputHistory(":appdata/ConsoleInputHistory.cfg");
  }
}

bool plGameApplication::Run_ProcessApplicationInput()
{
  // the show console command must be in the "Console" input set, because we are using that for exclusive input when the console is open
  if (plInputManager::GetInputActionState("Console", s_szShowConsole) == plKeyState::Pressed)
  {
    m_bShowConsole = !m_bShowConsole;

    if (m_bShowConsole)
      plInputManager::SetExclusiveInputSet("Console");
    else
    {
      plInputManager::SetExclusiveInputSet("");
      m_pConsole->SaveInputHistory(":appdata/ConsoleInputHistory.cfg").IgnoreResult();
    }
  }

  if (plInputManager::GetInputActionState(s_szInputSet, s_szShowFpsAction) == plKeyState::Pressed)
  {
    cvar_AppShowFPS = !cvar_AppShowFPS;
  }

  if (plInputManager::GetInputActionState(s_szInputSet, s_szReloadResourcesAction) == plKeyState::Pressed)
  {
    plResourceManager::ReloadAllResources(false);
  }

  if (plInputManager::GetInputActionState(s_szInputSet, s_szTakeScreenshot) == plKeyState::Pressed)
  {
    TakeScreenshot();
  }

  if (plInputManager::GetInputActionState(s_szInputSet, s_szCaptureProfilingAction) == plKeyState::Pressed)
  {
    TakeProfilingCapture();
  }

  if (plInputManager::GetInputActionState(s_szInputSet, s_szCaptureFrame) == plKeyState::Pressed)
  {
    CaptureFrame();
  }

  if (m_pConsole)
  {
    m_pConsole->DoDefaultInputHandling(m_bShowConsole);

    if (m_bShowConsole)
      return false;
  }

  if (plInputManager::GetInputActionState(s_szInputSet, s_szCloseAppAction) == plKeyState::Pressed)
  {
    RequestQuit();
  }

  return true;
}

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_GameApplication);
