#include <Core/CorePCH.h>

#include <Core/System/Window.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/System/Screen.h>

#if PL_ENABLED(PL_SUPPORTS_GLFW)
#  include <Core/System/Implementation/glfw/InputDevice_glfw.inl>
#  include <Core/System/Implementation/glfw/Window_glfw.inl>
#elif PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
#  include <Core/System/Implementation/Win/InputDevice_win32.inl>
#  include <Core/System/Implementation/Win/Window_win32.inl>
#elif PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
#  include <Core/System/Implementation/uwp/InputDevice_uwp.inl>
#  include <Core/System/Implementation/uwp/Window_uwp.inl>
#else
#  include <Core/System/Implementation/null/InputDevice_null.inl>
#  include <Core/System/Implementation/null/Window_null.inl>
#endif

plUInt8 plWindow::s_uiNextUnusedWindowNumber = 0;

plResult plWindowCreationDesc::AdjustWindowSizeAndPosition()
{
  if (m_WindowMode == plWindowMode::WindowFixedResolution || m_WindowMode == plWindowMode::WindowResizable)
    return PL_SUCCESS;

  plHybridArray<plScreenInfo, 2> screens;
  if (plScreen::EnumerateScreens(screens).Failed() || screens.IsEmpty())
    return PL_FAILURE;

  plInt32 iShowOnMonitor = m_iMonitor;

  if (iShowOnMonitor >= (plInt32)screens.GetCount())
    iShowOnMonitor = -1;

  const plScreenInfo* pScreen = nullptr;

  // this means 'pick the primary screen'
  if (iShowOnMonitor < 0)
  {
    pScreen = &screens[0];

    for (plUInt32 i = 0; i < screens.GetCount(); ++i)
    {
      if (screens[i].m_bIsPrimary)
      {
        pScreen = &screens[i];
        break;
      }
    }
  }
  else
  {
    pScreen = &screens[iShowOnMonitor];
  }

  m_Position.Set(pScreen->m_iOffsetX, pScreen->m_iOffsetY);

  if (m_WindowMode == plWindowMode::FullscreenBorderlessNativeResolution)
  {
    m_Resolution.width = pScreen->m_iResolutionX;
    m_Resolution.height = pScreen->m_iResolutionY;
  }
  else
  {
    // clamp the resolution to the native resolution ?
    // m_ClientAreaSize.width = plMath::Min<plUInt32>(m_ClientAreaSize.width, pScreen->m_iResolutionX);
    // m_ClientAreaSize.height= plMath::Min<plUInt32>(m_ClientAreaSize.height,pScreen->m_iResolutionY);
  }

  return PL_SUCCESS;
}

void plWindowCreationDesc::SaveToDDL(plOpenDdlWriter& ref_writer)
{
  ref_writer.BeginObject("WindowDesc");

  plOpenDdlUtils::StoreString(ref_writer, m_Title, "Title");

  switch (m_WindowMode.GetValue())
  {
    case plWindowMode::FullscreenBorderlessNativeResolution:
      plOpenDdlUtils::StoreString(ref_writer, "Borderless", "Mode");
      break;
    case plWindowMode::FullscreenFixedResolution:
      plOpenDdlUtils::StoreString(ref_writer, "Fullscreen", "Mode");
      break;
    case plWindowMode::WindowFixedResolution:
      plOpenDdlUtils::StoreString(ref_writer, "Window", "Mode");
      break;
    case plWindowMode::WindowResizable:
      plOpenDdlUtils::StoreString(ref_writer, "ResizableWindow", "Mode");
      break;
  }

  if (m_uiWindowNumber != 0)
    plOpenDdlUtils::StoreUInt8(ref_writer, m_uiWindowNumber, "Index");

  if (m_iMonitor >= 0)
    plOpenDdlUtils::StoreInt8(ref_writer, m_iMonitor, "Monitor");

  if (m_Position != plVec2I32(0x80000000, 0x80000000))
  {
    plOpenDdlUtils::StoreVec2I(ref_writer, m_Position, "Position");
  }

  plOpenDdlUtils::StoreVec2U(ref_writer, plVec2U32(m_Resolution.width, m_Resolution.height), "Resolution");

  plOpenDdlUtils::StoreBool(ref_writer, m_bClipMouseCursor, "ClipMouseCursor");
  plOpenDdlUtils::StoreBool(ref_writer, m_bShowMouseCursor, "ShowMouseCursor");
  plOpenDdlUtils::StoreBool(ref_writer, m_bSetForegroundOnInit, "SetForegroundOnInit");

  ref_writer.EndObject();
}


plResult plWindowCreationDesc::SaveToDDL(plStringView sFile)
{
  plFileWriter file;
  PL_SUCCEED_OR_RETURN(file.Open(sFile));

  plOpenDdlWriter writer;
  writer.SetOutputStream(&file);

  SaveToDDL(writer);

  return PL_SUCCESS;
}

void plWindowCreationDesc::LoadFromDDL(const plOpenDdlReaderElement* pParentElement)
{
  if (const plOpenDdlReaderElement* pDesc = pParentElement->FindChildOfType("WindowDesc"))
  {
    if (const plOpenDdlReaderElement* pTitle = pDesc->FindChildOfType(plOpenDdlPrimitiveType::String, "Title"))
      m_Title = pTitle->GetPrimitivesString()[0];

    if (const plOpenDdlReaderElement* pMode = pDesc->FindChildOfType(plOpenDdlPrimitiveType::String, "Mode"))
    {
      auto mode = pMode->GetPrimitivesString()[0];

      if (mode == "Borderless")
        m_WindowMode = plWindowMode::FullscreenBorderlessNativeResolution;
      else if (mode == "Fullscreen")
        m_WindowMode = plWindowMode::FullscreenFixedResolution;
      else if (mode == "Window")
        m_WindowMode = plWindowMode::WindowFixedResolution;
      else if (mode == "ResizableWindow")
        m_WindowMode = plWindowMode::WindowResizable;
    }

    if (const plOpenDdlReaderElement* pIndex = pDesc->FindChildOfType(plOpenDdlPrimitiveType::UInt8, "Index"))
    {
      m_uiWindowNumber = pIndex->GetPrimitivesUInt8()[0];
    }

    if (const plOpenDdlReaderElement* pMonitor = pDesc->FindChildOfType(plOpenDdlPrimitiveType::Int8, "Monitor"))
    {
      m_iMonitor = pMonitor->GetPrimitivesInt8()[0];
    }

    if (const plOpenDdlReaderElement* pPosition = pDesc->FindChild("Position"))
    {
      plOpenDdlUtils::ConvertToVec2I(pPosition, m_Position).IgnoreResult();
    }

    if (const plOpenDdlReaderElement* pPosition = pDesc->FindChild("Resolution"))
    {
      plVec2U32 res;
      plOpenDdlUtils::ConvertToVec2U(pPosition, res).IgnoreResult();
      m_Resolution.width = res.x;
      m_Resolution.height = res.y;
    }

    if (const plOpenDdlReaderElement* pClipMouseCursor = pDesc->FindChildOfType(plOpenDdlPrimitiveType::Bool, "ClipMouseCursor"))
      m_bClipMouseCursor = pClipMouseCursor->GetPrimitivesBool()[0];

    if (const plOpenDdlReaderElement* pShowMouseCursor = pDesc->FindChildOfType(plOpenDdlPrimitiveType::Bool, "ShowMouseCursor"))
      m_bShowMouseCursor = pShowMouseCursor->GetPrimitivesBool()[0];

    if (const plOpenDdlReaderElement* pSetForegroundOnInit = pDesc->FindChildOfType(plOpenDdlPrimitiveType::Bool, "SetForegroundOnInit"))
      m_bSetForegroundOnInit = pSetForegroundOnInit->GetPrimitivesBool()[0];
  }
}


plResult plWindowCreationDesc::LoadFromDDL(plStringView sFile)
{
  plFileReader file;
  PL_SUCCEED_OR_RETURN(file.Open(sFile));

  plOpenDdlReader reader;
  PL_SUCCEED_OR_RETURN(reader.ParseDocument(file));

  LoadFromDDL(reader.GetRootElement());

  return PL_SUCCESS;
}

plWindow::plWindow()
{
  ++s_uiNextUnusedWindowNumber;
}

plWindow::~plWindow()
{
  if (m_bInitialized)
  {
    Destroy().IgnoreResult();
  }
  PL_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call plGALDevice::WaitIdle before destroying a window.");
}

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
void plWindow::OnWindowMessage(plMinWindows::HWND hWnd, plMinWindows::UINT msg, plMinWindows::WPARAM wparam, plMinWindows::LPARAM lparam)
{
}
#endif

plUInt8 plWindow::GetNextUnusedWindowNumber()
{
  return s_uiNextUnusedWindowNumber;
}


