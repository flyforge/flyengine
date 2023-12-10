#include <EditorEngineProcess/EditorEngineProcessPCH.h>

#include <EditorEngineProcess/EngineProcGameAppUWP.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessAppUWP.h>
#include <GameEngine/XR/XRInputDevice.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)

plEngineProcessGameApplicationUWP::plEngineProcessGameApplicationUWP() {}

plEngineProcessGameApplicationUWP::~plEngineProcessGameApplicationUWP() {}

plUniquePtr<plEditorEngineProcessApp> plEngineProcessGameApplicationUWP::CreateEngineProcessApp()
{
  plUniquePtr<plEditorEngineProcessApp> ptr = PLASMA_DEFAULT_NEW(plEditorEngineProcessAppUWP);
  m_pEngineProcessApp = static_cast<plEditorEngineProcessAppUWP*>(ptr.Borrow());
  return ptr;
}

void plEngineProcessGameApplicationUWP::Init_ConfigureInput()
{
  plEngineProcessGameApplication::Init_ConfigureInput();

  // Set Anchor
  {
    plInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = plInputSlot_XR_Hand_Left_Trigger;
    cfg.m_bApplyTimeScaling = false;
    plInputManager::SetInputActionConfig("RemoteProcess", "AirTap", cfg, true);
  }
}

bool plEngineProcessGameApplicationUWP::Run_ProcessApplicationInput()
{
  return SUPER::Run_ProcessApplicationInput();
}

#endif
