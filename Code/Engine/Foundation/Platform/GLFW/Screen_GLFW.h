#include <Foundation/FoundationInternal.h>
PL_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/PlatformFeatures.h>
#include <Foundation/System/Screen.h>
#include <GLFW/glfw3.h>

namespace
{
  plResult plGlfwError(const char* file, size_t line)
  {
    const char* desc;
    int errorCode = glfwGetError(&desc);
    if (errorCode != GLFW_NO_ERROR)
    {
      plLog::Error("GLFW error {} ({}): {} - {}", file, line, errorCode, desc);
      return PL_FAILURE;
    }
    return PL_SUCCESS;
  }
} // namespace

#define PL_GLFW_RETURN_FAILURE_ON_ERROR()         \
  do                                              \
  {                                               \
    if (plGlfwError(__FILE__, __LINE__).Failed()) \
      return PL_FAILURE;                          \
  } while (false)

plResult plScreen::EnumerateScreens(plHybridArray<plScreenInfo, 2>& out_Screens)
{
  out_Screens.Clear();

  int iMonitorCount = 0;
  GLFWmonitor** pMonitors = glfwGetMonitors(&iMonitorCount);
  PL_GLFW_RETURN_FAILURE_ON_ERROR();
  if (iMonitorCount == 0)
  {
    return PL_FAILURE;
  }

  GLFWmonitor* pPrimaryMonitor = glfwGetPrimaryMonitor();
  PL_GLFW_RETURN_FAILURE_ON_ERROR();
  if (pPrimaryMonitor == nullptr)
  {
    return PL_FAILURE;
  }

  for (int i = 0; i < iMonitorCount; ++i)
  {
    plScreenInfo& screen = out_Screens.ExpandAndGetRef();
    screen.m_sDisplayName = glfwGetMonitorName(pMonitors[i]);
    PL_GLFW_RETURN_FAILURE_ON_ERROR();

    const GLFWvidmode* mode = glfwGetVideoMode(pMonitors[i]);
    PL_GLFW_RETURN_FAILURE_ON_ERROR();
    if (mode == nullptr)
    {
      return PL_FAILURE;
    }
    screen.m_iResolutionX = mode->width;
    screen.m_iResolutionY = mode->height;

    glfwGetMonitorPos(pMonitors[i], &screen.m_iOffsetX, &screen.m_iOffsetY);
    PL_GLFW_RETURN_FAILURE_ON_ERROR();

    screen.m_bIsPrimary = pMonitors[i] == pPrimaryMonitor;
  }

  return PL_SUCCESS;
}