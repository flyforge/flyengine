#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/ThreadUtils.h>

static void OSFileEventHandler(const plOSFile::EventData& e)
{
  if (!plTelemetry::IsConnectedToClient())
    return;

  plTelemetryMessage Msg;
  Msg.GetWriter() << e.m_iFileID;

  switch (e.m_EventType)
  {
    case plOSFile::EventType::FileOpen:
    {
      Msg.SetMessageID('FILE', 'OPEN');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << (plUInt8)e.m_FileMode;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case plOSFile::EventType::FileRead:
    {
      Msg.SetMessageID('FILE', 'READ');
      Msg.GetWriter() << e.m_uiBytesAccessed;
    }
    break;

    case plOSFile::EventType::FileWrite:
    {
      Msg.SetMessageID('FILE', 'WRIT');
      Msg.GetWriter() << e.m_uiBytesAccessed;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case plOSFile::EventType::FileClose:
    {
      Msg.SetMessageID('FILE', 'CLOS');
    }
    break;

    case plOSFile::EventType::FileExists:
    case plOSFile::EventType::DirectoryExists:
    {
      Msg.SetMessageID('FILE', 'EXST');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case plOSFile::EventType::FileDelete:
    {
      Msg.SetMessageID('FILE', ' DEL');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case plOSFile::EventType::MakeDir:
    {
      Msg.SetMessageID('FILE', 'CDIR');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case plOSFile::EventType::FileCopy:
    {
      Msg.SetMessageID('FILE', 'COPY');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_sFile2;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case plOSFile::EventType::FileStat:
    {
      Msg.SetMessageID('FILE', 'STAT');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case plOSFile::EventType::FileCasing:
    {
      Msg.SetMessageID('FILE', 'CASE');
      Msg.GetWriter() << e.m_sFile;
      Msg.GetWriter() << e.m_bSuccess;
    }
    break;

    case plOSFile::EventType::None:
      break;
  }

  plUInt8 uiThreadType = 0;

  if (plThreadUtils::IsMainThread())
    uiThreadType = 1 << 0;
  else if (plTaskSystem::GetCurrentThreadWorkerType() == plWorkerThreadType::FileAccess)
    uiThreadType = 1 << 1;
  else
    uiThreadType = 1 << 2;

  Msg.GetWriter() << e.m_Duration.GetSeconds();
  Msg.GetWriter() << uiThreadType;

  plTelemetry::Broadcast(plTelemetry::Reliable, Msg);
}

void AddOSFileEventHandler()
{
  plOSFile::AddEventHandler(OSFileEventHandler);
}

void RemoveOSFileEventHandler()
{
  plOSFile::RemoveEventHandler(OSFileEventHandler);
}



PLASMA_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_OSFile);
