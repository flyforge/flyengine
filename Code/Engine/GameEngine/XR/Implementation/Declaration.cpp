#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/Declarations.h>
#include <GameEngine/XR/XRRemotingInterface.h>

plCVarBool plXRRemotingInterface::cvar_XrRemoting("XR.Remoting", false, plCVarFlags::Default, "Enable XR Remoting if available.");
plCVarString plXRRemotingInterface::cvar_XrRemotingHostName("XR.Remoting.HostName", "", plCVarFlags::Save, "Hostname to connect to for XR Remoting.");

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plXRTransformSpace, 1)
  PLASMA_BITFLAGS_CONSTANTS(plXRTransformSpace::Local, plXRTransformSpace::Global)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plXRDeviceType, 1)
  PLASMA_BITFLAGS_CONSTANTS(plXRDeviceType::HMD, plXRDeviceType::LeftController, plXRDeviceType::RightController)
  PLASMA_BITFLAGS_CONSTANTS(plXRDeviceType::DeviceID0, plXRDeviceType::DeviceID1, plXRDeviceType::DeviceID2, plXRDeviceType::DeviceID3)
  PLASMA_BITFLAGS_CONSTANTS(plXRDeviceType::DeviceID4, plXRDeviceType::DeviceID5, plXRDeviceType::DeviceID6, plXRDeviceType::DeviceID7)
  PLASMA_BITFLAGS_CONSTANTS(plXRDeviceType::DeviceID8, plXRDeviceType::DeviceID9, plXRDeviceType::DeviceID10, plXRDeviceType::DeviceID11)
  PLASMA_BITFLAGS_CONSTANTS(plXRDeviceType::DeviceID12, plXRDeviceType::DeviceID13, plXRDeviceType::DeviceID14, plXRDeviceType::DeviceID15)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plXRRemotingConnectionState, 1)
  PLASMA_BITFLAGS_CONSTANTS(plXRRemotingConnectionState::Disconnected, plXRRemotingConnectionState::Connecting, plXRRemotingConnectionState::Connected)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plXRRemotingDisconnectReason, 1)
  PLASMA_BITFLAGS_CONSTANTS(plXRRemotingDisconnectReason::None, plXRRemotingDisconnectReason::Unknown, plXRRemotingDisconnectReason::NoServerCertificate)
  PLASMA_BITFLAGS_CONSTANTS(plXRRemotingDisconnectReason::HandshakePortBusy, plXRRemotingDisconnectReason::HandshakeUnreachable, plXRRemotingDisconnectReason::HandshakeConnectionFailed)
  PLASMA_BITFLAGS_CONSTANTS(plXRRemotingDisconnectReason::AuthenticationFailed, plXRRemotingDisconnectReason::RemotingVersionMismatch, plXRRemotingDisconnectReason::IncompatibleTransportProtocols)
  PLASMA_BITFLAGS_CONSTANTS(plXRRemotingDisconnectReason::HandshakeFailed, plXRRemotingDisconnectReason::TransportPortBusy, plXRRemotingDisconnectReason::TransportUnreachable)
  PLASMA_BITFLAGS_CONSTANTS(plXRRemotingDisconnectReason::TransportConnectionFailed, plXRRemotingDisconnectReason::ProtocolVersionMismatch, plXRRemotingDisconnectReason::ProtocolError)
  PLASMA_BITFLAGS_CONSTANTS(plXRRemotingDisconnectReason::VideoCodecNotAvailable, plXRRemotingDisconnectReason::Canceled, plXRRemotingDisconnectReason::ConnectionLost)
  PLASMA_BITFLAGS_CONSTANTS(plXRRemotingDisconnectReason::DeviceLost, plXRRemotingDisconnectReason::DisconnectRequest, plXRRemotingDisconnectReason::HandshakeNetworkUnreachable)
  PLASMA_BITFLAGS_CONSTANTS(plXRRemotingDisconnectReason::HandshakeConnectionRefused, plXRRemotingDisconnectReason::VideoFormatNotAvailable, plXRRemotingDisconnectReason::PeerDisconnectRequest)
  PLASMA_BITFLAGS_CONSTANTS(plXRRemotingDisconnectReason::PeerDisconnectTimeout, plXRRemotingDisconnectReason::SessionOpenTimeout, plXRRemotingDisconnectReason::RemotingHandshakeTimeout)
  PLASMA_BITFLAGS_CONSTANTS(plXRRemotingDisconnectReason::InternalError)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

plXRDeviceState::plXRDeviceState()
{
  m_vGripPosition.SetZero();
  m_qGripRotation.SetIdentity();

  m_vAimPosition.SetZero();
  m_qAimRotation.SetIdentity();
}


PLASMA_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_Declaration);
