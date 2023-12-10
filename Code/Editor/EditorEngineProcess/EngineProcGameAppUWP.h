#pragma once

#include <EditorEngineProcess/EngineProcGameApp.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)

class plEditorEngineProcessAppUWP;

class plEngineProcessGameApplicationUWP : public plEngineProcessGameApplication
{
  using SUPER = plEngineProcessGameApplication;

public:
  plEngineProcessGameApplicationUWP();
  ~plEngineProcessGameApplicationUWP();

protected:
  virtual bool Run_ProcessApplicationInput() override;
  virtual void Init_ConfigureInput() override;
  virtual plUniquePtr<plEditorEngineProcessApp> CreateEngineProcessApp() override;

private:
  plEditorEngineProcessAppUWP* m_pEngineProcessApp;
  plTime m_HandPressTime;
  plVec3 m_vHandStartPosition;
};

#endif
