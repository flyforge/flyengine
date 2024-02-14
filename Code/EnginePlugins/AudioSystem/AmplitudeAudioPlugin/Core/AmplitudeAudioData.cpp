#include <AmplitudeAudioPlugin/AmplitudeAudioPluginPCH.h>

#include <AmplitudeAudioPlugin/Core/AmplitudeAudioData.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioEntityData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioListenerData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioTriggerData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioRtpcData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioSwitchStateData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioEnvironmentData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioEventData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioSoundBankData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_ENUM(plAmplitudeAudioControlType, 1)
  PL_ENUM_CONSTANT(plAmplitudeAudioControlType::Invalid),
  PL_ENUM_CONSTANT(plAmplitudeAudioControlType::Trigger),
  PL_ENUM_CONSTANT(plAmplitudeAudioControlType::Rtpc),
  PL_ENUM_CONSTANT(plAmplitudeAudioControlType::SoundBank),
  PL_ENUM_CONSTANT(plAmplitudeAudioControlType::Switch),
  PL_ENUM_CONSTANT(plAmplitudeAudioControlType::SwitchState),
  PL_ENUM_CONSTANT(plAmplitudeAudioControlType::Environment),
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on
