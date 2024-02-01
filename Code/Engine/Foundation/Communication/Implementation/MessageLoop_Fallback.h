#pragma once

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/Implementation/MessageLoop.h>

class PL_FOUNDATION_DLL plMessageLoop_Fallback : public plMessageLoop
{
public:
  plMessageLoop_Fallback();
  ~plMessageLoop_Fallback();

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(plInt32 iTimeout, plIpcChannel* pFilter) override;

private:
};

