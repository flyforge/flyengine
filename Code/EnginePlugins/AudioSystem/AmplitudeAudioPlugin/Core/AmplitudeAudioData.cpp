#include <AmplitudeAudioPlugin/AmplitudeAudioPluginPCH.h>

#include <AmplitudeAudioPlugin/Core/AmplitudeAudioData.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioEntityData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioListenerData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioTriggerData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioRtpcData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioSwitchStateData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioEnvironmentData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioEventData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioSoundBankData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plAmplitudeAudioControlType, 1)
  PLASMA_ENUM_CONSTANT(plAmplitudeAudioControlType::Invalid),
  PLASMA_ENUM_CONSTANT(plAmplitudeAudioControlType::Trigger),
  PLASMA_ENUM_CONSTANT(plAmplitudeAudioControlType::Rtpc),
  PLASMA_ENUM_CONSTANT(plAmplitudeAudioControlType::SoundBank),
  PLASMA_ENUM_CONSTANT(plAmplitudeAudioControlType::Switch),
  PLASMA_ENUM_CONSTANT(plAmplitudeAudioControlType::SwitchState),
  PLASMA_ENUM_CONSTANT(plAmplitudeAudioControlType::Environment),
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on
