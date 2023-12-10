#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemTransform, plNoBase, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Position", m_vPosition),
    PLASMA_MEMBER_PROPERTY("Velocity", m_vVelocity),
    PLASMA_MEMBER_PROPERTY("Forward", m_vForward),
    PLASMA_MEMBER_PROPERTY("Up", m_vUp),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plAudioSystemSoundObstructionType, 1)
  PLASMA_ENUM_CONSTANT(plAudioSystemSoundObstructionType::None),
  PLASMA_ENUM_CONSTANT(plAudioSystemSoundObstructionType::SingleRay),
  PLASMA_ENUM_CONSTANT(plAudioSystemSoundObstructionType::MultipleRay),
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plAudioSystemTriggerState, 1)
  PLASMA_ENUM_CONSTANT(plAudioSystemTriggerState::Invalid),
  PLASMA_ENUM_CONSTANT(plAudioSystemTriggerState::Playing),
  PLASMA_ENUM_CONSTANT(plAudioSystemTriggerState::Ready),
  PLASMA_ENUM_CONSTANT(plAudioSystemTriggerState::Loading),
  PLASMA_ENUM_CONSTANT(plAudioSystemTriggerState::Unloading),
  PLASMA_ENUM_CONSTANT(plAudioSystemTriggerState::Starting),
  PLASMA_ENUM_CONSTANT(plAudioSystemTriggerState::Stopping),
  PLASMA_ENUM_CONSTANT(plAudioSystemTriggerState::Stopped),
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemEntityData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemListenerData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemTriggerData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemRtpcData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemSwitchStateData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemEnvironmentData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemEventData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemSourceData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemBankData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PLASMA_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_AudioSystemData);
