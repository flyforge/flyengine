#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/ThreadSignal.h>

plThreadSignal::plThreadSignal(Mode mode /*= Mode::AutoReset*/)
{
  m_Mode = mode;
}

plThreadSignal::~plThreadSignal() = default;

void plThreadSignal::WaitForSignal() const
{
  PL_LOCK(m_ConditionVariable);

  while (!m_bSignalState)
  {
    m_ConditionVariable.UnlockWaitForSignalAndLock();
  }

  if (m_Mode == Mode::AutoReset)
  {
    m_bSignalState = false;
  }
}

plThreadSignal::WaitResult plThreadSignal::WaitForSignal(plTime timeout) const
{
  PL_LOCK(m_ConditionVariable);

  const plTime tStart = plTime::Now();
  plTime tElapsed = plTime::MakeZero();

  while (!m_bSignalState)
  {
    if (m_ConditionVariable.UnlockWaitForSignalAndLock(timeout - tElapsed) == plConditionVariable::WaitResult::Timeout)
    {
      return WaitResult::Timeout;
    }

    tElapsed = plTime::Now() - tStart;
    if (tElapsed >= timeout)
    {
      return WaitResult::Timeout;
    }
  }

  if (m_Mode == Mode::AutoReset)
  {
    m_bSignalState = false;
  }

  return WaitResult::Signaled;
}

void plThreadSignal::RaiseSignal()
{
  {
    PL_LOCK(m_ConditionVariable);
    m_bSignalState = true;
  }

  if (m_Mode == Mode::AutoReset)
  {
    // with auto-reset there is no need to wake up more than one
    m_ConditionVariable.SignalOne();
  }
  else
  {
    m_ConditionVariable.SignalAll();
  }
}

void plThreadSignal::ClearSignal()
{
  PL_LOCK(m_ConditionVariable);
  m_bSignalState = false;
}


