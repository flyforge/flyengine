#pragma once

#if PLASMA_DISABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/Implementation/MessageLoop.h>

class PLASMA_FOUNDATION_DLL plMessageLoop_mobile : public plMessageLoop
{
public:
  plMessageLoop_mobile();
  ~plMessageLoop_mobile();

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(plInt32 iTimeout, plIpcChannel* pFilter) override;

private:
};

#endif
