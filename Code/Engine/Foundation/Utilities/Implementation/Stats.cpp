#include <Foundation/FoundationPCH.h>

#include <Foundation/Utilities/Stats.h>

plMutex plStats::s_Mutex;
plStats::MapType plStats::s_Stats;
plStats::plEventStats plStats::s_StatsEvents;

void plStats::RemoveStat(plStringView sStatName)
{
  PL_LOCK(s_Mutex);

  MapType::Iterator it = s_Stats.Find(sStatName);

  if (!it.IsValid())
    return;

  s_Stats.Remove(it);

  StatsEventData e;
  e.m_EventType = StatsEventData::Remove;
  e.m_sStatName = sStatName;

  s_StatsEvents.Broadcast(e);
}

void plStats::SetStat(plStringView sStatName, const plVariant& value)
{
  PL_LOCK(s_Mutex);

  bool bExisted = false;
  auto it = s_Stats.FindOrAdd(sStatName, &bExisted);

  if (it.Value() == value)
    return;

  it.Value() = value;

  StatsEventData e;
  e.m_EventType = bExisted ? StatsEventData::Set : StatsEventData::Add;
  e.m_sStatName = sStatName;
  e.m_NewStatValue = value;

  s_StatsEvents.Broadcast(e);
}


