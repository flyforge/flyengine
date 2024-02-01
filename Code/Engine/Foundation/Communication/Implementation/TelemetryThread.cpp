#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/Thread.h>

class plTelemetryThread : public plThread
{
public:
  plTelemetryThread()
    : plThread("plTelemetryThread")
  {
    m_bKeepRunning = true;
  }

  volatile bool m_bKeepRunning;

private:
  virtual plUInt32 Run()
  {
    plTime LastPing;

    while (m_bKeepRunning)
    {
      plTelemetry::UpdateNetwork();

      // Send a Ping every once in a while
      if (plTelemetry::s_ConnectionMode == plTelemetry::Client)
      {
        plTime tNow = plTime::Now();

        if (tNow - LastPing > plTime::MakeFromMilliseconds(500))
        {
          LastPing = tNow;

          plTelemetry::UpdateServerPing();
        }
      }

      plThreadUtils::Sleep(plTime::MakeFromMilliseconds(10));
    }

    return 0;
  }
};

static plTelemetryThread* g_pBroadcastThread = nullptr;
plMutex plTelemetry::s_TelemetryMutex;


plMutex& plTelemetry::GetTelemetryMutex()
{
  return s_TelemetryMutex;
}

void plTelemetry::StartTelemetryThread()
{
  if (!g_pBroadcastThread)
  {
    g_pBroadcastThread = PL_DEFAULT_NEW(plTelemetryThread);
    g_pBroadcastThread->Start();
  }
}

void plTelemetry::StopTelemetryThread()
{
  if (g_pBroadcastThread)
  {
    g_pBroadcastThread->m_bKeepRunning = false;
    g_pBroadcastThread->Join();

    PL_DEFAULT_DELETE(g_pBroadcastThread);
  }
}


