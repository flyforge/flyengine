#pragma once

#include <EditorEngineProcess/EngineProcGameApp.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)

class PlasmaEditorEngineProcessAppUWP;

class PlasmaEngineProcessGameApplicationUWP : public PlasmaEngineProcessGameApplication
{
  typedef PlasmaEngineProcessGameApplication SUPER;

public:
  PlasmaEngineProcessGameApplicationUWP();
  ~PlasmaEngineProcessGameApplicationUWP();

protected:
  virtual bool Run_ProcessApplicationInput() override;
  virtual void Init_ConfigureInput() override;
  virtual plUniquePtr<PlasmaEditorEngineProcessApp> CreateEngineProcessApp() override;

private:
  PlasmaEditorEngineProcessAppUWP* m_pEngineProcessApp;
  plTime m_HandPressTime;
  plVec3 m_vHandStartPosition;
};

#endif
