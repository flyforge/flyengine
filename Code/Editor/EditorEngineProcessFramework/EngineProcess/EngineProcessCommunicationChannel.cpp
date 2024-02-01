#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#elif PL_ENABLED(PL_PLATFORM_LINUX)
#  include <signal.h>
#endif

bool plEngineProcessCommunicationChannel::IsHostAlive() const
{
  if (plEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return true;

  if (m_iHostPID == 0)
    return false;

  bool bValid = true;

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
  DWORD pid = static_cast<DWORD>(m_iHostPID);
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  bValid = (hProcess != INVALID_HANDLE_VALUE) && (hProcess != nullptr);

  DWORD exitcode = 0;
  if (GetExitCodeProcess(hProcess, &exitcode) && exitcode != STILL_ACTIVE)
    bValid = false;

  CloseHandle(hProcess);
#elif PL_ENABLED(PL_PLATFORM_LINUX)
  // We send the signal 0 to the given PID (signal 0 is a no-op)
  // If this succeeds, the process with the given PID exists
  // if it fails, the process does not / no longer exist.
  if (kill(m_iHostPID, 0) < 0)
    bValid = false;
#else
#  error Not implemented
#endif

  return bValid;
}

plResult plEngineProcessCommunicationChannel::ConnectToHostProcess()
{
  PL_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");

  if (!plEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
  {
    if (plCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC").IsEmpty())
    {
      PL_REPORT_FAILURE("Command Line does not contain -IPC parameter");
      return PL_FAILURE;
    }

    if (plCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID").IsEmpty())
    {
      PL_REPORT_FAILURE("Command Line does not contain -PID parameter");
      return PL_FAILURE;
    }

    m_iHostPID = 0;
    PL_SUCCEED_OR_RETURN(plConversionUtils::StringToInt64(plCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID"), m_iHostPID));

    plLog::Debug("Host Process ID: {0}", m_iHostPID);

    m_pChannel = plIpcChannel::CreatePipeChannel(plCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC"), plIpcChannel::Mode::Client);
  }
  else
  {
    m_pChannel = plIpcChannel::CreateNetworkChannel("localhost:1050", plIpcChannel::Mode::Server);
  }
  m_pProtocol = PL_DEFAULT_NEW(plIpcProcessMessageProtocol, m_pChannel.Borrow());
  m_pProtocol->m_MessageEvent.AddEventHandler(plMakeDelegate(&plProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();

  return PL_SUCCESS;
}
