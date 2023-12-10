#include <Foundation/FoundationPCH.h>

#include <Foundation/System/PlatformFeatures.h>
#include <Foundation/System/Screen.h>

#if PLASMA_ENABLED(PLASMA_SUPPORTS_GLFW)
#  include <Foundation/System/Implementation/glfw/Screen_glfw.inl>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/System/Implementation/Win/Screen_win32.inl>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  include <Foundation/System/Implementation/uwp/Screen_uwp.inl>
#else

plResult plScreen::EnumerateScreens(plHybridArray<plScreenInfo, 2>& out_Screens)
{
  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return PLASMA_FAILURE;
}

#endif

void plScreen::PrintScreenInfo(const plHybridArray<plScreenInfo, 2>& screens, plLogInterface* pLog /*= plLog::GetThreadLocalLogSystem()*/)
{
  PLASMA_LOG_BLOCK(pLog, "Screens");

  plLog::Info(pLog, "Found {0} screens", screens.GetCount());

  for (const auto& screen : screens)
  {
    plLog::Dev(pLog, "'{0}': Offset = ({1}, {2}), Resolution = ({3}, {4}){5}", screen.m_sDisplayName, screen.m_iOffsetX, screen.m_iOffsetY, screen.m_iResolutionX, screen.m_iResolutionY, screen.m_bIsPrimary ? " (primary)" : "");
  }
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_System_Implementation_Screen);
