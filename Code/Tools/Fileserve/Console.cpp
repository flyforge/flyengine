#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <Foundation/Logging/Log.h>

void plFileserverApp::FileserverEventHandlerConsole(const plFileserverEvent& e)
{
  switch (e.m_Type)
  {
    case plFileserverEvent::Type::None:
      plLog::Error("Invalid Fileserver event type");
      break;

    case plFileserverEvent::Type::ServerStarted:
    {
      plLog::Info("plFileserver is running");
    }
    break;

    case plFileserverEvent::Type::ServerStopped:
    {
      plLog::Info("plFileserver was shut down");
    }
    break;

    case plFileserverEvent::Type::ClientConnected:
    {
      plLog::Success("Client connected");
    }
    break;

    case plFileserverEvent::Type::MountDataDir:
    {
      plLog::Info("Mounted data directory '{0}' ({1})", e.m_szName, e.m_szPath);
    }
    break;

    case plFileserverEvent::Type::UnmountDataDir:
    {
      plLog::Info("Unmount request for data directory '{0}' ({1})", e.m_szName, e.m_szPath);
    }
    break;

    case plFileserverEvent::Type::FileDownloadRequest:
    {
      if (e.m_FileState == plFileserveFileState::NonExistant)
        plLog::Dev("Request: (N/A) '{0}'", e.m_szPath);

      if (e.m_FileState == plFileserveFileState::SameHash)
        plLog::Dev("Request: (HASH) '{0}'", e.m_szPath);

      if (e.m_FileState == plFileserveFileState::SameTimestamp)
        plLog::Dev("Request: (TIME) '{0}'", e.m_szPath);

      if (e.m_FileState == plFileserveFileState::NonExistantEither)
        plLog::Dev("Request: (N/AE) '{0}'", e.m_szPath);

      if (e.m_FileState == plFileserveFileState::Different)
        plLog::Info("Request: '{0}' ({1} bytes)", e.m_szPath, e.m_uiSizeTotal);
    }
    break;

    case plFileserverEvent::Type::FileDownloading:
    {
      plLog::Debug("Transfer: {0}/{1} bytes", e.m_uiSentTotal, e.m_uiSizeTotal, e.m_szPath);
    }
    break;

    case plFileserverEvent::Type::FileDownloadFinished:
    {
      if (e.m_FileState == plFileserveFileState::Different)
        plLog::Info("Transfer done.");
    }
    break;

    case plFileserverEvent::Type::FileDeleteRequest:
    {
      plLog::Warning("File Deletion: '{0}'", e.m_szPath);
    }
    break;

    case plFileserverEvent::Type::FileUploading:
      plLog::Debug("Upload: {0}/{1} bytes", e.m_uiSentTotal, e.m_uiSizeTotal, e.m_szPath);
      break;

    case plFileserverEvent::Type::FileUploadFinished:
      plLog::Info("Upload finished: {0}", e.m_szPath);
      break;

    default:
      break;
  }
}
