#include <Foundation/FoundationPCH.h>

#if PLASMA_DISABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Communication/Implementation/Mobile/MessageLoop_mobile.h>
#  include <Foundation/Communication/IpcChannel.h>

plMessageLoop_mobile::plMessageLoop_mobile() {}

plMessageLoop_mobile::~plMessageLoop_mobile()
{
  StopUpdateThread();
}

void plMessageLoop_mobile::WakeUp()
{
  // nothing to do
}

bool plMessageLoop_mobile::WaitForMessages(plInt32 iTimeout, plIpcChannel* pFilter)
{
  // nothing to do

  if (iTimeout < 0)
  {
    // if timeout is 'indefinite' wait a little
    plThreadUtils::YieldTimeSlice();
  }

  return false;
}

#endif



PLASMA_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Mobile_MessageLoop_mobile);
