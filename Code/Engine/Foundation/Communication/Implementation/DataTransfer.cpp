#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/DataTransfer.h>

bool plDataTransfer::s_bInitialized = false;
plSet<plDataTransfer*> plDataTransfer::s_AllTransfers;

plDataTransferObject::plDataTransferObject(plDataTransfer& ref_belongsTo, plStringView sObjectName, plStringView sMimeType, plStringView sFileExtension)
  : m_BelongsTo(ref_belongsTo)
{
  m_bHasBeenTransferred = false;

  m_Msg.SetMessageID('TRAN', 'DATA');
  m_Msg.GetWriter() << ref_belongsTo.m_sDataName;
  m_Msg.GetWriter() << sObjectName;
  m_Msg.GetWriter() << sMimeType;
  m_Msg.GetWriter() << sFileExtension;
}

plDataTransferObject::~plDataTransferObject()
{
  PLASMA_ASSERT_DEV(m_bHasBeenTransferred, "The data transfer object has never been transmitted.");
}

void plDataTransferObject::Transmit()
{
  PLASMA_ASSERT_DEV(!m_bHasBeenTransferred, "The data transfer object has been transmitted already.");

  if (m_bHasBeenTransferred)
    return;

  m_bHasBeenTransferred = true;

  m_BelongsTo.Transfer(*this);
}

plDataTransfer::plDataTransfer()
{
  m_bTransferRequested = false;
  m_bEnabled = false;
}

plDataTransfer::~plDataTransfer()
{
  DisableDataTransfer();
}

void plDataTransfer::SendStatus()
{
  if (!plTelemetry::IsConnectedToClient())
    return;

  plTelemetryMessage msg;
  msg.GetWriter() << m_sDataName;

  if (m_bEnabled)
  {
    msg.SetMessageID('TRAN', 'ENBL');
  }
  else
  {
    msg.SetMessageID('TRAN', 'DSBL');
  }

  plTelemetry::Broadcast(plTelemetry::Reliable, msg);
}

void plDataTransfer::DisableDataTransfer()
{
  if (!m_bEnabled)
    return;

  plDataTransfer::s_AllTransfers.Remove(this);

  m_bEnabled = false;
  SendStatus();

  m_bTransferRequested = false;
  m_sDataName.Clear();
}

void plDataTransfer::EnableDataTransfer(plStringView sDataName)
{
  if (m_bEnabled && m_sDataName == sDataName)
    return;

  DisableDataTransfer();

  Initialize();

  plDataTransfer::s_AllTransfers.Insert(this);

  m_sDataName = sDataName;

  PLASMA_ASSERT_DEV(!m_sDataName.IsEmpty(), "The name for the data transfer must not be empty.");

  m_bEnabled = true;
  SendStatus();
}

void plDataTransfer::RequestDataTransfer()
{
  if (!m_bEnabled)
  {
    m_bTransferRequested = false;
    return;
  }

  plLog::Dev("Data Transfer Request: {0}", m_sDataName);

  m_bTransferRequested = true;

  OnTransferRequest();
}

bool plDataTransfer::IsTransferRequested(bool bReset)
{
  const bool bRes = m_bTransferRequested;

  if (bReset)
    m_bTransferRequested = false;

  return bRes;
}

void plDataTransfer::Transfer(plDataTransferObject& Object)
{
  if (!m_bEnabled)
    return;

  plTelemetry::Broadcast(plTelemetry::Reliable, Object.m_Msg);
}

void plDataTransfer::Initialize()
{
  if (s_bInitialized)
    return;

  s_bInitialized = true;

  plTelemetry::AddEventHandler(TelemetryEventsHandler);
  plTelemetry::AcceptMessagesForSystem('DTRA', true, TelemetryMessage, nullptr);
}

void plDataTransfer::TelemetryMessage(void* pPassThrough)
{
  plTelemetryMessage Msg;

  while (plTelemetry::RetrieveMessage('DTRA', Msg) == PLASMA_SUCCESS)
  {
    if (Msg.GetMessageID() == ' REQ')
    {
      plStringBuilder sName;
      Msg.GetReader() >> sName;

      plLog::Dev("Requested data transfer '{0}'", sName);

      for (auto it = s_AllTransfers.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Key()->m_sDataName == sName)
        {
          it.Key()->RequestDataTransfer();
          break;
        }
      }
    }
  }
}

void plDataTransfer::TelemetryEventsHandler(const plTelemetry::TelemetryEventData& e)
{
  if (!plTelemetry::IsConnectedToClient())
    return;

  switch (e.m_EventType)
  {
    case plTelemetry::TelemetryEventData::ConnectedToClient:
      SendAllDataTransfers();
      break;

    default:
      break;
  }
}

void plDataTransfer::SendAllDataTransfers()
{
  plTelemetryMessage msg;
  msg.SetMessageID('TRAN', ' CLR');
  plTelemetry::Broadcast(plTelemetry::Reliable, msg);

  for (auto it = s_AllTransfers.GetIterator(); it.IsValid(); ++it)
  {
    it.Key()->SendStatus();
  }
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_DataTransfer);
