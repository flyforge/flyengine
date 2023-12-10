#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <GLFW/glfw3.h>

namespace {
  plResult plGlfwError(const char* file, size_t line)
  {
    const char* desc;
    int errorCode = glfwGetError(&desc);
    if(errorCode != GLFW_NO_ERROR)
    {
      plLog::Error("GLFW error {} ({}): {} - {}", file, line, errorCode, desc);
      return PLASMA_FAILURE;
    }
    return PLASMA_SUCCESS;
  }
}

#define PLASMA_GLFW_RETURN_FAILURE_ON_ERROR() do { if(plGlfwError(__FILE__, __LINE__).Failed()) return PLASMA_FAILURE; } while(false)

plResult plScreen::EnumerateScreens(plHybridArray<plScreenInfo, 2>& out_Screens)
{
  out_Screens.Clear();

  int iMonitorCount = 0;
  GLFWmonitor** pMonitors = glfwGetMonitors(&iMonitorCount);
  PLASMA_GLFW_RETURN_FAILURE_ON_ERROR();
  if(iMonitorCount == 0)
  {
    return PLASMA_FAILURE;
  }

  GLFWmonitor* pPrimaryMonitor = glfwGetPrimaryMonitor();
  PLASMA_GLFW_RETURN_FAILURE_ON_ERROR();
  if(pPrimaryMonitor == nullptr)
  {
    return PLASMA_FAILURE;
  }

  for(int i=0; i < iMonitorCount; ++i)
  {
    plScreenInfo& screen = out_Screens.ExpandAndGetRef();
    screen.m_sDisplayName = glfwGetMonitorName(pMonitors[i]);
    PLASMA_GLFW_RETURN_FAILURE_ON_ERROR();

    const GLFWvidmode* mode = glfwGetVideoMode(pMonitors[i]);
    PLASMA_GLFW_RETURN_FAILURE_ON_ERROR();
    if(mode == nullptr)
    {
      return PLASMA_FAILURE;
    }
    screen.m_iResolutionX = mode->width;
    screen.m_iResolutionY = mode->height;

    glfwGetMonitorPos(pMonitors[i], &screen.m_iOffsetX, &screen.m_iOffsetY);
    PLASMA_GLFW_RETURN_FAILURE_ON_ERROR();
    
    screen.m_bIsPrimary = pMonitors[i] == pPrimaryMonitor;
  }

  return PLASMA_SUCCESS;
}