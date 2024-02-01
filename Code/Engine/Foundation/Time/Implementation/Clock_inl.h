#pragma once

#include <Foundation/Time/Clock.h>

inline void plClock::SetClockName(plStringView sName)
{
  m_sName = sName;
}

inline plStringView plClock::GetClockName() const
{
  return m_sName;
}

inline void plClock::SetTimeStepSmoothing(plTimeStepSmoothing* pSmoother)
{
  m_pTimeStepSmoother = pSmoother;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

inline plTimeStepSmoothing* plClock::GetTimeStepSmoothing() const
{
  return m_pTimeStepSmoother;
}

inline void plClock::SetPaused(bool bPaused)
{
  m_bPaused = bPaused;

  // when we enter a pause, inform the time step smoother to throw away his statistics
  if (bPaused && m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

inline bool plClock::GetPaused() const
{
  return m_bPaused;
}

inline plTime plClock::GetFixedTimeStep() const
{
  return m_FixedTimeStep;
}

inline plTime plClock::GetAccumulatedTime() const
{
  return m_AccumulatedTime;
}

inline plTime plClock::GetTimeDiff() const
{
  return m_LastTimeDiff;
}

inline double plClock::GetSpeed() const
{
  return m_fSpeed;
}

inline void plClock::SetMinimumTimeStep(plTime min)
{
  PL_ASSERT_DEV(min >= plTime::MakeFromSeconds(0.0), "Time flows in one direction only.");

  m_MinTimeStep = min;
}

inline void plClock::SetMaximumTimeStep(plTime max)
{
  PL_ASSERT_DEV(max >= plTime::MakeFromSeconds(0.0), "Time flows in one direction only.");

  m_MaxTimeStep = max;
}

inline plTime plClock::GetMinimumTimeStep() const
{
  return m_MinTimeStep;
}

inline plTime plClock::GetMaximumTimeStep() const
{
  return m_MaxTimeStep;
}

inline void plClock::SetFixedTimeStep(plTime diff)
{
  PL_ASSERT_DEV(m_FixedTimeStep.GetSeconds() >= 0.0, "Fixed Time Stepping cannot reverse time!");

  m_FixedTimeStep = diff;
}

inline void plClock::SetSpeed(double fFactor)
{
  PL_ASSERT_DEV(fFactor >= 0.0, "Time cannot run backwards.");

  m_fSpeed = fFactor;
}
