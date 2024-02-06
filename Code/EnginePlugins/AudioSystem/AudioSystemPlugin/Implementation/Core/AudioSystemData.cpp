#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemTransform, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Position", m_vPosition),
    PL_MEMBER_PROPERTY("Velocity", m_vVelocity),
    PL_MEMBER_PROPERTY("Forward", m_vForward),
    PL_MEMBER_PROPERTY("Up", m_vUp),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_ENUM(plAudioSystemSoundObstructionType, 1)
  PL_ENUM_CONSTANT(plAudioSystemSoundObstructionType::None),
  PL_ENUM_CONSTANT(plAudioSystemSoundObstructionType::SingleRay),
  PL_ENUM_CONSTANT(plAudioSystemSoundObstructionType::MultipleRay),
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plAudioSystemTriggerState, 1)
  PL_ENUM_CONSTANT(plAudioSystemTriggerState::Invalid),
  PL_ENUM_CONSTANT(plAudioSystemTriggerState::Playing),
  PL_ENUM_CONSTANT(plAudioSystemTriggerState::Ready),
  PL_ENUM_CONSTANT(plAudioSystemTriggerState::Loading),
  PL_ENUM_CONSTANT(plAudioSystemTriggerState::Unloading),
  PL_ENUM_CONSTANT(plAudioSystemTriggerState::Starting),
  PL_ENUM_CONSTANT(plAudioSystemTriggerState::Stopping),
  PL_ENUM_CONSTANT(plAudioSystemTriggerState::Stopped),
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemEntityData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemListenerData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemTriggerData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemRtpcData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemSwitchStateData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemEnvironmentData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemEventData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemSourceData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemBankData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PL_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_AudioSystemData);
