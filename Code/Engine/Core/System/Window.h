#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Strings/String.h>
#include <Foundation/System/PlatformFeatures.h>
#include <Foundation/Types/UniquePtr.h>

class plOpenDdlWriter;
class plOpenDdlReader;
class plOpenDdlReaderElement;

// Include the proper Input implementation to use
#if PLASMA_ENABLED(PLASMA_SUPPORTS_GLFW)
#  include <Core/System/Implementation/glfw/InputDevice_glfw.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#  include <Core/System/Implementation/Win/InputDevice_win32.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  include <Core/System/Implementation/uwp/InputDevice_uwp.h>
#else
#  include <Core/System/Implementation/null/InputDevice_null.h>
#endif

// Currently the following scenarios are possible
// - Windows native implementation, using HWND
// - GLFW on windows, using GLFWWindow* internally and HWND to pass windows around
// - GLFW / XCB on linux. Runtime uses GLFWWindow*. Editor uses xcb-window. Tagged union is passed around as window handle.

#if PLASMA_ENABLED(PLASMA_SUPPORTS_GLFW)

extern "C"
{
  typedef struct GLFWwindow GLFWwindow;
}

#  if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#    include <Foundation/Basics/Platform/Win/MinWindows.h>
using plWindowHandle = plMinWindows::HWND;
using plWindowInternalHandle = GLFWwindow*;
#    define INVALID_WINDOW_HANDLE_VALUE (plWindowHandle)(0)
#    define INVALID_INTERNAL_WINDOW_HANDLE_VALUE nullptr
#  elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)

extern "C"
{
  typedef struct xcb_connection_t xcb_connection_t;
}

struct plXcbWindowHandle
{
  xcb_connection_t* m_pConnection;
  plUInt32 m_Window;
};

struct plWindowHandle
{
  enum class Type
  {
    Invalid = 0,
    GLFW = 1, // Used by the runtime
    XCB = 2   // Used by the editor
  };

  Type type;
  union
  {
    GLFWwindow* glfwWindow;
    plXcbWindowHandle xcbWindow;
  };

  bool operator==(plWindowHandle& rhs)
  {
    if (type != rhs.type)
      return false;

    if (type == Type::GLFW)
    {
      return glfwWindow == rhs.glfwWindow;
    }
    else
    {
      // We don't compare the connection because we only want to know if we reference the same window.
      return xcbWindow.m_Window == rhs.xcbWindow.m_Window;
    }
  }
};

using plWindowInternalHandle = plWindowHandle;
#    define INVALID_WINDOW_HANDLE_VALUE \
      plWindowHandle {}
#  else
using plWindowHandle = GLFWwindow*;
using plWindowInternalHandle = GLFWwindow*;
#    define INVALID_WINDOW_HANDLE_VALUE (GLFWwindow*)(0)
#  endif

#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Basics/Platform/Win/MinWindows.h>
using plWindowHandle = plMinWindows::HWND;
using plWindowInternalHandle = plWindowHandle;
#  define INVALID_WINDOW_HANDLE_VALUE (plWindowHandle)(0)

#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)

using plWindowHandle = IUnknown*;
using plWindowInternalHandle = plWindowHandle;
#  define INVALID_WINDOW_HANDLE_VALUE nullptr

#else

using plWindowHandle = void*;
using plWindowInternalHandle = plWindowHandle;
#  define INVALID_WINDOW_HANDLE_VALUE nullptr

#endif

#ifndef INVALID_INTERNAL_WINDOW_HANDLE_VALUE
#  define INVALID_INTERNAL_WINDOW_HANDLE_VALUE INVALID_WINDOW_HANDLE_VALUE
#endif

/// \brief Base class of all window classes that have a client area and a native window handle.
class PLASMA_CORE_DLL plWindowBase
{
public:
  virtual ~plWindowBase() {}

  virtual plSizeU32 GetClientAreaSize() const = 0;
  virtual plSizeU32 GetRenderAreaSize() const = 0;
  virtual plWindowHandle GetNativeWindowHandle() const = 0;

  /// \brief Whether the window is a fullscreen window
  /// or should be one - some platforms may enforce this via the GALSwapchain)
  ///
  /// If bOnlyProperFullscreenMode, the caller accepts borderless windows that cover the entire screen as "fullscreen".
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const = 0;

  virtual void ProcessWindowMessages() = 0;

  virtual void AddReference() = 0;
  virtual void RemoveReference() = 0;
};

/// \brief Determines how the position and resolution for a window are picked
struct PLASMA_CORE_DLL plWindowMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    WindowFixedResolution,                ///< The resolution and size are what the user picked and will not be changed. The window will not be resizable.
    WindowResizable,                      ///< The resolution and size are what the user picked and will not be changed. Allows window resizing by the user.
    FullscreenBorderlessNativeResolution, ///< A borderless window, the position and resolution are taken from the monitor on which the
                                          ///< window shall appear.
    FullscreenFixedResolution,            ///< A fullscreen window using the user provided resolution. Tries to change the monitor resolution
                                          ///< accordingly.

    Default = WindowFixedResolution
  };

  /// \brief Returns whether the window covers an entire monitor. This includes borderless windows and proper fullscreen modes.
  static constexpr bool IsFullscreen(Enum e) { return e == FullscreenBorderlessNativeResolution || e == FullscreenFixedResolution; }
};

/// \brief Parameters for creating a window, such as position and resolution
struct PLASMA_CORE_DLL plWindowCreationDesc
{
  /// \brief Adjusts the position and size members, depending on the current value of m_WindowMode and m_iMonitor.
  ///
  /// For windowed mode, this does nothing.
  /// For fullscreen modes, the window position is taken from the given monitor.
  /// For borderless fullscreen mode, the window resolution is also taken from the given monitor.
  ///
  /// This function can only fail if plScreen::EnumerateScreens fails to enumerate the available screens.
  plResult AdjustWindowSizeAndPosition();

  /// Serializes the configuration to DDL.
  void SaveToDDL(plOpenDdlWriter& writer);

  /// Serializes the configuration to DDL.
  plResult SaveToDDL(plStringView sFile);

  /// Deserializes the configuration from DDL.
  void LoadFromDDL(const plOpenDdlReaderElement* pParentElement);

  /// Deserializes the configuration from DDL.
  plResult LoadFromDDL(plStringView sFile);


  /// The window title to be displayed.
  plString m_Title = "PlasmaEngine";

  /// Defines how the window size is determined.
  plEnum<plWindowMode> m_WindowMode;

  /// The monitor index is as given by plScreen::EnumerateScreens.
  /// -1 as the index means to pick the primary monitor.
  plInt8 m_iMonitor = -1;

  /// The virtual position of the window. Determines on which monitor the window ends up.
  plVec2I32 m_Position = plVec2I32(0x80000000, 0x80000000); // Magic number on windows that positions the window at a 'good default position'

  /// The pixel resolution of the window.
  plSizeU32 m_Resolution = plSizeU32(1280, 720);

  /// The pixel resolution of the render target.
  plSizeU32 m_RenderResolution = plSizeU32(1280, 720);

  /// The number of the window. This is mostly used for setting up the input system, which then reports
  /// different mouse positions for each window.
  plUInt8 m_uiWindowNumber = 0;

  /// Whether the mouse cursor should be trapped inside the window or not.
  /// \see plStandardInputDevice::SetClipMouseCursor
  bool m_bClipMouseCursor = true;

  /// Whether the mouse cursor should be visible or not.
  /// \see plStandardInputDevice::SetShowMouseCursor
  bool m_bShowMouseCursor = false;

  /// Whether the window is activated and focussed on Initialize()
  bool m_bSetForegroundOnInit = true;
};

/// \brief A simple abstraction for platform specific window creation.
///
/// Will handle basic message looping. Notable events can be listened to by overriding the corresponding callbacks.
/// You should call ProcessWindowMessages every frame to keep the window responsive.
/// Input messages will not be forwarded automatically. You can do so by overriding the OnWindowMessage function.
class PLASMA_CORE_DLL plWindow : public plWindowBase
{
public:
  /// \brief Creates empty window instance with standard settings
  ///
  /// You need to call Initialize to actually create a window.
  /// \see plWindow::Initialize
  plWindow();

  /// \brief Destroys the window if not already done.
  virtual ~plWindow();

  /// \brief Returns the currently active description struct.
  inline const plWindowCreationDesc& GetCreationDescription() const { return m_CreationDescription; }

  /// \brief Returns the size of the client area / ie. the window resolution.
  virtual plSizeU32 GetClientAreaSize() const override { return m_CreationDescription.m_Resolution; }

  /// \brief Returns the size of the render area.
  virtual plSizeU32 GetRenderAreaSize() const override { return m_CreationDescription.m_RenderResolution; }

  /// \brief Returns the platform specific window handle.
  virtual plWindowHandle GetNativeWindowHandle() const override;

  /// \brief Returns whether the window covers an entire monitor.
  ///
  /// If bOnlyProperFullscreenMode == false, this includes borderless windows.
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override
  {
    if (bOnlyProperFullscreenMode)
      return m_CreationDescription.m_WindowMode == plWindowMode::FullscreenFixedResolution;

    return plWindowMode::IsFullscreen(m_CreationDescription.m_WindowMode);
  }

  virtual void AddReference() override { m_iReferenceCount.Increment(); }
  virtual void RemoveReference() override { m_iReferenceCount.Decrement(); }


  /// \brief Runs the platform specific message pump.
  ///
  /// You should call ProcessWindowMessages every frame to keep the window responsive.
  virtual void ProcessWindowMessages() override;

  /// \brief Creates a new platform specific window with the current settings
  ///
  /// Will automatically call plWindow::Destroy if window is already initialized.
  ///
  /// \see plWindow::Destroy, plWindow::Initialize
  plResult Initialize();

  /// \brief Creates a new platform specific window with the given settings.
  ///
  /// Will automatically call plWindow::Destroy if window is already initialized.
  ///
  /// \param creationDescription
  ///   Struct with various settings for window creation. Will be saved internally for later lookup.
  ///
  /// \see plWindow::Destroy, plWindow::Initialize
  plResult Initialize(const plWindowCreationDesc& creationDescription)
  {
    m_CreationDescription = creationDescription;
    return Initialize();
  }

  /// \brief Gets if the window is up and running.
  inline bool IsInitialized() const { return m_bInitialized; }

  /// \brief Destroys the window.
  plResult Destroy();

  /// \brief Tries to resize the window.
  /// Override OnResize to get the actual new window size.
  plResult Resize(const plSizeU32& newWindowSize);

  /// \brief Called on window resize messages.
  ///
  /// \param newWindowSize
  ///   New window size in pixel.
  /// \see OnWindowMessage
  virtual void OnResize(const plSizeU32& newWindowSize);

  /// \brief Called when the window position is changed. Not possible on all OSes.
  virtual void OnWindowMove(const plInt32 newPosX, const plInt32 newPosY) {}

  /// \brief Called when the window gets focus or loses focus.
  virtual void OnFocus(bool bHasFocus) {}

  /// \brief Called when the close button of the window is clicked. Does nothing by default.
  virtual void OnClickClose() {}


#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  /// \brief Called on any window message.
  ///
  /// You can use this function for example to dispatch the message to another system.
  ///
  /// \remarks
  ///   Will be called <i>after</i> the On[...] callbacks!
  ///
  /// \see OnResizeMessage
  virtual void OnWindowMessage(plMinWindows::HWND hWnd, plMinWindows::UINT Msg, plMinWindows::WPARAM WParam, plMinWindows::LPARAM LParam);

#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX)

#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)

#elif PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)

#else
#  error "Missing code for plWindow on this platform!"
#endif

  /// \brief Returns the input device that is attached to this window and typically provides mouse / keyboard input.
  plStandardInputDevice* GetInputDevice() const { return m_pInputDevice.Borrow(); }

  /// \brief Returns a number that can be used as a window number in plWindowCreationDesc
  ///
  /// This number just increments every time an plWindow is created. It starts at zero.
  static plUInt8 GetNextUnusedWindowNumber();

protected:
  /// Description at creation time. plWindow will not update this in any method other than Initialize.
  /// \remarks That means that messages like Resize will also have no effect on this variable.
  plWindowCreationDesc m_CreationDescription;

private:
  bool m_bInitialized = false;

  plUniquePtr<plStandardInputDevice> m_pInputDevice;

  mutable plWindowInternalHandle m_hWindowHandle = plWindowInternalHandle();

#if PLASMA_ENABLED(PLASMA_SUPPORTS_GLFW)
  static void SizeCallback(GLFWwindow* window, int width, int height);
  static void PositionCallback(GLFWwindow* window, int xpos, int ypos);
  static void CloseCallback(GLFWwindow* window);
  static void FocusCallback(GLFWwindow* window, int focused);
  static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
  static void CharacterCallback(GLFWwindow* window, unsigned int codepoint);
  static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
  static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
  static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
#endif

  /// increased every time an plWindow is created, to be able to get a free window index easily
  static plUInt8 s_uiNextUnusedWindowNumber;
  plAtomicInteger32 m_iReferenceCount = 0;
};
