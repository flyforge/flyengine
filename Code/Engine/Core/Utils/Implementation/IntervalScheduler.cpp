#include <Core/CorePCH.h>

#include <Core/Utils/IntervalScheduler.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plUpdateRate, 1)
  PL_ENUM_CONSTANTS(plUpdateRate::EveryFrame)
  PL_ENUM_CONSTANTS(plUpdateRate::Max30fps, plUpdateRate::Max20fps, plUpdateRate::Max10fps)
  PL_ENUM_CONSTANTS(plUpdateRate::Max5fps, plUpdateRate::Max2fps, plUpdateRate::Max1fps)
  PL_ENUM_CONSTANTS(plUpdateRate::Never)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

static plTime s_Intervals[] = {
  plTime::MakeZero(),                  // EveryFrame
  plTime::MakeFromSeconds(1.0 / 30.0), // Max30fps
  plTime::MakeFromSeconds(1.0 / 20.0), // Max20fps
  plTime::MakeFromSeconds(1.0 / 10.0), // Max10fps
  plTime::MakeFromSeconds(1.0 / 5.0),  // Max5fps
  plTime::MakeFromSeconds(1.0 / 2.0),  // Max2fps
  plTime::MakeFromSeconds(1.0 / 1.0),  // Max1fps
};

static_assert(PL_ARRAY_SIZE(s_Intervals) == plUpdateRate::Max1fps + 1);

plTime plUpdateRate::GetInterval(Enum updateRate)
{
  return s_Intervals[updateRate];
}

//////////////////////////////////////////////////////////////////////////

plIntervalSchedulerBase::plIntervalSchedulerBase(plTime minInterval, plTime maxInterval)
  : m_MinInterval(minInterval)
  , m_MaxInterval(maxInterval)
{
  PL_ASSERT_DEV(m_MinInterval.IsPositive(), "Min interval must be greater than zero");
  PL_ASSERT_DEV(m_MaxInterval > m_MinInterval, "Max interval must be greater than min interval");

  m_fInvIntervalRange = 1.0 / (m_MaxInterval - m_MinInterval).GetSeconds();

  for (plUInt32 i = 0; i < HistogramSize; ++i)
  {
    m_HistogramSlotValues[i] = GetHistogramSlotValue(i);
  }
}

plIntervalSchedulerBase::~plIntervalSchedulerBase() = default;


PL_STATICLINK_FILE(Core, Core_Utils_Implementation_IntervalScheduler);
