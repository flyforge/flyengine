#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Reflection/Reflection.h>

struct plXRRemotingConnectionState
{
  using StorageType = plUInt8;
  enum Enum : plUInt8
  {
    Disconnected,
    Connecting,
    Connected,
    Default = Disconnected
  };
};
PL_DECLARE_REFLECTABLE_TYPE(PL_GAMEENGINE_DLL, plXRRemotingConnectionState);

struct plXRRemotingDisconnectReason
{
  using StorageType = plUInt8;
  enum Enum : plUInt8
  {
    None = 0,
    Unknown = 1,
    NoServerCertificate = 2,
    HandshakePortBusy = 3,
    HandshakeUnreachable = 4,
    HandshakeConnectionFailed = 5,
    AuthenticationFailed = 6,
    RemotingVersionMismatch = 7,
    IncompatibleTransportProtocols = 8,
    HandshakeFailed = 9,
    TransportPortBusy = 10,
    TransportUnreachable = 11,
    TransportConnectionFailed = 12,
    ProtocolVersionMismatch = 13,
    ProtocolError = 14,
    VideoCodecNotAvailable = 15,
    Canceled = 16,
    ConnectionLost = 17,
    DeviceLost = 18,
    DisconnectRequest = 19,
    HandshakeNetworkUnreachable = 20,
    HandshakeConnectionRefused = 21,
    VideoFormatNotAvailable = 22,
    PeerDisconnectRequest = 23,
    PeerDisconnectTimeout = 24,
    SessionOpenTimeout = 25,
    RemotingHandshakeTimeout = 26,
    InternalError = 27,
    Default = None
  };
};
PL_DECLARE_REFLECTABLE_TYPE(PL_GAMEENGINE_DLL, plXRRemotingDisconnectReason);

struct plXRRemotingConnectionEventData
{
  plEnum<plXRRemotingConnectionState> m_connectionState;
  plEnum<plXRRemotingDisconnectReason> m_disconnectReason;
};

using plXRRemotingConnectionEvent = plEvent<const plXRRemotingConnectionEventData&>;

/// \brief XR Remoting singleton interface. Allows for streaming the XR application to a remote device.
///
/// Needs to be initialized before plXRInterface to be able to use remoting.
class plXRRemotingInterface
{
public:
  /// \brief Enable XR Remoting if available.
  static plCVarBool cvar_XrRemoting;
  /// \brief Hostname to connect to for XR Remoting.
  static plCVarString cvar_XrRemotingHostName;

  /// \brief Initializes the XR Remoting system. Needs to be done before plXRInterface is initialized.
  virtual plResult Initialize() = 0;
  /// \brief Shuts down XR Remoting. This will fail if XR actors still exists or if plXRInterface is still initialized.
  virtual plResult Deinitialize() = 0;
  /// \brief Returns whether XR Remoting is initialized.
  virtual bool IsInitialized() const = 0;

  /// \name Connection Functions
  ///@{

  /// \brief Tries to connect to the remote device.
  virtual plResult Connect(const char* szRemoteHostName, uint16_t remotePort = 8265, bool bEnableAudio = true, int iMaxBitrateKbps = 20000) = 0;
  /// \brief Disconnects from the remote device.
  virtual plResult Disconnect() = 0;
  /// \brief Get the current connection state to the remote device.
  virtual plEnum<plXRRemotingConnectionState> GetConnectionState() const = 0;
  /// \brief Returns the connection event to subscribe to connection changes.
  virtual plXRRemotingConnectionEvent& GetConnectionEvent() = 0;
  ///@}
};
