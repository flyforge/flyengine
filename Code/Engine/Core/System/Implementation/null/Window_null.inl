#include <Core/System/Window.h>

plResult plWindow::Initialize()
{
  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return PLASMA_FAILURE;
}

plResult plWindow::Destroy()
{
  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return PLASMA_FAILURE;
}

plResult plWindow::Resize(const plSizeU32& newWindowSize)
{
  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return PLASMA_FAILURE;
}

void plWindow::ProcessWindowMessages()
{
  PLASMA_ASSERT_NOT_IMPLEMENTED;
}

void plWindow::OnResize(const plSizeU32& newWindowSize)
{
  PLASMA_ASSERT_NOT_IMPLEMENTED;
}

plWindowHandle plWindow::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}
