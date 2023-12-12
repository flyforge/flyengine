#pragma once

#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#if PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)

#  include <Foundation/Basics.h>
#  include <Foundation/Strings/String.h>

class plApplication;
struct AInputEvent;

class plAndroidApplication
{
public:
  plAndroidApplication(struct android_app* pApp, plApplication* pEzApp);
  ~plAndroidApplication();
  void AndroidRun();
  void HandleCmd(int32_t cmd);
  int32_t HandleInput(AInputEvent* pEvent);
  void HandleIdent(plInt32 iIdent);

private:
  struct android_app* m_pApp;
  plApplication* m_pEzApp;
};

#endif
