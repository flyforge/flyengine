#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioSystemMessages.h>

#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
PL_IMPLEMENT_MESSAGE_TYPE(plMsgAudioSystemSetRtpcValue);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAudioSystemSetRtpcValue, 1, plRTTIDefaultAllocator<plMsgAudioSystemSetRtpcValue>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Value", m_fValue)->AddAttributes(new plDefaultValueAttribute(0.0f)),
    PL_MEMBER_PROPERTY("Sync", m_bSync)->AddAttributes(new plDefaultValueAttribute(false)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgAudioSystemRtpcValueChanged);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAudioSystemRtpcValueChanged, 1, plRTTIDefaultAllocator<plMsgAudioSystemRtpcValueChanged>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Value", m_fValue)->AddAttributes(new plDefaultValueAttribute(0.0f)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgAudioSystemSetSwitchState);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAudioSystemSetSwitchState, 1, plRTTIDefaultAllocator<plMsgAudioSystemSetSwitchState>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("State", m_sSwitchState),
    PL_MEMBER_PROPERTY("Sync", m_bSync)->AddAttributes(new plDefaultValueAttribute(false)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgAudioSystemSwitchStateChanged);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAudioSystemSwitchStateChanged, 1, plRTTIDefaultAllocator<plMsgAudioSystemSwitchStateChanged>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("State", m_sSwitchState),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgAudioSystemSetEnvironmentAmount);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAudioSystemSetEnvironmentAmount, 1, plRTTIDefaultAllocator<plMsgAudioSystemSetEnvironmentAmount>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Value", m_fAmount)->AddAttributes(new plDefaultValueAttribute(0.0f)),
    PL_MEMBER_PROPERTY("Sync", m_bSync)->AddAttributes(new plDefaultValueAttribute(false)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PL_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_AudioSystemMessages);
