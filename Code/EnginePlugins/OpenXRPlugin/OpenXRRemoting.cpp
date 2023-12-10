#include <OpenXRPlugin/OpenXRPluginPCH.h>

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Reflection/ReflectionUtils.h>
#  include <Foundation/System/EnvironmentVariableUtils.h>
#  include <OpenXRPlugin/OpenXRDeclarations.h>
#  include <OpenXRPlugin/OpenXRRemoting.h>
#  include <OpenXRPlugin/OpenXRSingleton.h>

PLASMA_IMPLEMENT_SINGLETON(plOpenXRRemoting);

plOpenXRRemoting::plOpenXRRemoting(plOpenXR* pOpenXR)
  : m_SingletonRegistrar(this)
  , m_pOpenXR(pOpenXR)
{
}

plOpenXRRemoting::~plOpenXRRemoting()
{
}

plResult plOpenXRRemoting::Initialize()
{
  if (m_pOpenXR->IsInitialized())
  {
    return PLASMA_FAILURE;
  }

  plStringBuilder sRemotingJson = plOSFile::GetApplicationDirectory();
  sRemotingJson.AppendPath("RemotingXR.json");

  if (plOSFile::ExistsFile(sRemotingJson))
  {
    m_sPreviousRuntime = plEnvironmentVariableUtils::GetValueString("XR_RUNTIME_JSON");
    if (plEnvironmentVariableUtils::SetValueString("XR_RUNTIME_JSON", sRemotingJson).Failed())
    {
      plLog::Error("Failed to set environment variable XR_RUNTIME_JSON.");
      return PLASMA_FAILURE;
    }

    m_bInitialized = true;
    return PLASMA_SUCCESS;
  }
  else
  {
    plLog::Error("XR_RUNTIME_JSON not found: {}", sRemotingJson);
    return PLASMA_FAILURE;
  }
}

plResult plOpenXRRemoting::Deinitialize()
{
  if (!m_bInitialized)
  {
    return PLASMA_SUCCESS;
  }

  if (m_pOpenXR->IsInitialized())
    return PLASMA_FAILURE;

  m_bInitialized = false;
  SetEnvironmentVariableW(L"XR_RUNTIME_JSON", plStringWChar(m_sPreviousRuntime));
  m_sPreviousRuntime.Clear();
  return PLASMA_SUCCESS;
}

bool plOpenXRRemoting::IsInitialized() const
{
  return m_bInitialized;
}

plResult plOpenXRRemoting::Connect(const char* remoteHostName, uint16_t remotePort, bool enableAudio, int maxBitrateKbps)
{
  PLASMA_ASSERT_DEV(IsInitialized(), "Need to call 'plXRRemotingInterface::Initialize' first.");
  PLASMA_ASSERT_DEV(m_pOpenXR->IsInitialized(), "Need to call 'plXRInterface::Initialize' first.");

  XrRemotingRemoteContextPropertiesMSFT contextProperties;
  contextProperties = XrRemotingRemoteContextPropertiesMSFT{static_cast<XrStructureType>(XR_TYPE_REMOTING_REMOTE_CONTEXT_PROPERTIES_MSFT)};
  contextProperties.enableAudio = enableAudio;
  contextProperties.maxBitrateKbps = maxBitrateKbps;
  contextProperties.videoCodec = XR_REMOTING_VIDEO_CODEC_ANY_MSFT;
  contextProperties.depthBufferStreamResolution = XR_REMOTING_DEPTH_BUFFER_STREAM_RESOLUTION_HALF_MSFT;
  XrResult res = m_pOpenXR->m_extensions.pfn_xrRemotingSetContextPropertiesMSFT(m_pOpenXR->m_instance, m_pOpenXR->m_systemId, &contextProperties);
  if (res != XrResult::XR_SUCCESS)
  {
    XR_LOG_ERROR(res);
    return PLASMA_FAILURE;
  }

  XrRemotingConnectInfoMSFT connectInfo{static_cast<XrStructureType>(XR_TYPE_REMOTING_CONNECT_INFO_MSFT)};
  connectInfo.remoteHostName = remoteHostName;
  connectInfo.remotePort = remotePort;
  connectInfo.secureConnection = false;
  res = m_pOpenXR->m_extensions.pfn_xrRemotingConnectMSFT(m_pOpenXR->m_instance, m_pOpenXR->m_systemId, &connectInfo);
  if (res != XrResult::XR_SUCCESS)
  {
    return PLASMA_FAILURE;
  }
  {
    plXRRemotingConnectionEventData data;
    data.m_connectionState = plXRRemotingConnectionState::Connecting;
    data.m_disconnectReason = plXRRemotingDisconnectReason::None;
    m_event.Broadcast(data);
  }
  return PLASMA_SUCCESS;
}

plResult plOpenXRRemoting::Disconnect()
{
  if (!m_bInitialized || !m_pOpenXR->IsInitialized())
    return PLASMA_SUCCESS;

  XrRemotingDisconnectInfoMSFT disconnectInfo;
  XrResult res = m_pOpenXR->m_extensions.pfn_xrRemotingDisconnectMSFT(m_pOpenXR->m_instance, m_pOpenXR->m_systemId, &disconnectInfo);
  if (res != XrResult::XR_SUCCESS)
  {
    return PLASMA_FAILURE;
  }
  return PLASMA_SUCCESS;
}

plEnum<plXRRemotingConnectionState> plOpenXRRemoting::GetConnectionState() const
{
  if (!m_bInitialized || !m_pOpenXR->IsInitialized())
    return plXRRemotingConnectionState::Disconnected;

  XrRemotingConnectionStateMSFT connectionState;
  XrResult res = m_pOpenXR->m_extensions.pfn_xrRemotingGetConnectionStateMSFT(m_pOpenXR->m_instance, m_pOpenXR->m_systemId, &connectionState, nullptr);
  if (res != XrResult::XR_SUCCESS)
  {
    return plXRRemotingConnectionState::Disconnected;
  }
  switch (connectionState)
  {
    case XR_REMOTING_CONNECTION_STATE_DISCONNECTED_MSFT:
      return plXRRemotingConnectionState::Disconnected;
    case XR_REMOTING_CONNECTION_STATE_CONNECTING_MSFT:
      return plXRRemotingConnectionState::Connecting;
    case XR_REMOTING_CONNECTION_STATE_CONNECTED_MSFT:
      return plXRRemotingConnectionState::Connected;
    default:
      PLASMA_REPORT_FAILURE("Unknown enum value");
      break;
  }

  return plXRRemotingConnectionState::Disconnected;
}

plXRRemotingConnectionEvent& plOpenXRRemoting::GetConnectionEvent()
{
  return m_event;
}

void plOpenXRRemoting::HandleEvent(const XrEventDataBuffer& event)
{
  if (!m_bInitialized)
    return;

  switch (event.type)
  {
    case XR_TYPE_REMOTING_EVENT_DATA_CONNECTED_MSFT:
    {
      plXRRemotingConnectionEventData data;
      data.m_connectionState = plXRRemotingConnectionState::Connected;
      data.m_disconnectReason = plXRRemotingDisconnectReason::None;

      plLog::Info("XR Remoting connected.");
      m_event.Broadcast(data);
    }
    break;
    case XR_TYPE_REMOTING_EVENT_DATA_DISCONNECTED_MSFT:
    {
      plXRRemotingConnectionEventData data;
      data.m_connectionState = plXRRemotingConnectionState::Disconnected;
      XrRemotingDisconnectReasonMSFT reason = reinterpret_cast<const XrRemotingEventDataDisconnectedMSFT*>(&event)->disconnectReason;
      data.m_disconnectReason = static_cast<plXRRemotingDisconnectReason::Enum>(reason);

      plStringBuilder sTemp;
      plReflectionUtils::EnumerationToString(data.m_disconnectReason, sTemp, plReflectionUtils::EnumConversionMode::ValueNameOnly);
      plLog::Info("XR Remoting disconnected with reason: {}", sTemp);

      m_event.Broadcast(data);
    }
    break;
  }
}

#endif
