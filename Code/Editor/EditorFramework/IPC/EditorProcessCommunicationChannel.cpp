#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>

plResult PlasmaEditorProcessCommunicationChannel::StartClientProcess(
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

  m_pChannel->m_MessageEvent.AddEventHandler(plMakeDelegate(&plProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();

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

      PLASMA_DEFAULT_DELETE(m_pChannel);

      plLog::Error("Failed to start process '{0}'", sPath);
      return PLASMA_FAILURE;
    }
  }

  return PLASMA_SUCCESS;
}

bool PlasmaEditorProcessCommunicationChannel::IsClientAlive() const
{
  if (m_pClientProcess == nullptr)
    return false;

  bool bRunning = m_pClientProcess->state() != QProcess::NotRunning;
  bool bNoError = m_pClientProcess->error() == QProcess::UnknownError;

  return bRunning && bNoError;
}

void PlasmaEditorProcessCommunicationChannel::CloseConnection()
{
  if (m_pChannel)
  {
    m_pChannel->m_MessageEvent.RemoveEventHandler(plMakeDelegate(&plProcessCommunicationChannel::MessageFunc, this));
    PLASMA_DEFAULT_DELETE(m_pChannel);
  }
  if (m_pClientProcess)
  {
    m_pClientProcess->close();
    delete m_pClientProcess;
    m_pClientProcess = nullptr;
  }
}

plString PlasmaEditorProcessCommunicationChannel::GetStdoutContents()
{
  if (m_pClientProcess)
  {
    QByteArray output = m_pClientProcess->readAllStandardOutput();
    return plString(plStringView((const char*)output.data(), output.size()));
  }
  return plString();
}

//////////////////////////////////////////////////////////////////////////

plResult PlasmaEditorProcessRemoteCommunicationChannel::ConnectToServer(const char* szAddress)
{
  PLASMA_LOG_BLOCK("PlasmaEditorProcessRemoteCommunicationChannel::ConnectToServer");

  PLASMA_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");

  m_pFirstAllowedMessageType = nullptr;

  m_pChannel = plIpcChannel::CreateNetworkChannel(szAddress, plIpcChannel::Mode::Client);

  m_pChannel->m_MessageEvent.AddEventHandler(plMakeDelegate(&plProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();

  return PLASMA_SUCCESS;
}

bool PlasmaEditorProcessRemoteCommunicationChannel::IsConnected() const
{
  return m_pChannel->IsConnected();
}

void PlasmaEditorProcessRemoteCommunicationChannel::CloseConnection()
{
  if (m_pChannel)
  {
    m_pChannel->m_MessageEvent.RemoveEventHandler(plMakeDelegate(&plProcessCommunicationChannel::MessageFunc, this));
    PLASMA_DEFAULT_DELETE(m_pChannel);
  }
}

void PlasmaEditorProcessRemoteCommunicationChannel::TryConnect()
{
  if (m_pChannel && !m_pChannel->IsConnected())
  {
    m_pChannel->Connect();
  }
}
