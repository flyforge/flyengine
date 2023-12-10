#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>

plResult plEditorProcessCommunicationChannel::StartClientProcess(
  const char* szProcess, const QStringList& args, bool bRemote, const plRTTI* pFirstAllowedMessageType, plUInt32 uiMemSize)
{
  PLASMA_LOG_BLOCK("plProcessCommunicationChannel::StartClientProcess");

  PLASMA_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");
  PLASMA_ASSERT_DEV(m_pClientProcess == nullptr, "ProcessCommunication object already in use");

  m_pFirstAllowedMessageType = pFirstAllowedMessageType;

  static plUInt64 uiUniqueHash = 0;
  plOsProcessID PID = plProcess::GetCurrentProcessID();
  uiUniqueHash = plHashingUtils::xxHash64(&PID, sizeof(PID), uiUniqueHash);
  plTime time = plTime::Now();
  uiUniqueHash = plHashingUtils::xxHash64(&time, sizeof(time), uiUniqueHash);
  plStringBuilder sMemName;
  sMemName.Format("{0}", plArgU(uiUniqueHash, 16, false, 16, true));
  ++uiUniqueHash;

  if (bRemote)
  {
    m_pChannel = plIpcChannel::CreateNetworkChannel("172.16.80.3:1050", plIpcChannel::Mode::Client);
  }
  else
  {
    m_pChannel = plIpcChannel::CreatePipeChannel(sMemName, plIpcChannel::Mode::Server);
  }
  m_pProtocol = PLASMA_DEFAULT_NEW(plIpcProcessMessageProtocol, m_pChannel.Borrow());
  m_pProtocol->m_MessageEvent.AddEventHandler(plMakeDelegate(&plProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();
  for (plUInt32 i = 0; i < 100; i++)
  {
    if (m_pChannel->GetConnectionState() == plIpcChannel::ConnectionState::Connecting)
      break;

    plThreadUtils::Sleep(plTime::MakeFromMilliseconds(10));
  }
  if (m_pChannel->GetConnectionState() != plIpcChannel::ConnectionState::Connecting)
  {
    plLog::Error("Failed to start IPC server");
    CloseConnection();
    return PLASMA_FAILURE;
  }

  plStringBuilder sPath = szProcess;

  if (!sPath.IsAbsolutePath())
  {
    sPath = plOSFile::GetApplicationDirectory();
    sPath.AppendPath(szProcess);
  }

  sPath.MakeCleanPath();

  plStringBuilder sPID;
  plConversionUtils::ToString((plUInt64)QCoreApplication::applicationPid(), sPID);

  QStringList arguments;
  arguments << "-IPC";
  arguments << QLatin1String(sMemName.GetData());
  arguments << "-PID";
  arguments << sPID.GetData();
  arguments.append(args);

  m_pClientProcess = new QProcess();

  if (!bRemote)
  {
    m_pClientProcess->start(QString::fromUtf8(sPath.GetData()), arguments, QIODevice::OpenModeFlag::NotOpen);

    if (!m_pClientProcess->waitForStarted())
    {
      delete m_pClientProcess;
      m_pClientProcess = nullptr;

      m_pProtocol.Clear();
      m_pChannel.Clear();

      plLog::Error("Failed to start process '{0}'", sPath);
      return PLASMA_FAILURE;
    }
  }

  return PLASMA_SUCCESS;
}

bool plEditorProcessCommunicationChannel::IsClientAlive() const
{
  if (m_pClientProcess == nullptr)
    return false;

  bool bRunning = m_pClientProcess->state() != QProcess::NotRunning;
  bool bNoError = m_pClientProcess->error() == QProcess::UnknownError;

  return bRunning && bNoError;
}

void plEditorProcessCommunicationChannel::CloseConnection()
{
  if (m_pProtocol)
  {
    m_pProtocol->m_MessageEvent.RemoveEventHandler(plMakeDelegate(&plProcessCommunicationChannel::MessageFunc, this));
    m_pProtocol.Clear();
  }
  m_pChannel.Clear();

  if (m_pClientProcess)
  {
    m_pClientProcess->close();
    delete m_pClientProcess;
    m_pClientProcess = nullptr;
  }
}

plString plEditorProcessCommunicationChannel::GetStdoutContents()
{
  if (m_pClientProcess)
  {
    QByteArray output = m_pClientProcess->readAllStandardOutput();
    return plString(plStringView((const char*)output.data(), output.size()));
  }
  return plString();
}

//////////////////////////////////////////////////////////////////////////

plResult plEditorProcessRemoteCommunicationChannel::ConnectToServer(const char* szAddress)
{
  PLASMA_LOG_BLOCK("plEditorProcessRemoteCommunicationChannel::ConnectToServer");

  PLASMA_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");

  m_pFirstAllowedMessageType = nullptr;

  m_pChannel = plIpcChannel::CreateNetworkChannel(szAddress, plIpcChannel::Mode::Client);
  m_pProtocol = PLASMA_DEFAULT_NEW(plIpcProcessMessageProtocol, m_pChannel.Borrow());
  m_pProtocol->m_MessageEvent.AddEventHandler(plMakeDelegate(&plProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();

  return PLASMA_SUCCESS;
}

bool plEditorProcessRemoteCommunicationChannel::IsConnected() const
{
  return m_pChannel->IsConnected();
}

void plEditorProcessRemoteCommunicationChannel::CloseConnection()
{
  if (m_pProtocol)
  {
    m_pProtocol->m_MessageEvent.RemoveEventHandler(plMakeDelegate(&plProcessCommunicationChannel::MessageFunc, this));
    m_pProtocol.Clear();
  }
  m_pChannel.Clear();
}

void plEditorProcessRemoteCommunicationChannel::TryConnect()
{
  if (m_pChannel && !m_pChannel->IsConnected())
  {
    m_pChannel->Connect();
  }
}
