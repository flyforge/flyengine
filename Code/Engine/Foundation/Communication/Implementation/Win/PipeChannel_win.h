#pragma once

#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Basics.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Communication/IpcChannel.h>

struct IOContext
{
  OVERLAPPED Overlapped;  ///< Must be first field in class so we can do a reinterpret cast from *Overlapped to *IOContext.
  plIpcChannel* pChannel; ///< Owner of this IOContext.
};

class PLASMA_FOUNDATION_DLL plPipeChannel_win : public plIpcChannel
{
public:
  plPipeChannel_win(plStringView sAddress, Mode::Enum mode);
  ~plPipeChannel_win();

private:
  friend class plMessageLoop;
  friend class plMessageLoop_win;

  bool CreatePipe(plStringView sAddress);

  virtual void AddToMessageLoop(plMessageLoop* pMsgLoop) override;

  // All functions from here on down are run from worker thread only
  virtual void InternalConnect() override;
  virtual void InternalDisconnect() override;
  virtual void InternalSend() override;
  virtual bool NeedWakeup() const override;

  bool ProcessConnection();
  bool ProcessIncomingMessages(DWORD uiBytesRead);
  bool ProcessOutgoingMessages(DWORD uiBytesWritten);


protected:
  void OnIOCompleted(IOContext* pContext, DWORD uiBytesTransfered, DWORD uiError);

private:
  struct State
  {
    explicit State(plPipeChannel_win* pChannel);
    ~State();
    IOContext Context;
    plAtomicInteger32 IsPending = false; ///< Whether an async operation is in process.
  };

  enum Constants
  {
    BUFFER_SIZE = 4096,
  };

  // Shared data
  State m_InputState;
  State m_OutputState;

  // Setup in ctor
  HANDLE m_hPipeHandle = INVALID_HANDLE_VALUE;

  // Only accessed from worker thread
  plUInt8 m_InputBuffer[BUFFER_SIZE];
};

#endif
