#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioSystemMessages.h>

#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgAudioSystemSetRtpcValue);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAudioSystemSetRtpcValue, 1, plRTTIDefaultAllocator<plMsgAudioSystemSetRtpcValue>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Value", m_fValue)->AddAttributes(new plDefaultValueAttribute(0.0f)),
    PLASMA_MEMBER_PROPERTY("Sync", m_bSync)->AddAttributes(new plDefaultValueAttribute(false)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgAudioSystemRtpcValueChanged);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAudioSystemRtpcValueChanged, 1, plRTTIDefaultAllocator<plMsgAudioSystemRtpcValueChanged>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Value", m_fValue)->AddAttributes(new plDefaultValueAttribute(0.0f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgAudioSystemSetSwitchState);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAudioSystemSetSwitchState, 1, plRTTIDefaultAllocator<plMsgAudioSystemSetSwitchState>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("State", m_sSwitchState),
    PLASMA_MEMBER_PROPERTY("Sync", m_bSync)->AddAttributes(new plDefaultValueAttribute(false)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgAudioSystemSwitchStateChanged);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAudioSystemSwitchStateChanged, 1, plRTTIDefaultAllocator<plMsgAudioSystemSwitchStateChanged>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("State", m_sSwitchState),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgAudioSystemSetEnvironmentAmount);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAudioSystemSetEnvironmentAmount, 1, plRTTIDefaultAllocator<plMsgAudioSystemSetEnvironmentAmount>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Value", m_fAmount)->AddAttributes(new plDefaultValueAttribute(0.0f)),
    PLASMA_MEMBER_PROPERTY("Sync", m_bSync)->AddAttributes(new plDefaultValueAttribute(false)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PLASMA_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_AudioSystemMessages);
