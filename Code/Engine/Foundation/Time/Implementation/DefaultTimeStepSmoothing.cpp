#include <Foundation/FoundationPCH.h>

#include <Foundation/Time/DefaultTimeStepSmoothing.h>

plDefaultTimeStepSmoothing::plDefaultTimeStepSmoothing()
{
  m_fLerpFactor = 0.2f;
}

void plDefaultTimeStepSmoothing::Reset(const plClock* pClock)
{
  m_LastTimeSteps.Clear();
}

plTime plDefaultTimeStepSmoothing::GetSmoothedTimeStep(plTime rawTimeStep, const plClock* pClock)
{
  rawTimeStep = plMath::Clamp(rawTimeStep * pClock->GetSpeed(), pClock->GetMinimumTimeStep(), pClock->GetMaximumTimeStep());

  if (m_LastTimeSteps.GetCount() < 10)
  {
    m_LastTimeSteps.PushBack(rawTimeStep);
    m_LastTimeStepTaken = rawTimeStep;
    return m_LastTimeStepTaken;
  }

  if (!m_LastTimeSteps.CanAppend(1))
    m_LastTimeSteps.PopFront(1);

  m_LastTimeSteps.PushBack(rawTimeStep);

  plStaticArray<plTime, 11> Sorted;
  Sorted.SetCountUninitialized(m_LastTimeSteps.GetCount());

  for (plUInt32 i = 0; i < m_LastTimeSteps.GetCount(); ++i)
    Sorted[i] = m_LastTimeSteps[i];

  Sorted.Sort();

  plUInt32 uiFirstSample = 2;
  plUInt32 uiLastSample = 8;

  plTime tAvg;

  for (plUInt32 i = uiFirstSample; i <= uiLastSample; ++i)
  {
    tAvg = tAvg + Sorted[i];
  }

  tAvg = tAvg / (double)((uiLastSample - uiFirstSample) + 1.0);


  m_LastTimeStepTaken = plMath::Lerp(m_LastTimeStepTaken, tAvg, m_fLerpFactor);

  return m_LastTimeStepTaken;
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_DefaultTimeStepSmoothing);
