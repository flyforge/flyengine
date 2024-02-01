#include <Foundation/FoundationPCH.h>

#include <Foundation/Tracks/EventTrack.h>

plEventTrack::plEventTrack() = default;

plEventTrack::~plEventTrack() = default;

void plEventTrack::Clear()
{
  m_Events.Clear();
  m_ControlPoints.Clear();
}

bool plEventTrack::IsEmpty() const
{
  return m_ControlPoints.IsEmpty();
}

void plEventTrack::AddControlPoint(plTime time, plStringView sEvent)
{
  m_bSort = true;

  const plUInt32 uiNumEvents = m_Events.GetCount();

  auto& cp = m_ControlPoints.ExpandAndGetRef();
  cp.m_Time = time;

  // search for existing event
  {
    plTempHashedString tmp(sEvent);

    for (plUInt32 i = 0; i < uiNumEvents; ++i)
    {
      if (m_Events[i] == tmp)
      {
        cp.m_uiEvent = i;
        return;
      }
    }
  }

  // not found -> add event name
  {
    cp.m_uiEvent = uiNumEvents;

    plHashedString hs;
    hs.Assign(sEvent);

    m_Events.PushBack(hs);
  }
}

plUInt32 plEventTrack::FindControlPointAfter(plTime x) const
{
  // searches for a control point after OR AT x

  PL_ASSERT_DEBUG(!m_ControlPoints.IsEmpty(), "");

  plUInt32 uiLowIdx = 0;
  plUInt32 uiHighIdx = m_ControlPoints.GetCount() - 1;

  // do a binary search to reduce the search space
  while (uiHighIdx - uiLowIdx > 8)
  {
    const plUInt32 uiMidIdx = uiLowIdx + ((uiHighIdx - uiLowIdx) >> 1); // lerp

    if (m_ControlPoints[uiMidIdx].m_Time >= x)
      uiHighIdx = uiMidIdx;
    else
      uiLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (plUInt32 idx = uiLowIdx; idx <= uiHighIdx; ++idx)
  {
    if (m_ControlPoints[idx].m_Time >= x)
    {
      return idx;
    }
  }

  PL_ASSERT_DEBUG(uiHighIdx + 1 == m_ControlPoints.GetCount(), "Unexpected event track entry index");
  return m_ControlPoints.GetCount();
}

plInt32 plEventTrack::FindControlPointBefore(plTime x) const
{
  // searches for a control point before OR AT x

  PL_ASSERT_DEBUG(!m_ControlPoints.IsEmpty(), "");

  plInt32 iLowIdx = 0;
  plInt32 iHighIdx = (plInt32)m_ControlPoints.GetCount() - 1;

  // do a binary search to reduce the search space
  while (iHighIdx - iLowIdx > 8)
  {
    const plInt32 uiMidIdx = iLowIdx + ((iHighIdx - iLowIdx) >> 1); // lerp

    if (m_ControlPoints[uiMidIdx].m_Time >= x)
      iHighIdx = uiMidIdx;
    else
      iLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (plInt32 idx = iHighIdx; idx >= iLowIdx; --idx)
  {
    if (m_ControlPoints[idx].m_Time <= x)
    {
      return idx;
    }
  }

  PL_ASSERT_DEBUG(iLowIdx == 0, "Unexpected event track entry index");
  return -1;
}

void plEventTrack::Sample(plTime rangeStart, plTime rangeEnd, plDynamicArray<plHashedString>& out_events) const
{
  if (m_ControlPoints.IsEmpty())
    return;

  if (m_bSort)
  {
    m_bSort = false;
    m_ControlPoints.Sort();
  }

  if (rangeStart <= rangeEnd)
  {
    plUInt32 curCpIdx = FindControlPointAfter(rangeStart);

    const plUInt32 uiNumCPs = m_ControlPoints.GetCount();
    while (curCpIdx < uiNumCPs && m_ControlPoints[curCpIdx].m_Time < rangeEnd)
    {
      const plHashedString& sEvent = m_Events[m_ControlPoints[curCpIdx].m_uiEvent];

      out_events.PushBack(sEvent);

      ++curCpIdx;
    }
  }
  else
  {
    plInt32 curCpIdx = FindControlPointBefore(rangeStart);

    while (curCpIdx >= 0 && m_ControlPoints[curCpIdx].m_Time > rangeEnd)
    {
      const plHashedString& sEvent = m_Events[m_ControlPoints[curCpIdx].m_uiEvent];

      out_events.PushBack(sEvent);

      --curCpIdx;
    }
  }
}

void plEventTrack::Save(plStreamWriter& inout_stream) const
{
  if (m_bSort)
  {
    m_bSort = false;
    m_ControlPoints.Sort();
  }

  plUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_Events.GetCount();
  for (const plHashedString& name : m_Events)
  {
    inout_stream << name.GetString();
  }

  inout_stream << m_ControlPoints.GetCount();
  for (const ControlPoint& cp : m_ControlPoints)
  {
    inout_stream << cp.m_Time;
    inout_stream << cp.m_uiEvent;
  }
}

void plEventTrack::Load(plStreamReader& inout_stream)
{
  // don't rely on the data being sorted
  m_bSort = true;

  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion == 1, "Invalid event track version {0}", uiVersion);

  plUInt32 count = 0;
  plStringBuilder tmp;

  inout_stream >> count;
  m_Events.SetCount(count);
  for (plHashedString& name : m_Events)
  {
    inout_stream >> tmp;
    name.Assign(tmp);
  }

  inout_stream >> count;
  m_ControlPoints.SetCount(count);
  for (ControlPoint& cp : m_ControlPoints)
  {
    inout_stream >> cp.m_Time;
    inout_stream >> cp.m_uiEvent;
  }
}


