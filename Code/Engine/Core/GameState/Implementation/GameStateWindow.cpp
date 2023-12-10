#include <Core/CorePCH.h>

#include <Core/GameState/GameStateWindow.h>

plGameStateWindow::plGameStateWindow(const plWindowCreationDesc& windowdesc, plDelegate<void()> onClickClose)
  : m_OnClickClose(onClickClose)
{
  m_CreationDescription = windowdesc;
  m_CreationDescription.AdjustWindowSizeAndPosition().IgnoreResult();

  Initialize().IgnoreResult();
}

plGameStateWindow::~plGameStateWindow()
{
  Destroy().IgnoreResult();
}


void plGameStateWindow::ResetOnClickClose(plDelegate<void()> onClickClose)
{
  m_OnClickClose = onClickClose;
}

void plGameStateWindow::OnClickClose()
{
  if (m_OnClickClose.IsValid())
  {
    m_OnClickClose();
  }
}

void plGameStateWindow::OnResize(const plSizeU32& newWindowSize)
{
  plLog::Info("Resolution changed to {0} * {1}", newWindowSize.width, newWindowSize.height);

  m_CreationDescription.m_Resolution = newWindowSize;
}



PLASMA_STATICLINK_FILE(Core, Core_GameState_Implementation_GameStateWindow);
