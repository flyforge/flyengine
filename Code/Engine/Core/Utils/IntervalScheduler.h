#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

struct PLASMA_CORE_DLL plUpdateRate
{
  using StorageType = plUInt8;

  enum Enum
  {
    EveryFrame,
    Max30fps,
    Max20fps,
    Max10fps,
    Max5fps,
    Max2fps,
    Max1fps,
    Never,

    Default = Max30fps
  };

  static plTime GetInterval(Enum updateRate);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plUpdateRate);

//////////////////////////////////////////////////////////////////////////

/// \brief Helper class to schedule work in intervals typically larger than the duration of one frame
///
/// Tries to maintain an even workload per frame and also keep the given interval for a work as best as possible.
/// A typical use case would be e.g. component update functions that don't need to be called every frame.
class PLASMA_CORE_DLL plIntervalSchedulerBase
{
protected:
  plIntervalSchedulerBase(plTime minInterval, plTime maxInterval);
  ~plIntervalSchedulerBase();

  plUInt32 GetHistogramIndex(plTime value);
  plTime GetHistogramSlotValue(plUInt32 uiIndex);

  static float GetRandomZeroToOne(int pos, plUInt32& seed);
  static plTime GetRandomTimeJitter(int pos, plUInt32& seed);

  plTime m_MinInterval;
  plTime m_MaxInterval;
  double m_fInvIntervalRange;

  plTime m_CurrentTime;

  plUInt32 m_uiSeed = 0;

  static constexpr plUInt32 HistogramSize = 32;
  plUInt32 m_Histogram[HistogramSize] = {};
  plTime m_HistogramSlotValues[HistogramSize] = {};
};

//////////////////////////////////////////////////////////////////////////

/// \brief \see plIntervalSchedulerBase
template <typename T>
class plIntervalScheduler : public plIntervalSchedulerBase
{
  using SUPER = plIntervalSchedulerBase;

public:
  PLASMA_ALWAYS_INLINE plIntervalScheduler(plTime minInterval = plTime::Milliseconds(1), plTime maxInterval = plTime::Seconds(1))
    : SUPER(minInterval, maxInterval)
  {
  }

  void AddOrUpdateWork(const T& work, plTime interval);
  void RemoveWork(const T& work);

  plTime GetInterval(const T& work) const;

  // reference to the work that should be run and time passed since this work has been last run.
  using RunWorkCallback = plDelegate<void(const T&, plTime)>;

  /// \brief Advances the scheduler by deltaTime and triggers runWorkCallback for each work that should be run during this update step.
  /// Since it is not possible to maintain the exact interval all the time the actual delta time for the work is also passed to runWorkCallback.
  void Update(plTime deltaTime, RunWorkCallback runWorkCallback);

  void Clear();

private:
  struct Data
  {
    T m_Work;
    plTime m_Interval;
    plTime m_DueTime;
    plTime m_LastScheduledTime;

    bool IsValid() const;
    void MarkAsInvalid();
  };

  using DataMap = plMap<plTime, Data>;
  DataMap m_Data;
  plHashTable<T, typename DataMap::Iterator> m_WorkIdToData;

  typename DataMap::Iterator InsertData(Data& data);
  plDynamicArray<typename DataMap::Iterator> m_ScheduledWork;
};

#include <Core/Utils/Implementation/IntervalScheduler_inl.h>
