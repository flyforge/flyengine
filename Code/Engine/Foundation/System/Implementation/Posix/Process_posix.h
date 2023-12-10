#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <Foundation/System/SystemInformation.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

PLASMA_DEFINE_AS_POD_TYPE(struct pollfd);

class plFd
{
public:
  plFd() = default;
  plFd(const plFd&) = delete;
  plFd(plFd&& other)
  {
    m_fd = other.m_fd;
    other.m_fd = -1;
  }

  ~plFd()
  {
    Close();
  }

  void Close()
  {
    if (m_fd != -1)
    {
      close(m_fd);
      m_fd = -1;
    }
  }

  bool IsValid() const
  {
    return m_fd >= 0;
  }

  void operator=(const plFd&) = delete;
  void operator=(plFd&& other)
  {
    Close();
    m_fd = other.m_fd;
    other.m_fd = -1;
  }

  void TakeOwnership(int fd)
  {
    Close();
    m_fd = fd;
  }

  int Borrow() const { return m_fd; }

  int Detach()
  {
    auto result = m_fd;
    m_fd = -1;
    return result;
  }

  plResult AddFlags(int addFlags)
  {
    if(m_fd < 0)
      return PLASMA_FAILURE;

    if(addFlags & O_CLOEXEC)
    {
      int flags = fcntl(m_fd, F_GETFD);
      flags |= addFlags;
      if (fcntl(m_fd, F_SETFD, FD_CLOEXEC) != 0)
      {
        plLog::Error("Failed to set flags on {}: {}", m_fd, errno);
        return PLASMA_FAILURE;
      }
      addFlags &= ~O_CLOEXEC;
    }

    if(addFlags)
    {
      int flags = fcntl(m_fd, F_GETFL);
      flags |= addFlags;
      if (fcntl(m_fd, F_SETFL, addFlags) != 0)
      {
        plLog::Error("Failed to set flags on {}: {}", m_fd, errno);
        return PLASMA_FAILURE;
      }
    }

    return PLASMA_SUCCESS;
  }

  static plResult MakePipe(plFd (&fds)[2], int flags = 0)
  {
    fds[0].Close();
    fds[1].Close();
#if PLASMA_ENABLED(PLASMA_USE_LINUX_POSIX_EXTENSIONS)
    if (pipe2((int*)fds, flags) != 0)
    {
      return PLASMA_FAILURE;
    }
#else
    if (pipe((int*)fds) != 0)
    {
      return PLASMA_FAILURE;
    }
    if(flags != 0 && (fds[0].AddFlags(flags).Failed() || fds[1].AddFlags(flags).Failed()))
    {
      fds[0].Close();
      fds[1].Close();
      return PLASMA_FAILURE;
    }
#endif
    return PLASMA_SUCCESS;
  }

private:
  int m_fd = -1;
};

namespace
{
  struct ProcessStartupError
  {
    enum class Type : plUInt32
    {
      FailedToChangeWorkingDirectory = 0,
      FailedToExecv = 1
    };

    Type type;
    int errorCode;
  };
} // namespace


struct plProcessImpl
{
  ~plProcessImpl()
  {
    StopStreamWatcher();
  }

  pid_t m_childPid = -1;
  bool m_exitCodeAvailable = false;
  bool m_processSuspended = false;

  struct StdStreamInfo
  {
    plFd fd;
    plDelegate<void(plStringView)> callback;
  };
  plHybridArray<StdStreamInfo, 2> m_streams;
  plDynamicArray<plStringBuilder> m_overflowBuffers;
  plUniquePtr<plOSThread> m_streamWatcherThread;
  plFd m_wakeupPipeReadEnd;
  plFd m_wakeupPipeWriteEnd;

  static void* StreamWatcherThread(void* context)
  {
    plProcessImpl* self = reinterpret_cast<plProcessImpl*>(context);
    char buffer[4096];

    plHybridArray<struct pollfd, 3> pollfds;

    pollfds.PushBack({self->m_wakeupPipeReadEnd.Borrow(), POLLIN, 0});
    for (StdStreamInfo& stream : self->m_streams)
    {
      pollfds.PushBack({stream.fd.Borrow(), POLLIN, 0});
    }

    bool run = true;
    while (run)
    {
      int result = poll(pollfds.GetData(), pollfds.GetCount(), -1);
      if (result > 0)
      {
        // Result at index 0 is special and means there was a WakeUp
        if (pollfds[0].revents != 0)
        {
          run = false;
        }

        for (plUInt32 i = 1; i < pollfds.GetCount(); ++i)
        {
          if (pollfds[i].revents & POLLIN)
          {
            plStringBuilder& overflowBuffer = self->m_overflowBuffers[i - 1];
            StdStreamInfo& stream = self->m_streams[i - 1];
            while (true)
            {
              ssize_t numBytes = read(stream.fd.Borrow(), buffer, PLASMA_ARRAY_SIZE(buffer));
              if (numBytes < 0)
              {
                if (errno == EWOULDBLOCK)
                {
                  break;
                }
                plLog::Error("Process Posix read error on {}: {}", stream.fd.Borrow(), errno);
                return nullptr;
              }

              const char* szCurrentPos = buffer;
              const char* szEndPos = buffer + numBytes;
              while (szCurrentPos < szEndPos)
              {
                const char* szFound = plStringUtils::FindSubString(szCurrentPos, "\n", szEndPos);
                if (szFound)
                {
                  if (overflowBuffer.IsEmpty())
                  {
                    // If there is nothing in the overflow buffer this is a complete line and can be fired as is.
                    stream.callback(plStringView(szCurrentPos, szFound + 1));
                  }
                  else
                  {
                    // We have data in the overflow buffer so this is the final part of a partial line so we need to complete and fire the overflow buffer.
                    overflowBuffer.Append(plStringView(szCurrentPos, szFound + 1));
                    stream.callback(overflowBuffer);
                    overflowBuffer.Clear();
                  }
                  szCurrentPos = szFound + 1;
                }
                else
                {
                  // This is either the start or a middle segment of a line, append to overflow buffer.
                  overflowBuffer.Append(plStringView(szCurrentPos, szEndPos));
                  szCurrentPos = szEndPos;
                }
              }

              if (numBytes < PLASMA_ARRAY_SIZE(buffer))
              {
                break;
              }
            }
          }
          pollfds[i].revents = 0;
        }
      }
      else if (result < 0)
      {
        plLog::Error("poll error {}", errno);
        break;
      }
    }

    for (plUInt32 i = 0; i < self->m_streams.GetCount(); ++i)
    {
      plStringBuilder& overflowBuffer = self->m_overflowBuffers[i];
      if (!overflowBuffer.IsEmpty())
      {
        self->m_streams[i].callback(overflowBuffer);
        overflowBuffer.Clear();
      }

      self->m_streams[i].fd.Close();
    }

    return nullptr;
  }

  plResult StartStreamWatcher()
  {
    plFd wakeupPipe[2];
    if (plFd::MakePipe(wakeupPipe, O_NONBLOCK | O_CLOEXEC).Failed())
    {
      plLog::Error("Failed to setup wakeup pipe {}", errno);
      return PLASMA_FAILURE;
    }
    else
    {
      m_wakeupPipeReadEnd = std::move(wakeupPipe[0]);
      m_wakeupPipeWriteEnd = std::move(wakeupPipe[1]);
    }

    m_streamWatcherThread = PLASMA_DEFAULT_NEW(plOSThread, &StreamWatcherThread, this, "StdStrmWtch");
    m_streamWatcherThread->Start();

    return PLASMA_SUCCESS;
  }

  void StopStreamWatcher()
  {
    if (m_streamWatcherThread)
    {
      char c = 0;
      PLASMA_IGNORE_UNUSED(write(m_wakeupPipeWriteEnd.Borrow(), &c, 1));
      m_streamWatcherThread->Join();
      m_streamWatcherThread = nullptr;
    }
    m_wakeupPipeReadEnd.Close();
    m_wakeupPipeWriteEnd.Close();
  }

  void AddStream(plFd fd, const plDelegate<void(plStringView)>& callback)
  {
    m_streams.PushBack({std::move(fd), callback});
    m_overflowBuffers.SetCount(m_streams.GetCount());
  }

  plUInt32 GetNumStreams() const { return m_streams.GetCount(); }

  static plResult StartChildProcess(const plProcessOptions& opt, pid_t& outPid, bool suspended, plFd& outStdOutFd, plFd& outStdErrFd)
  {
    plFd stdoutPipe[2];
    plFd stderrPipe[2];
    plFd startupErrorPipe[2];

    plStringBuilder executablePath = opt.m_sProcess;
    plFileStats stats;
    if (!opt.m_sProcess.IsAbsolutePath())
    {
      executablePath = plOSFile::GetCurrentWorkingDirectory();
      executablePath.AppendPath(opt.m_sProcess);
    }

    if (plOSFile::GetFileStats(executablePath, stats).Failed() || stats.m_bIsDirectory)
    {
	  plHybridArray<char, 512> confPath;
      auto envPATH = getenv("PATH");
      if (envPATH == nullptr) // if no PATH environment variable is available, we need to fetch the system default;
      {
#if _POSIX_C_SOURCE >= 2 || _XOPEN_SOURCE
        size_t confPathSize = confstr(_CS_PATH, nullptr, 0);
        if (confPathSize > 0)
        {
          confPath.SetCountUninitialized(confPathSize);
          if (confstr(_CS_PATH, confPath.GetData(), confPath.GetCount()) == 0)
          {
            confPath.SetCountUninitialized(0);
          }
        }
#endif
        if (confPath.GetCount() == 0)
        {
          confPath.PushBack('\0');
        }
        envPATH = confPath.GetData();
      }

      plStringView path = envPATH;
      plHybridArray<plStringView, 16> pathParts;
      path.Split(false, pathParts, ":");

      for (auto& pathPart : pathParts)
      {
        executablePath = pathPart;
        executablePath.AppendPath(opt.m_sProcess);
        if (plOSFile::GetFileStats(executablePath, stats).Succeeded() && !stats.m_bIsDirectory)
        {
          break;
        }
        executablePath.Clear();
      }
    }

    if (executablePath.IsEmpty())
    {
      return PLASMA_FAILURE;
    }

    if (opt.m_onStdOut.IsValid())
    {
      if (plFd::MakePipe(stdoutPipe).Failed())
      {
        return PLASMA_FAILURE;
      }
      if(stdoutPipe[0].AddFlags(O_NONBLOCK).Failed())
      {
        return PLASMA_FAILURE;
      }
    }

    if (opt.m_onStdError.IsValid())
    {
      if (plFd::MakePipe(stderrPipe).Failed())
      {
        return PLASMA_FAILURE;
      }
      if(stderrPipe[0].AddFlags(O_NONBLOCK).Failed())
      {
        return PLASMA_FAILURE;
      }
    }

    if (plFd::MakePipe(startupErrorPipe, O_CLOEXEC).Failed())
    {
      return PLASMA_FAILURE;
    }

    pid_t childPid = fork();
    if (childPid < 0)
    {
      return PLASMA_FAILURE;
    }

    if (childPid == 0) // We are the child
    {
      if (suspended)
      {
        if (raise(SIGSTOP) < 0)
        {
          _exit(-1);
        }
      }

      if (opt.m_bHideConsoleWindow == true)
      {
        // Redirect STDIN to /dev/null
        int stdinReplace = open("/dev/null", O_RDONLY);
        dup2(stdinReplace, STDIN_FILENO);
        close(stdinReplace);

        if (!opt.m_onStdOut.IsValid())
        {
          int stdoutReplace = open("/dev/null", O_WRONLY);
          dup2(stdoutReplace, STDOUT_FILENO);
          close(stdoutReplace);
        }

        if (!opt.m_onStdError.IsValid())
        {
          int stderrReplace = open("/dev/null", O_WRONLY);
          dup2(stderrReplace, STDERR_FILENO);
          close(stderrReplace);
        }
      }
      else
      {
        // TODO: Launch a x-terminal-emulator with the command and somehow redirect STDOUT, etc?
        PLASMA_ASSERT_NOT_IMPLEMENTED;
      }

      if (opt.m_onStdOut.IsValid())
      {
        stdoutPipe[0].Close();                       // We don't need the read end of the pipe in the child process
        dup2(stdoutPipe[1].Borrow(), STDOUT_FILENO); // redirect the write end to STDOUT
        stdoutPipe[1].Close();
      }

      if (opt.m_onStdError.IsValid())
      {
        stderrPipe[0].Close();                       // We don't need the read end of the pipe in the child process
        dup2(stderrPipe[1].Borrow(), STDERR_FILENO); // redirect the write end to STDERR
        stderrPipe[1].Close();
      }

      startupErrorPipe[0].Close(); // we don't need the read end of the startup error pipe in the child process

      plHybridArray<char*, 9> args;

      args.PushBack(const_cast<char*>(executablePath.GetData()));
      for (const plString& arg : opt.m_Arguments)
      {
        args.PushBack(const_cast<char*>(arg.GetData()));
      }
      args.PushBack(nullptr);

      if (!opt.m_sWorkingDirectory.IsEmpty())
      {
        if (chdir(opt.m_sWorkingDirectory.GetData()) < 0)
        {
          auto err = ProcessStartupError{ProcessStartupError::Type::FailedToChangeWorkingDirectory, 0};
          PLASMA_IGNORE_UNUSED(write(startupErrorPipe[1].Borrow(), &err, sizeof(err)));
          startupErrorPipe[1].Close();
          _exit(-1);
        }
      }

      if (execv(executablePath, args.GetData()) < 0)
      {
        auto err = ProcessStartupError{ProcessStartupError::Type::FailedToExecv, errno};
        PLASMA_IGNORE_UNUSED(write(startupErrorPipe[1].Borrow(), &err, sizeof(err)));
        startupErrorPipe[1].Close();
        _exit(-1);
      }
    }
    else
    {
      startupErrorPipe[1].Close(); // We don't need the write end of the startup error pipe in the parent process
      stdoutPipe[1].Close();       // Don't need the write end in the parent process
      stderrPipe[1].Close();       // Don't need the write end in the parent process

      ProcessStartupError err = {};
      auto errSize = read(startupErrorPipe[0].Borrow(), &err, sizeof(err));
      startupErrorPipe[0].Close(); // we no longer need the read end of the startup error pipe

      // There are two possible cases here
      // Case 1: errSize is equal to 0, which means no error happened on the startupErrorPipe was closed during the execv call
      // Case 2: errSize > 0 in which case there was an error before the pipe was closed normally.
      if (errSize > 0)
      {
        PLASMA_ASSERT_DEV(errSize == sizeof(err), "Child process should have written a full ProcessStartupError struct");
        switch (err.type)
        {
          case ProcessStartupError::Type::FailedToChangeWorkingDirectory:
            plLog::Error("Failed to start process '{}' because the given working directory '{}' is invalid", opt.m_sProcess, opt.m_sWorkingDirectory);
            break;
          case ProcessStartupError::Type::FailedToExecv:
            plLog::Error("Failed to exec when starting process '{}' the error code is '{}'", opt.m_sProcess, err.errorCode);
            break;
        }
        return PLASMA_FAILURE;
      }

      outPid = childPid;

      if (opt.m_onStdOut.IsValid())
      {
        outStdOutFd = std::move(stdoutPipe[0]);
      }

      if (opt.m_onStdError.IsValid())
      {
        outStdErrFd = std::move(stderrPipe[0]);
      }
    }

    return PLASMA_SUCCESS;
  }
};

plProcess::plProcess()
{
  m_pImpl = PLASMA_DEFAULT_NEW(plProcessImpl);
}

plProcess::~plProcess()
{
  if (GetState() == plProcessState::Running)
  {
    plLog::Dev("Process still running - terminating '{}'", m_sProcess);

    Terminate().IgnoreResult();
  }

  // Explicitly clear the implementation here so that member
  // state (e.g. delegates) used by the impl survives the implementation.
  m_pImpl.Clear();
}

plResult plProcess::Execute(const plProcessOptions& opt, plInt32* out_iExitCode /*= nullptr*/)
{
  pid_t childPid = 0;
  plFd stdoutFd;
  plFd stderrFd;
  if (plProcessImpl::StartChildProcess(opt, childPid, false, stdoutFd, stderrFd).Failed())
  {
    return PLASMA_FAILURE;
  }

  plProcessImpl impl;
  if (stdoutFd.IsValid())
  {
    impl.AddStream(std::move(stdoutFd), opt.m_onStdOut);
  }

  if (stderrFd.IsValid())
  {
    impl.AddStream(std::move(stderrFd), opt.m_onStdError);
  }

  if (impl.GetNumStreams() > 0 && impl.StartStreamWatcher().Failed())
  {
    return PLASMA_FAILURE;
  }

  int childStatus = -1;
  pid_t waitedPid = waitpid(childPid, &childStatus, 0);
  if (waitedPid < 0)
  {
    return PLASMA_FAILURE;
  }
  if (out_iExitCode != nullptr)
  {
    if (WIFEXITED(childStatus))
    {
      *out_iExitCode = WEXITSTATUS(childStatus);
    }
    else
    {
      *out_iExitCode = -1;
    }
  }
  return PLASMA_SUCCESS;
}

plResult plProcess::Launch(const plProcessOptions& opt, plBitflags<plProcessLaunchFlags> launchFlags /*= plProcessLaunchFlags::None*/)
{
  PLASMA_ASSERT_DEV(m_pImpl->m_childPid == -1, "Can not reuse an instance of plProcess");

  plFd stdoutFd;
  plFd stderrFd;

  if (plProcessImpl::StartChildProcess(opt, m_pImpl->m_childPid, launchFlags.IsSet(plProcessLaunchFlags::Suspended), stdoutFd, stderrFd).Failed())
  {
    return PLASMA_FAILURE;
  }

  m_pImpl->m_exitCodeAvailable = false;
  m_pImpl->m_processSuspended = launchFlags.IsSet(plProcessLaunchFlags::Suspended);

  if (stdoutFd.IsValid())
  {
    m_pImpl->AddStream(std::move(stdoutFd), opt.m_onStdOut);
  }

  if (stderrFd.IsValid())
  {
    m_pImpl->AddStream(std::move(stderrFd), opt.m_onStdError);
  }

  if (m_pImpl->GetNumStreams() > 0)
  {
    if (m_pImpl->StartStreamWatcher().Failed())
    {
      return PLASMA_FAILURE;
    }
  }

  if (launchFlags.IsSet(plProcessLaunchFlags::Detached))
  {
    Detach();
  }

  return PLASMA_SUCCESS;
}

plResult plProcess::ResumeSuspended()
{
  if (m_pImpl->m_childPid < 0 || !m_pImpl->m_processSuspended)
  {
    return PLASMA_FAILURE;
  }

  if (kill(m_pImpl->m_childPid, SIGCONT) < 0)
  {
    return PLASMA_FAILURE;
  }
  m_pImpl->m_processSuspended = false;
  return PLASMA_SUCCESS;
}

plResult plProcess::WaitToFinish(plTime timeout /*= plTime::MakeZero()*/)
{
  int childStatus = 0;
  PLASMA_SCOPE_EXIT(m_pImpl->StopStreamWatcher());

  if (timeout.IsZero())
  {
    if (waitpid(m_pImpl->m_childPid, &childStatus, 0) < 0)
    {
      return PLASMA_FAILURE;
    }
  }
  else
  {
    int waitResult = 0;
    plTime startWait = plTime::Now();
    while (true)
    {
      waitResult = waitpid(m_pImpl->m_childPid, &childStatus, WNOHANG);
      if (waitResult < 0)
      {
        return PLASMA_FAILURE;
      }
      if (waitResult > 0)
      {
        break;
      }
      plTime timeSpent = plTime::Now() - startWait;
      if (timeSpent > timeout)
      {
        return PLASMA_FAILURE;
      }
      plThreadUtils::Sleep(plMath::Min(plTime::MakeFromMilliseconds(100.0), timeout - timeSpent));
    }
  }

  if (WIFEXITED(childStatus))
  {
    m_iExitCode = WEXITSTATUS(childStatus);
  }
  else
  {
    m_iExitCode = -1;
  }
  m_pImpl->m_exitCodeAvailable = true;

  return PLASMA_SUCCESS;
}

plResult plProcess::Terminate()
{
  if (m_pImpl->m_childPid == -1)
  {
    return PLASMA_FAILURE;
  }

  PLASMA_SCOPE_EXIT(m_pImpl->StopStreamWatcher());

  if (kill(m_pImpl->m_childPid, SIGKILL) < 0)
  {
    if (errno != ESRCH) // ESRCH = Process does not exist
    {
      return PLASMA_FAILURE;
    }
  }
  m_pImpl->m_exitCodeAvailable = true;
  m_iExitCode = -1;

  return PLASMA_SUCCESS;
}

plProcessState plProcess::GetState() const
{
  if (m_pImpl->m_childPid == -1)
  {
    return plProcessState::NotStarted;
  }

  if (m_pImpl->m_exitCodeAvailable)
  {
    return plProcessState::Finished;
  }

  int childStatus = -1;
  int waitResult = waitpid(m_pImpl->m_childPid, &childStatus, WNOHANG);
  if (waitResult > 0)
  {
    m_iExitCode = WEXITSTATUS(childStatus);
    m_pImpl->m_exitCodeAvailable = true;

    m_pImpl->StopStreamWatcher();

    return plProcessState::Finished;
  }

  return plProcessState::Running;
}

void plProcess::Detach()
{
  m_pImpl->m_childPid = -1;
}

plOsProcessHandle plProcess::GetProcessHandle() const
{
  PLASMA_ASSERT_DEV(false, "There is no process handle on posix");
  return nullptr;
}

plOsProcessID plProcess::GetProcessID() const
{
  PLASMA_ASSERT_DEV(m_pImpl->m_childPid != -1, "No ProcessID available");
  return m_pImpl->m_childPid;
}

plOsProcessID plProcess::GetCurrentProcessID()
{
  return getpid();
}
