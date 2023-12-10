
#pragma once

#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/Implementation/MessageLoop.h>
#  include <Foundation/Threading/Mutex.h>

#  include <poll.h>

class plIpcChannel;
class plPipeChannel_linux;

PLASMA_DEFINE_AS_POD_TYPE(struct pollfd);

class PLASMA_FOUNDATION_DLL plMessageLoop_linux : public plMessageLoop
{
public:
  plMessageLoop_linux();
  ~plMessageLoop_linux();

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(plInt32 iTimeout, plIpcChannel* pFilter) override;

private:
  friend class plPipeChannel_linux;

  enum class WaitType
  {
    Accept,
    IncomingMessage,
    Connect,
    Send
  };

  void RegisterWait(plPipeChannel_linux* pChannel, WaitType type, int fd);
  void RemovePendingWaits(plPipeChannel_linux* pChannel);

private:
  struct WaitInfo
  {
    PLASMA_DECLARE_POD_TYPE();

    plPipeChannel_linux* m_pChannel;
    WaitType m_type;
  };

  // m_waitInfos and m_pollInfos are alway the same size.
  // related information is stored at the same index.
  plHybridArray<WaitInfo, 16> m_waitInfos;
  plHybridArray<struct pollfd, 16> m_pollInfos;
  plMutex m_pollMutex;
  plAtomicInteger32 m_numPendingPollModifications = 0;
  int m_wakeupPipeReadEndFd = -1;
  int m_wakeupPipeWriteEndFd = -1;
};

#endif
