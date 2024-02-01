#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/MessageLoop_Fallback.h>
#include <Foundation/Communication/IpcChannel.h>

plMessageLoop_Fallback::plMessageLoop_Fallback() = default;

plMessageLoop_Fallback::~plMessageLoop_Fallback()
{
  StopUpdateThread();
}

void plMessageLoop_Fallback::WakeUp()
{
  // nothing to do
}

bool plMessageLoop_Fallback::WaitForMessages(plInt32 iTimeout, plIpcChannel* pFilter)
{
  // nothing to do

  if (iTimeout < 0)
  {
    // if timeout is 'indefinite' wait a little
    plThreadUtils::YieldTimeSlice();
  }

  return false;
}


