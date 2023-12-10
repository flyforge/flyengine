#pragma once

#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

class plRemoteMessage;

struct plFileserverEvent
{
  enum class Type
  {
    None,
    ServerStarted,
    ServerStopped,
    ClientConnected,
    ClientReconnected, // connected again after a disconnect
    ClientDisconnected,
    MountDataDir,
    MountDataDirFailed,
    UnmountDataDir,
    FileDownloadRequest,
    FileDownloading,
    FileDownloadFinished,
    FileDeleteRequest,
    FileUploadRequest,
    FileUploading,
    FileUploadFinished,
    AreYouThereRequest,
  };

  Type m_Type = Type::None;
  plUInt32 m_uiClientID = 0;
  const char* m_szName = nullptr;
  const char* m_szPath = nullptr;
  const char* m_szRedirectedPath = nullptr;
  plUInt32 m_uiSizeTotal = 0;
  plUInt32 m_uiSentTotal = 0;
  plFileserveFileState m_FileState = plFileserveFileState::None;
};

/// \brief A file server allows to serve files from a host PC to another process that is potentially on another device.
///
/// This is mostly useful for mobile devices, that do not have access to the data on the development machine.
/// Typically every change to a file would require packaging the app and deploying it to the device again.
/// Fileserve allows to only deploy a very lean application and instead get all asset data directly from a host PC.
/// This also allows to modify data on the PC and reload the data in the running application without delay.
///
/// A single file server can serve multiple clients. However, to mount "special directories" (see plFileSystem) the server
/// needs to know what local path to map them to (it uses the configuration on plFileSystem).
/// That means it cannot serve two clients that require different settings for the same special directory.
///
/// The port on which the server connects to clients can be configured through the command line option "-fs_port X"
class PLASMA_FILESERVEPLUGIN_DLL plFileserver
{
  PLASMA_DECLARE_SINGLETON(plFileserver);

public:
  plFileserver();

  /// \brief Starts listening for client connections. Uses the configured port.
  void StartServer();

  /// \brief Disconnects all clients.
  void StopServer();

  /// \brief Has to be executed regularly to serve clients and keep the connection alive.
  bool UpdateServer();

  /// \brief Whether the server was started.
  bool IsServerRunning() const;

  /// \brief Overrides the current port setting. May only be called when the server is currently not running.
  void SetPort(plUInt16 uiPort);

  /// \brief Returns the currently set port. If the command line option "-fs_port X" was used, this will return that value, otherwise the default is
  /// 1042.
  plUInt16 GetPort() const { return m_uiPort; }

  /// \brief The server broadcasts events about its activity
  plEvent<const plFileserverEvent&> m_Events;

  /// \brief Broadcasts to all clients that they should reload their resources
  void BroadcastReloadResourcesCommand();

  static plResult SendConnectionInfo(
    const char* szClientAddress, plUInt16 uiMyPort, const plArrayPtr<plStringBuilder>& MyIPs, plTime timeout = plTime::Seconds(10));

private:
  void NetworkEventHandler(const plRemoteEvent& e);
  plFileserveClientContext& DetermineClient(plRemoteMessage& msg);
  void NetworkMsgHandler(plRemoteMessage& msg);
  void HandleMountRequest(plFileserveClientContext& client, plRemoteMessage& msg);
  void HandleUnmountRequest(plFileserveClientContext& client, plRemoteMessage& msg);
  void HandleFileRequest(plFileserveClientContext& client, plRemoteMessage& msg);
  void HandleDeleteFileRequest(plFileserveClientContext& client, plRemoteMessage& msg);
  void HandleUploadFileHeader(plFileserveClientContext& client, plRemoteMessage& msg);
  void HandleUploadFileTransfer(plFileserveClientContext& client, plRemoteMessage& msg);
  void HandleUploadFileFinished(plFileserveClientContext& client, plRemoteMessage& msg);

  plHashTable<plUInt32, plFileserveClientContext> m_Clients;
  plUniquePtr<plRemoteInterface> m_pNetwork;
  plDynamicArray<plUInt8> m_SendToClient;   // ie. 'downloads' from server to client
  plDynamicArray<plUInt8> m_SentFromClient; // ie. 'uploads' from client to server
  plStringBuilder m_sCurFileUpload;
  plUuid m_FileUploadGuid;
  plUInt32 m_uiFileUploadSize;
  plUInt16 m_uiPort = 1042;
};
