
plEditorProcessViewWindow::~plEditorProcessViewWindow()
{
  plGALDevice::GetDefaultDevice()->WaitIdle();

  PLASMA_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call plGALDevice::WaitIdle before destroying a window.");
}

plResult plEditorProcessViewWindow::UpdateWindow(plWindowHandle hParentWindow, plUInt16 uiWidth, plUInt16 uiHeight)
{
  m_hWnd = hParentWindow;
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;

  return PLASMA_SUCCESS;
}
