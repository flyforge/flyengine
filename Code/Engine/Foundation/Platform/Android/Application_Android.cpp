#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_ANDROID)

#  include <Foundation/Application/Application.h>
#  include <Foundation/Application/Implementation/Android/Application_android.h>
#  include <android/log.h>
#  include <android_native_app_glue.h>

static void plAndroidHandleCmd(struct android_app* pApp, int32_t cmd)
{
  plAndroidApplication* pAndroidApp = static_cast<plAndroidApplication*>(pApp->userData);
  pAndroidApp->HandleCmd(cmd);
}

static int32_t plAndroidHandleInput(struct android_app* pApp, AInputEvent* pEvent)
{
  plAndroidApplication* pAndroidApp = static_cast<plAndroidApplication*>(pApp->userData);
  return pAndroidApp->HandleInput(pEvent);
}

plAndroidApplication::plAndroidApplication(struct android_app* pApp, plApplication* pPlApp)
  : m_pApp(pApp)
  , m_pPlApp(pPlApp)
{
  pApp->userData = this;
  pApp->onAppCmd = plAndroidHandleCmd;
  pApp->onInputEvent = plAndroidHandleInput;
  //#TODO: acquire sensors, set app->onAppCmd, set app->onInputEvent
}

plAndroidApplication::~plAndroidApplication() {}

void plAndroidApplication::AndroidRun()
{
  bool bRun = true;
  while (true)
  {
    struct android_poll_source* pSource = nullptr;
    int iIdent = 0;
    int iEvents = 0;
    while ((iIdent = ALooper_pollAll(0, nullptr, &iEvents, (void**)&pSource)) >= 0)
    {
      if (pSource != nullptr)
        pSource->process(m_pApp, pSource);

      HandleIdent(iIdent);
    }
    if (bRun && m_pPlApp->Run() != plApplication::Execution::Continue)
    {
      bRun = false;
      ANativeActivity_finish(m_pApp->activity);
    }
    if (m_pApp->destroyRequested)
    {
      break;
    }
  }
}

void plAndroidApplication::HandleCmd(int32_t cmd)
{
  //#TODO:
}

int32_t plAndroidApplication::HandleInput(AInputEvent* pEvent)
{
  //#TODO:
  return 0;
}

void plAndroidApplication::HandleIdent(plInt32 iIdent)
{
  //#TODO:
}

PL_FOUNDATION_DLL void plAndroidRun(struct android_app* pApp, plApplication* pPlApp)
{
  plAndroidApplication androidApp(pApp, pPlApp);

  if (plRun_Startup(pPlApp).Succeeded())
  {
    androidApp.AndroidRun();
  }
  plRun_Shutdown(pPlApp);

  const int iReturnCode = pPlApp->GetReturnCode();
  if (iReturnCode != 0)
  {
    const char* szReturnCode = pPlApp->TranslateReturnCode();
    if (szReturnCode != nullptr && szReturnCode[0] != '\0')
      __android_log_print(ANDROID_LOG_ERROR, "plEngine", "Return Code: '%s'", szReturnCode);
  }
}

#endif


