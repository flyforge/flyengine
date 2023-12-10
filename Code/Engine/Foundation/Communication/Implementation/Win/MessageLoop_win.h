#pragma once

#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Basics.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Communication/Implementation/MessageLoop.h>

class plIpcChannel;
struct IOContext;

class PLASMA_FOUNDATION_DLL plMessageLoop_win : public plMessageLoop
{
public:
  struct IOItem
  {
    PLASMA_DECLARE_POD_TYPE();

    plIpcChannel* pChannel;
    IOContext* pContext;
    DWORD uiBytesTransfered;
    DWORD uiError;
  };

public:
  plMessageLoop_win();
  ~plMessageLoop_win();

  HANDLE GetPort() const { return m_hPort; }

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(plInt32 iTimeout, plIpcChannel* pFilter) override;

  bool GetIOItem(plInt32 iTimeout, IOItem* pItem);
  bool ProcessInternalIOItem(const IOItem& item);
  bool MatchCompletedIOItem(plIpcChannel* pFilter, IOItem* pItem);

private:
  plDynamicArray<IOItem> m_CompletedIO;
  LONG m_iHaveWork = 0;
  HANDLE m_hPort = INVALID_HANDLE_VALUE;
};

#endif
