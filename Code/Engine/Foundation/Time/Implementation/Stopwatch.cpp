#include <Foundation/FoundationPCH.h>

#include <Foundation/Time/Stopwatch.h>

plStopwatch::plStopwatch()
{
  m_LastCheckpoint = plTime::Now();

  StopAndReset();
  Resume();
}

void plStopwatch::StopAndReset()
{
  m_TotalDuration = plTime::MakeZero();
  m_bRunning = false;
}

void plStopwatch::Resume()
{
  if (m_bRunning)
    return;

  m_bRunning = true;
  m_LastUpdate = plTime::Now();
}

void plStopwatch::Pause()
{
  if (!m_bRunning)
    return;

  m_bRunning = false;

  m_TotalDuration += plTime::Now() - m_LastUpdate;
}

plTime plStopwatch::GetRunningTotal() const
{
  if (m_bRunning)
  {
    const plTime tNow = plTime::Now();

    m_TotalDuration += tNow - m_LastUpdate;
    m_LastUpdate = tNow;
  }

  return m_TotalDuration;
}

plTime plStopwatch::Checkpoint()
{
  const plTime tNow = plTime::Now();

  const plTime tDiff = tNow - m_LastCheckpoint;
  m_LastCheckpoint = tNow;

  return tDiff;
}


