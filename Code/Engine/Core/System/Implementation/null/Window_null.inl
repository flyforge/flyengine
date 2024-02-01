#include <Core/System/Window.h>

plResult plWindow::Initialize()
{
  PL_ASSERT_NOT_IMPLEMENTED;
  return PL_FAILURE;
}

plResult plWindow::Destroy()
{
  PL_ASSERT_NOT_IMPLEMENTED;
  return PL_FAILURE;
}

plResult plWindow::Resize(const plSizeU32& newWindowSize)
{
  PL_ASSERT_NOT_IMPLEMENTED;
  return PL_FAILURE;
}

void plWindow::ProcessWindowMessages()
{
  PL_ASSERT_NOT_IMPLEMENTED;
}

void plWindow::OnResize(const plSizeU32& newWindowSize)
{
  PL_ASSERT_NOT_IMPLEMENTED;
}

plWindowHandle plWindow::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}
