#include <Foundation/FoundationPCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Communication/Implementation/MessageLoop.h>
#  include <Foundation/Communication/Implementation/Win/MessageLoop_win.h>
#  include <Foundation/Communication/Implementation/Win/PipeChannel_win.h>
#  include <Foundation/Communication/RemoteMessage.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Serialization/ReflectionSerializer.h>

plPipeChannel_win::State::State(plPipeChannel_win* pChannel)
  : IsPending(false)
{
  memset(&Context.Overlapped, 0, sizeof(Context.Overlapped));
  Context.pChannel = pChannel;
  IsPending = false;
}

plPipeChannel_win::State::~State() = default;

plPipeChannel_win::plPipeChannel_win(plStringView sAddress, Mode::Enum mode)
  : plIpcChannel(sAddress, mode)
  , m_InputState(this)
  , m_OutputState(this)
{
  m_pOwner->AddChannel(this);
}

plPipeChannel_win::~plPipeChannel_win()
{
  if (m_hPipeHandle != INVALID_HANDLE_VALUE)
  {
    Disconnect();
  }
  while (IsConnected())
  {
    plThreadUtils::Sleep(plTime::MakeFromMilliseconds(10));
  }
  m_pOwner->RemoveChannel(this);
}

bool plPipeChannel_win::CreatePipe(plStringView sAddress)
{
  plStringBuilder sPipename("\\\\.\\pipe\\", sAddress);

  if (m_Mode == Mode::Server)
  {
    SECURITY_ATTRIBUTES attributes = {0};
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = NULL;
    attributes.bInheritHandle = FALSE;

    m_hPipeHandle = CreateNamedPipeW(plStringWChar(sPipename).GetData(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, 1, BUFFER_SIZE, BUFFER_SIZE, 5000, &attributes);
  }
  else
  {
    m_hPipeHandle = CreateFileW(plStringWChar(sPipename).GetData(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
      SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION | FILE_FLAG_OVERLAPPED, NULL);
  }

  if (m_hPipeHandle == INVALID_HANDLE_VALUE)
  {
    plLog::Error("Could not create named pipe: {0}", plArgErrorCode(GetLastError()));
    return false;
  }

  if (m_hPipeHandle != INVALID_HANDLE_VALUE)
  {
    plMessageLoop_win* pMsgLoopWin = static_cast<plMessageLoop_win*>(m_pOwner);

    ULONG_PTR key = reinterpret_cast<ULONG_PTR>(this);
    HANDLE port = CreateIoCompletionPort(m_hPipeHandle, pMsgLoopWin->GetPort(), key, 1);
    PLASMA_ASSERT_DEBUG(pMsgLoopWin->GetPort() == port, "Failed to CreateIoCompletionPort: {0}", plArgErrorCode(GetLastError()));
  }
  return true;
}

void plPipeChannel_win::InternalConnect()
{
  if (GetConnectionState() != ConnectionState::Disconnected)
    return;

#  if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  if (m_ThreadId == 0)
    m_ThreadId = plThreadUtils::GetCurrentThreadID();
#  endif

  if (!CreatePipe(m_sAddress))
    return;

  if (m_hPipeHandle == INVALID_HANDLE_VALUE)
    return;

  SetConnectionState(ConnectionState::Connecting);
  if (m_Mode == Mode::Server)
  {
    ProcessConnection();
  }
  else
  {
    // If CreatePipe succeeded, we are already connected.
    SetConnectionState(ConnectionState::Connected);
  }

  if (!m_InputState.IsPending)
  {
    OnIOCompleted(&m_InputState.Context, 0, 0);
  }

  if (IsConnected())
  {
    ProcessOutgoingMessages(0);
  }

  return;
}

void plPipeChannel_win::InternalDisconnect()
{
  if (GetConnectionState() == ConnectionState::Disconnected)
    return;

#  if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  if (m_ThreadId != 0)
    PLASMA_ASSERT_DEBUG(m_ThreadId == plThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");
#  endif
  if (m_InputState.IsPending || m_OutputState.IsPending)
  {
    CancelIo(m_hPipeHandle);
  }

  if (m_hPipeHandle != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hPipeHandle);
    m_hPipeHandle = INVALID_HANDLE_VALUE;
  }

  while (m_InputState.IsPending || m_OutputState.IsPending)
  {
    FlushPendingOperations();
  }

  bool bWasConnected = false;
  {
    PLASMA_LOCK(m_OutputQueueMutex);
    m_OutputQueue.Clear();
    bWasConnected = IsConnected();
  }

  if (bWasConnected)
  {
    SetConnectionState(ConnectionState::Disconnected);
    // Raise in case another thread is waiting for new messages (as we would sleep forever otherwise).
    m_IncomingMessages.RaiseSignal();
  }
}

void plPipeChannel_win::InternalSend()
{
  if (!m_OutputState.IsPending && IsConnected())
  {
    ProcessOutgoingMessages(0);
  }
}


bool plPipeChannel_win::NeedWakeup() const
{
  return m_OutputState.IsPending == 0;
}

bool plPipeChannel_win::ProcessConnection()
{
  PLASMA_ASSERT_DEBUG(m_ThreadId == plThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");
  if (m_InputState.IsPending)
    m_InputState.IsPending = false;

  BOOL res = ConnectNamedPipe(m_hPipeHandle, &m_InputState.Context.Overlapped);
  if (res)
  {
    // PLASMA_REPORT_FAILURE
    return false;
  }

  plUInt32 error = GetLastError();
  switch (error)
  {
    case ERROR_IO_PENDING:
      m_InputState.IsPending = true;
      break;
    case ERROR_PIPE_CONNECTED:
      SetConnectionState(ConnectionState::Connected);
      break;
    case ERROR_NO_DATA:
      return false;
    default:
      plLog::Error("Could not connect to pipe (Error code: {0})", plArgErrorCode(error));
      return false;
  }

  return true;
}

bool plPipeChannel_win::ProcessIncomingMessages(DWORD uiBytesRead)
{
  PLASMA_ASSERT_DEBUG(m_ThreadId == plThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");
  if (m_InputState.IsPending)
  {
    m_InputState.IsPending = false;
    if (uiBytesRead == 0)
      return false;
  }

  while (true)
  {
    if (uiBytesRead == 0)
    {
      if (m_hPipeHandle == INVALID_HANDLE_VALUE)
        return false;

      BOOL res = ReadFile(m_hPipeHandle, m_InputBuffer, BUFFER_SIZE, &uiBytesRead, &m_InputState.Context.Overlapped);

      if (!res)
      {
        plUInt32 error = GetLastError();
        if (error == ERROR_IO_PENDING)
        {
          m_InputState.IsPending = true;
          return true;
        }
        if (m_Mode == Mode::Server)
        {
          // only log when in server mode, otherwise this can result in an endless recursion
          plLog::Error("Read from pipe failed: {0}", plArgErrorCode(error));
        }
        return false;
      }
      m_InputState.IsPending = true;
      return true;
    }

    PLASMA_ASSERT_DEBUG(uiBytesRead != 0, "We really should have data at this point.");
    ReceiveData(plArrayPtr<plUInt8>(m_InputBuffer, uiBytesRead));
    uiBytesRead = 0;
  }
  return true;
}

bool plPipeChannel_win::ProcessOutgoingMessages(DWORD uiBytesWritten)
{
  PLASMA_ASSERT_DEBUG(IsConnected(), "Must be connected to process outgoing messages.");
  PLASMA_ASSERT_DEBUG(m_ThreadId == plThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");

  if (m_OutputState.IsPending)
  {
    if (uiBytesWritten == 0)
    {
      // Don't reset isPending right away as we want to use it to decide
      // whether we need to wake up the worker thread again.
      plLog::Error("pipe error: {0}", plArgErrorCode(GetLastError()));
      m_OutputState.IsPending = false;
      return false;
    }

    PLASMA_LOCK(m_OutputQueueMutex);
    // message was send
    m_OutputQueue.PopFront();
  }

  if (m_hPipeHandle == INVALID_HANDLE_VALUE)
  {
    m_OutputState.IsPending = false;
    return false;
  }
  const plMemoryStreamStorageInterface* storage = nullptr;
  {
    PLASMA_LOCK(m_OutputQueueMutex);
    if (m_OutputQueue.IsEmpty())
    {
      m_OutputState.IsPending = false;
      return true;
    }
    storage = &m_OutputQueue.PeekFront();
  }

  plUInt64 uiToWrite = storage->GetStorageSize64();
  plUInt64 uiNextOffset = 0;
  while (uiToWrite > 0)
  {
    const plArrayPtr<const plUInt8> range = storage->GetContiguousMemoryRange(uiNextOffset);
    uiToWrite -= range.GetCount();

    BOOL res = WriteFile(m_hPipeHandle, range.GetPtr(), range.GetCount(), &uiBytesWritten, &m_OutputState.Context.Overlapped);

    if (!res)
    {
      plUInt32 error = GetLastError();
      if (error == ERROR_IO_PENDING)
      {
        m_OutputState.IsPending = true;
        return true;
      }
      plLog::Error("Write to pipe failed: {0}", plArgErrorCode(error));
      return false;
    }

    uiNextOffset += range.GetCount();
  }


  m_OutputState.IsPending = true;
  return true;
}

void plPipeChannel_win::OnIOCompleted(IOContext* pContext, DWORD uiBytesTransfered, DWORD uiError)
{
  PLASMA_ASSERT_DEBUG(m_ThreadId == plThreadUtils::GetCurrentThreadID(), "Function must be called from worker thread!");
  bool bRes = true;
  if (pContext == &m_InputState.Context)
  {
    if (!IsConnected())
    {
      if (!ProcessConnection())
        return;

      bool bHasOutput = false;
      {
        PLASMA_LOCK(m_OutputQueueMutex);
        bHasOutput = !m_OutputQueue.IsEmpty();
      }

      if (bHasOutput && m_OutputState.IsPending == 0)
        ProcessOutgoingMessages(0);
      if (m_InputState.IsPending)
        return;
    }
    bRes = ProcessIncomingMessages(uiBytesTransfered);
  }
  else
  {
    PLASMA_ASSERT_DEBUG(pContext == &m_OutputState.Context, "");
    bRes = ProcessOutgoingMessages(uiBytesTransfered);
  }
  if (!bRes && m_hPipeHandle != INVALID_HANDLE_VALUE)
  {
    InternalDisconnect();
  }
}
#endif

PLASMA_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Win_PipeChannel_win);
