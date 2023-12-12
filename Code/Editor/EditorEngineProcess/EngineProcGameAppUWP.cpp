#include <EditorEngineProcess/EditorEngineProcessPCH.h>

#include <EditorEngineProcess/EngineProcGameAppUWP.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessAppUWP.h>
#include <GameEngine/XR/XRInputDevice.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)

PlasmaEngineProcessGameApplicationUWP::PlasmaEngineProcessGameApplicationUWP() {}

PlasmaEngineProcessGameApplicationUWP::~PlasmaEngineProcessGameApplicationUWP() {}

plUniquePtr<PlasmaEditorEngineProcessApp> PlasmaEngineProcessGameApplicationUWP::CreateEngineProcessApp()
{
  plUniquePtr<PlasmaEditorEngineProcessApp> ptr = PLASMA_DEFAULT_NEW(PlasmaEditorEngineProcessAppUWP);
  m_pEngineProcessApp = static_cast<PlasmaEditorEngineProcessAppUWP*>(ptr.Borrow());
  return ptr;
}

void PlasmaEngineProcessGameApplicationUWP::Init_ConfigureInput()
{
  PlasmaEngineProcessGameApplication::Init_ConfigureInput();

  // Set Anchor
  {
    plInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = plInputSlot_XR_Hand_Left_Trigger;
    cfg.m_bApplyTimeScaling = false;
    plInputManager::SetInputActionConfig("RemoteProcess", "AirTap", cfg, true);
  }
}

bool PlasmaEngineProcessGameApplicationUWP::Run_ProcessApplicationInput()
{
  return SUPER::Run_ProcessApplicationInput();
}

#endif
