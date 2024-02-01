#pragma once

#include <Core/System/Window.h>

/// \brief A window class that expands a little on plWindow. Default type used by plGameState to create a window.
class PL_CORE_DLL plGameStateWindow : public plWindow
{
public:
  plGameStateWindow(const plWindowCreationDesc& windowdesc, plDelegate<void()> onClickClose = {});
  ~plGameStateWindow();

  void ResetOnClickClose(plDelegate<void()> onClickClose);

private:
  virtual void OnResize(const plSizeU32& newWindowSize) override;
  virtual void OnClickClose() override;

  plDelegate<void()> m_OnClickClose;
};
