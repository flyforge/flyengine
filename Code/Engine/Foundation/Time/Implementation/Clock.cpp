#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Time/Clock.h>

plClock::Event plClock::s_TimeEvents;
plClock* plClock::s_pGlobalClock = nullptr;

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Clock)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_BASESYSTEMS_STARTUP
  {
    plClock::s_pGlobalClock = new plClock("Global");
  }

PL_END_SUBSYSTEM_DECLARATION;

PL_BEGIN_STATIC_REFLECTED_TYPE(plClock, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Paused", GetPaused, SetPaused),
    PL_ACCESSOR_PROPERTY("Speed", GetSpeed, SetSpeed),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(GetGlobalClock),
    PL_SCRIPT_FUNCTION_PROPERTY(GetAccumulatedTime),
    PL_SCRIPT_FUNCTION_PROPERTY(GetTimeDiff)
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

plClock::plClock(plStringView sName)
{
  SetClockName(sName);

  Reset(true);
}

void plClock::Reset(bool bEverything)
{
  if (bEverything)
  {
    m_pTimeStepSmoother = nullptr;
    m_MinTimeStep = plTime::MakeFromSeconds(0.001); // 1000 FPS
    m_MaxTimeStep = plTime::MakeFromSeconds(0.1);   //   10 FPS, many simulations will be instable at that rate already
    m_FixedTimeStep = plTime::MakeFromSeconds(0.0);
  }

  m_AccumulatedTime = plTime::MakeFromSeconds(0.0);
  m_fSpeed = 1.0;
  m_bPaused = false;

  // this is to prevent having a time difference of zero (which might not work with some code)
  // in case the next Update() call is done right after this
  m_LastTimeUpdate = plTime::Now() - m_MinTimeStep;
  m_LastTimeDiff = m_MinTimeStep;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

void plClock::Update()
{
  const plTime tNow = plTime::Now();
  const plTime tDiff = tNow - m_LastTimeUpdate;
  m_LastTimeUpdate = tNow;

  if (m_bPaused)
  {
    // no change during pause
    m_LastTimeDiff = plTime::MakeFromSeconds(0.0);
  }
  else if (m_FixedTimeStep > plTime::MakeFromSeconds(0.0))
  {
    // scale the time step by the speed factor
    m_LastTimeDiff = m_FixedTimeStep * m_fSpeed;
  }
  else
  {
    // in variable time step mode, apply the time step smoother, if available
    if (m_pTimeStepSmoother)
      m_LastTimeDiff = m_pTimeStepSmoother->GetSmoothedTimeStep(tDiff, this);
    else
    {
      // scale the time step by the speed factor
      // and make sure the time step does not leave the predetermined bounds
      m_LastTimeDiff = plMath::Clamp(tDiff * m_fSpeed, m_MinTimeStep, m_MaxTimeStep);
    }
  }

  m_AccumulatedTime += m_LastTimeDiff;

  EventData ed;
  ed.m_sClockName = m_sName;
  ed.m_RawTimeStep = tDiff;
  ed.m_SmoothedTimeStep = m_LastTimeDiff;

  s_TimeEvents.Broadcast(ed);
}

void plClock::SetAccumulatedTime(plTime t)
{
  m_AccumulatedTime = t;

  // this is to prevent having a time difference of zero (which might not work with some code)
  // in case the next Update() call is done right after this
  m_LastTimeUpdate = plTime::Now() - plTime::MakeFromSeconds(0.01);
  m_LastTimeDiff = plTime::MakeFromSeconds(0.01);
}

void plClock::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 1;

  inout_stream << uiVersion;
  inout_stream << m_AccumulatedTime;
  inout_stream << m_LastTimeDiff;
  inout_stream << m_FixedTimeStep;
  inout_stream << m_MinTimeStep;
  inout_stream << m_MaxTimeStep;
  inout_stream << m_fSpeed;
  inout_stream << m_bPaused;
}

void plClock::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion == 1, "Wrong version for plClock: {0}", uiVersion);

  inout_stream >> m_AccumulatedTime;
  inout_stream >> m_LastTimeDiff;
  inout_stream >> m_FixedTimeStep;
  inout_stream >> m_MinTimeStep;
  inout_stream >> m_MaxTimeStep;
  inout_stream >> m_fSpeed;
  inout_stream >> m_bPaused;

  // make sure we continue properly
  m_LastTimeUpdate = plTime::Now() - m_MinTimeStep;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}



PL_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Clock);
