
#include <Foundation/SimdMath/SimdRandom.h>

PL_ALWAYS_INLINE plUInt32 plIntervalSchedulerBase::GetHistogramIndex(plTime value)
{
  if (value.IsZero())
    return 0;

  constexpr plUInt32 maxSlotIndex = HistogramSize - 1;
  const double x = plMath::Max((value - m_MinInterval).GetSeconds() * m_fInvIntervalRange, 0.0);
  const double i = plMath::Sqrt(x) * (maxSlotIndex - 1) + 1;
  return plMath::Min(static_cast<plUInt32>(i), maxSlotIndex);
}

PL_ALWAYS_INLINE plTime plIntervalSchedulerBase::GetHistogramSlotValue(plUInt32 uiIndex)
{
  if (uiIndex == 0)
    return plTime::MakeZero();

  constexpr double norm = 1.0 / (HistogramSize - 2.0);
  const double x = (uiIndex - 1) * norm;
  return (x * x) * (m_MaxInterval - m_MinInterval) + m_MinInterval;
}

// static
PL_ALWAYS_INLINE float plIntervalSchedulerBase::GetRandomZeroToOne(int pos, plUInt32& seed)
{
  return plSimdRandom::FloatZeroToOne(plSimdVec4i(pos), plSimdVec4u(seed++)).x();
}

constexpr plTime s_JitterRange = plTime::MakeFromMicroseconds(10);

// static
PL_ALWAYS_INLINE plTime plIntervalSchedulerBase::GetRandomTimeJitter(int pos, plUInt32& seed)
{
  const float x = plSimdRandom::FloatZeroToOne(plSimdVec4i(pos), plSimdVec4u(seed++)).x();
  return s_JitterRange * (x * 2.0f - 1.0f);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
bool plIntervalScheduler<T>::Data::IsValid() const
{
  return m_Interval.IsZeroOrPositive();
}

template <typename T>
void plIntervalScheduler<T>::Data::MarkAsInvalid()
{
  m_Interval = plTime::MakeFromSeconds(-1);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
void plIntervalScheduler<T>::AddOrUpdateWork(const T& work, plTime interval)
{
  typename DataMap::Iterator it;
  if (m_WorkIdToData.TryGetValue(work, it))
  {
    auto& data = it.Value();
    plTime oldInterval = data.m_Interval;
    if (interval == oldInterval)
      return;

    data.MarkAsInvalid();

    const plUInt32 uiHistogramIndex = GetHistogramIndex(oldInterval);
    m_Histogram[uiHistogramIndex]--;
  }

  Data data;
  data.m_Work = work;
  data.m_Interval = plMath::Max(interval, plTime::MakeZero());
  data.m_DueTime = m_CurrentTime + GetRandomZeroToOne(m_Data.GetCount(), m_uiSeed) * data.m_Interval;
  data.m_LastScheduledTime = m_CurrentTime;

  m_WorkIdToData[work] = InsertData(data);

  const plUInt32 uiHistogramIndex = GetHistogramIndex(data.m_Interval);
  m_Histogram[uiHistogramIndex]++;
}

template <typename T>
void plIntervalScheduler<T>::RemoveWork(const T& work)
{
  typename DataMap::Iterator it;
  if (m_WorkIdToData.Remove(work, &it))
  {
    auto& data = it.Value();
    plTime oldInterval = data.m_Interval;
    data.MarkAsInvalid();

    const plUInt32 uiHistogramIndex = GetHistogramIndex(oldInterval);
    m_Histogram[uiHistogramIndex]--;
  }
}

template <typename T>
plTime plIntervalScheduler<T>::GetInterval(const T& work) const
{
  typename DataMap::Iterator it;
  PL_VERIFY(m_WorkIdToData.TryGetValue(work, it), "Entry not found");
  return it.Value().m_Interval;
}

template <typename T>
void plIntervalScheduler<T>::Update(plTime deltaTime, RunWorkCallback runWorkCallback)
{
  if (deltaTime.IsZeroOrNegative())
    return;

  m_CurrentTime += deltaTime;

  if (m_Data.IsEmpty())
    return;

  {
    double fNumWork = 0;
    for (plUInt32 i = 1; i < HistogramSize; ++i)
    {
      fNumWork += (1.0 / plMath::Max(m_HistogramSlotValues[i], deltaTime).GetSeconds()) * m_Histogram[i];
    }
    fNumWork *= deltaTime.GetSeconds();

    const float fRemainder = static_cast<float>(plMath::Fraction(fNumWork));
    const int pos = static_cast<int>(m_CurrentTime.GetNanoseconds());
    const plUInt32 extra = GetRandomZeroToOne(pos, m_uiSeed) < fRemainder ? 1 : 0;
    const plUInt32 uiScheduleCount = plMath::Min(static_cast<plUInt32>(fNumWork) + extra + m_Histogram[0], m_Data.GetCount());

    // schedule work
    {
      auto RunWork = [&](typename DataMap::Iterator it, plUInt32 uiIndex) {
        auto& data = it.Value();
        if (data.IsValid())
        {
          if (runWorkCallback.IsValid())
          {
            runWorkCallback(data.m_Work, m_CurrentTime - data.m_LastScheduledTime);
          }

          // add a little bit of random jitter so we don't end up with perfect timings that might collide with other work
          data.m_DueTime = m_CurrentTime + data.m_Interval + GetRandomTimeJitter(uiIndex, m_uiSeed);
          data.m_LastScheduledTime = m_CurrentTime;
        }

        m_ScheduledWork.PushBack(it);
      };

      auto it = m_Data.GetIterator();
      for (plUInt32 i = 0; i < uiScheduleCount; ++i, ++it)
      {
        RunWork(it, i);
      }

      // check if the next works have a zero interval if so execute them as well to fulfill the every frame guarantee
      plUInt32 uiNumExtras = 0;
      while (it.IsValid())
      {
        auto& data = it.Value();
        if (data.m_Interval.IsPositive())
          break;

        RunWork(it, uiNumExtras);

        ++uiNumExtras;
        ++it;
      }
    }

    // re-sort
    for (auto& it : m_ScheduledWork)
    {
      if (it.Value().IsValid())
      {
        // make a copy of data and re-insert at new due time
        Data data = it.Value();
        m_WorkIdToData[data.m_Work] = InsertData(data);
      }

      m_Data.Remove(it);
    }
    m_ScheduledWork.Clear();
  }
}

template <typename T>
void plIntervalScheduler<T>::Clear()
{
  m_CurrentTime = plTime::MakeZero();
  m_uiSeed = 0;
  plMemoryUtils::ZeroFill(m_Histogram, HistogramSize);

  m_Data.Clear();
  m_WorkIdToData.Clear();
}

template <typename T>
PL_FORCE_INLINE typename plIntervalScheduler<T>::DataMap::Iterator plIntervalScheduler<T>::InsertData(Data& data)
{
  // make sure that we have a unique due time since the map can't store multiple keys with the same value
  int pos = 0;
  while (m_Data.Contains(data.m_DueTime))
  {
    data.m_DueTime += GetRandomTimeJitter(pos++, m_uiSeed);
  }

  return m_Data.Insert(data.m_DueTime, data);
}
