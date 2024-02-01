#include <Core/System/Window.h>
#include <Foundation/Configuration/Startup.h>

#include <GLFW/glfw3.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
#  ifdef APIENTRY
#    undef APIENTRY
#endif

# include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  define GLFW_EXPOSE_NATIVE_WIN32
#  include <GLFW/glfw3native.h>
#endif

namespace
{
  void glfwErrorCallback(int errorCode, const char* msg)
  {
    plLog::Error("GLFW error {}: {}", errorCode, msg);
  }
}


// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(Core, Window)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    if (!glfwInit())
    {
      const char* szErrorDesc = nullptr;
      int iErrorCode = glfwGetError(&szErrorDesc);
      plLog::Warning("Failed to initialize glfw. Window and input related functionality will not be available. Error Code {}. GLFW Error Message: {}", iErrorCode, szErrorDesc);
    }
    else
    {
      // Set the error callback after init, so we don't print an error if init fails.
      glfwSetErrorCallback(&glfwErrorCallback);
    }
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    glfwSetErrorCallback(nullptr);
    glfwTerminate();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace {
  plResult plGlfwError(const char* file, size_t line)
  {
    const char* desc;
    int errorCode = glfwGetError(&desc);
    if(errorCode != GLFW_NO_ERROR)
    {
      plLog::Error("GLFW error {} ({}): {} - {}", file, line, errorCode, desc);
      return PL_FAILURE;
    }
    return PL_SUCCESS;
  }
}

#define PL_GLFW_RETURN_FAILURE_ON_ERROR() do { if(plGlfwError(__FILE__, __LINE__).Failed()) return PL_FAILURE; } while(false)

plResult plWindow::Initialize()
{
  PL_LOG_BLOCK("plWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    Destroy().IgnoreResult();
  }

  PL_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");

  GLFWmonitor* pMonitor = nullptr; // nullptr for windowed, fullscreen otherwise

  switch (m_CreationDescription.m_WindowMode)
  {
    case plWindowMode::WindowResizable:
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
      PL_GLFW_RETURN_FAILURE_ON_ERROR();
      break;
    case plWindowMode::WindowFixedResolution:
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
      PL_GLFW_RETURN_FAILURE_ON_ERROR();
      break;
    case plWindowMode::FullscreenFixedResolution:
    case plWindowMode::FullscreenBorderlessNativeResolution:
      if (m_CreationDescription.m_iMonitor == -1)
      {
        pMonitor = glfwGetPrimaryMonitor();
        PL_GLFW_RETURN_FAILURE_ON_ERROR();
      }
      else
      {
        int iMonitorCount = 0;
        GLFWmonitor** pMonitors = glfwGetMonitors(&iMonitorCount);
        PL_GLFW_RETURN_FAILURE_ON_ERROR();
        if (m_CreationDescription.m_iMonitor >= iMonitorCount)
        {
          plLog::Error("Can not create window on monitor {} only {} monitors connected", m_CreationDescription.m_iMonitor, iMonitorCount);
          return PL_FAILURE;
        }
        pMonitor = pMonitors[m_CreationDescription.m_iMonitor];
      }

      if (m_CreationDescription.m_WindowMode == plWindowMode::FullscreenBorderlessNativeResolution)
      {
        const GLFWvidmode* pVideoMode = glfwGetVideoMode(pMonitor);
        PL_GLFW_RETURN_FAILURE_ON_ERROR();
        if(pVideoMode == nullptr)
        {
          plLog::Error("Failed to get video mode for monitor");
          return PL_FAILURE;
        }
        m_CreationDescription.m_Resolution.width = pVideoMode->width;
        m_CreationDescription.m_Resolution.height = pVideoMode->height;
        m_CreationDescription.m_Position.x = 0;
        m_CreationDescription.m_Position.y = 0;

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        PL_GLFW_RETURN_FAILURE_ON_ERROR();
      }

      break;
  }


  glfwWindowHint(GLFW_FOCUS_ON_SHOW, m_CreationDescription.m_bSetForegroundOnInit ? GLFW_TRUE : GLFW_FALSE);
  PL_GLFW_RETURN_FAILURE_ON_ERROR();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  PL_GLFW_RETURN_FAILURE_ON_ERROR();

  GLFWwindow* pWindow = glfwCreateWindow(m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height, m_CreationDescription.m_Title.GetData(), pMonitor, NULL);
  PL_GLFW_RETURN_FAILURE_ON_ERROR();

  if (pWindow == nullptr)
  {
    plLog::Error("Failed to create glfw window");
    return PL_FAILURE;
  }
#if PL_ENABLED(PL_PLATFORM_LINUX)
  m_hWindowHandle.type = plWindowHandle::Type::GLFW;
  m_hWindowHandle.glfwWindow = pWindow;
#else
  m_hWindowHandle = pWindow;
#endif

  if (m_CreationDescription.m_Position != plVec2I32(0x80000000, 0x80000000))
  {
    glfwSetWindowPos(pWindow, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);
    PL_GLFW_RETURN_FAILURE_ON_ERROR();
  }

  glfwSetWindowUserPointer(pWindow, this);
  glfwSetWindowSizeCallback(pWindow, &plWindow::SizeCallback);
  glfwSetWindowPosCallback(pWindow, &plWindow::PositionCallback);
  glfwSetWindowCloseCallback(pWindow, &plWindow::CloseCallback);
  glfwSetWindowFocusCallback(pWindow, &plWindow::FocusCallback);
  glfwSetKeyCallback(pWindow, &plWindow::KeyCallback);
  glfwSetCharCallback(pWindow, &plWindow::CharacterCallback);
  glfwSetCursorPosCallback(pWindow, &plWindow::CursorPositionCallback);
  glfwSetMouseButtonCallback(pWindow, &plWindow::MouseButtonCallback);
  glfwSetScrollCallback(pWindow, &plWindow::ScrollCallback);
  PL_GLFW_RETURN_FAILURE_ON_ERROR();

#if PL_ENABLED(PL_PLATFORM_LINUX)
  PL_ASSERT_DEV(m_hWindowHandle.type == plWindowHandle::Type::GLFW, "not a GLFW handle");
  m_pInputDevice = PL_DEFAULT_NEW(plStandardInputDevice, m_CreationDescription.m_uiWindowNumber, m_hWindowHandle.glfwWindow);
#else
  m_pInputDevice = PL_DEFAULT_NEW(plStandardInputDevice, m_CreationDescription.m_uiWindowNumber, m_hWindowHandle);
#endif

  m_pInputDevice->SetClipMouseCursor(m_CreationDescription.m_bClipMouseCursor ? plMouseCursorClipMode::ClipToWindowImmediate : plMouseCursorClipMode::NoClip);
  m_pInputDevice->SetShowMouseCursor(m_CreationDescription.m_bShowMouseCursor);

  m_bInitialized = true;
  plLog::Success("Created glfw window successfully. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);

  return PL_SUCCESS;
}

plResult plWindow::Destroy()
{
  if (m_bInitialized)
  {
    PL_LOG_BLOCK("plWindow::Destroy");

    m_pInputDevice = nullptr;

#if PL_ENABLED(PL_PLATFORM_LINUX)
    PL_ASSERT_DEV(m_hWindowHandle.type == plWindowHandle::Type::GLFW, "GLFW handle expected");
    glfwDestroyWindow(m_hWindowHandle.glfwWindow);
#else
    glfwDestroyWindow(m_hWindowHandle);
#endif
    m_hWindowHandle = INVALID_INTERNAL_WINDOW_HANDLE_VALUE;

    m_bInitialized = false;
  }

  return PL_SUCCESS;
}

plResult plWindow::Resize(const plSizeU32& newWindowSize)
{
  if (!m_bInitialized)
    return PL_FAILURE;

#if PL_ENABLED(PL_PLATFORM_LINUX)
  PL_ASSERT_DEV(m_hWindowHandle.type == plWindowHandle::Type::GLFW, "Expected GLFW handle");
  glfwSetWindowSize(m_hWindowHandle.glfwWindow, newWindowSize.width, newWindowSize.height);
#else
  glfwSetWindowSize(m_hWindowHandle, newWindowSize.width, newWindowSize.height);
#endif
  PL_GLFW_RETURN_FAILURE_ON_ERROR();

  return PL_SUCCESS;
}

void plWindow::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;

  // Only run the global event processing loop for the main window.
  if (m_CreationDescription.m_uiWindowNumber == 0)
  {
    glfwPollEvents();
  }

#if PL_ENABLED(PL_PLATFORM_LINUX)
  PL_ASSERT_DEV(m_hWindowHandle.type == plWindowHandle::Type::GLFW, "Expected GLFW handle");
  if (glfwWindowShouldClose(m_hWindowHandle.glfwWindow))
  {
    Destroy().IgnoreResult();
  }
#else
  if (glfwWindowShouldClose(m_hWindowHandle))
  {
    Destroy().IgnoreResult();
  }
#endif
}

void plWindow::OnResize(const plSizeU32& newWindowSize)
{
  plLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}

void plWindow::SizeCallback(GLFWwindow* window, int width, int height)
{
  auto self = static_cast<plWindow*>(glfwGetWindowUserPointer(window));
  if (self && width > 0 && height > 0)
  {
    self->OnResize(plSizeU32(static_cast<plUInt32>(width), static_cast<plUInt32>(height)));
  }
}

void plWindow::PositionCallback(GLFWwindow* window, int xpos, int ypos)
{
  auto self = static_cast<plWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnWindowMove(xpos, ypos);
  }
}

void plWindow::CloseCallback(GLFWwindow* window)
{
  auto self = static_cast<plWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnClickClose();
  }
}

void plWindow::FocusCallback(GLFWwindow* window, int focused)
{
  auto self = static_cast<plWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnFocus(focused ? true : false);
  }
}

void plWindow::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  auto self = static_cast<plWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnKey(key, scancode, action, mods);
  }
}

void plWindow::CharacterCallback(GLFWwindow* window, unsigned int codepoint)
{
  auto self = static_cast<plWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnCharacter(codepoint);
  }
}

void plWindow::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
  auto self = static_cast<plWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnCursorPosition(xpos, ypos);
  }
}

void plWindow::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
  auto self = static_cast<plWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnMouseButton(button, action, mods);
  }
}

void plWindow::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
  auto self = static_cast<plWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnScroll(xoffset, yoffset);
  }
}

plWindowHandle plWindow::GetNativeWindowHandle() const
{
#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
  return plMinWindows::FromNative<HWND>(glfwGetWin32Window(m_hWindowHandle));
#else
  return m_hWindowHandle;
#endif
}
