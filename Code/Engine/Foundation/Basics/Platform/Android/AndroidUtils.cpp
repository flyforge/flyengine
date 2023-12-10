#include <Foundation/FoundationPCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>

android_app* plAndroidUtils::s_app;
JavaVM* plAndroidUtils::s_vm;
jobject plAndroidUtils::s_na;

void plAndroidUtils::SetAndroidApp(android_app* app)
{
  s_app = app;
  SetAndroidJavaVM(s_app->activity->vm);
  SetAndroidNativeActivity(s_app->activity->clazz);
}

android_app* plAndroidUtils::GetAndroidApp()
{
  return s_app;
}

void plAndroidUtils::SetAndroidJavaVM(JavaVM* vm)
{
  s_vm = vm;
}

JavaVM* plAndroidUtils::GetAndroidJavaVM()
{
  return s_vm;
}

void plAndroidUtils::SetAndroidNativeActivity(jobject nativeActivity)
{
  s_na = nativeActivity;
}

jobject plAndroidUtils::GetAndroidNativeActivity()
{
  return s_na;
}

#endif


PLASMA_STATICLINK_FILE(Foundation, Foundation_Basics_Platform_Android_AndroidUtils);
