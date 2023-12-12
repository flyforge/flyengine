#pragma once

#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/IpcChannel.h>

#  include <sys/stat.h>
#  include <sys/types.h>


class PLASMA_FOUNDATION_DLL plPipeChannel_linux : public plIpcChannel
{
public:
  plPipeChannel_linux(plStringView sAddress, Mode::Enum mode);
  ~plPipeChannel_linux();

private:
  friend class plMessageLoop;
  friend class plMessageLoop_linux;

  virtual void AddToMessageLoop(plMessageLoop* pMsgLoop) override;

  // All functions from here on down are run from worker thread only
  virtual void InternalConnect() override;
  virtual void InternalDisconnect() override;
  virtual void InternalSend() override;
  virtual bool NeedWakeup() const override;

  // These are called from MessageLoop_linux on OS events
  void AcceptIncomingConnection();
  void ProcessIncomingPackages();
  void ProcessConnectSuccessfull();

private:
  plString m_serverSocketPath;
  plString m_clientSocketPath;
  int m_serverSocketFd = -1;
  int m_clientSocketFd = -1;

  plUInt8 m_InputBuffer[4096];
  plAtomicBool m_Connecting = false;

  plUInt64 m_previousSendOffset = 0;
};
#endif
