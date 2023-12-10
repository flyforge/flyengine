#pragma once

#include "Foundation/IO/Stream.h"
#include <AmplitudeAudioPlugin/AmplitudeAudioPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>

#include <SparkyStudios/Audio/Amplitude/Amplitude.h>
#include <SparkyStudios/Audio/Amplitude/Core/Common/Types.h>

/// \brief The type of a control. This is used by control assets to determine the type of the control
/// when the audio system is parsing them.
struct PLASMA_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeAudioControlType
{
  using StorageType = plUInt8;

  enum Enum : StorageType
  {
    /// \brief The control is not known to the audio system.
    Invalid = 0,

    /// \brief The control is a source.
    Trigger = 1,

    /// \brief The control is a real-time parameter.
    Rtpc = 2,

    /// \brief The control is a sound bank.
    SoundBank = 3,

    /// \brief The control is a switch container.
    Switch = 4,

    /// \brief The control is a switch state.
    SwitchState = 5,

    /// \brief The control is an environment effect.
    Environment = 6,

    Default = Invalid,
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_AMPLITUDEAUDIOPLUGIN_DLL, plAmplitudeAudioControlType);

class PLASMA_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeAudioEntityData : public plAudioSystemEntityData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioEntityData, plAudioSystemEntityData);

public:
  explicit plAmplitudeAudioEntityData(const SparkyStudios::Audio::Amplitude::AmEntityID uiEntityID, bool bHasPosition = true)
    : plAudioSystemEntityData()
    , m_uiAmId(uiEntityID)
    , m_bHasPosition(bHasPosition)
  {
  }

  bool m_bHasPosition;
  const SparkyStudios::Audio::Amplitude::AmEntityID m_uiAmId;
};

class PLASMA_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeAudioListenerData : public plAudioSystemListenerData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioListenerData, plAudioSystemListenerData);

public:
  explicit plAmplitudeAudioListenerData(const SparkyStudios::Audio::Amplitude::AmEntityID uiListenerID)
    : plAudioSystemListenerData()
    , m_uiAmId(uiListenerID)
  {
  }

  const SparkyStudios::Audio::Amplitude::AmListenerID m_uiAmId;
};

class PLASMA_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeAudioTriggerData : public plAudioSystemTriggerData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioTriggerData, plAudioSystemTriggerData);

public:
  explicit plAmplitudeAudioTriggerData(const SparkyStudios::Audio::Amplitude::AmEventID uiEventId)
    : plAudioSystemTriggerData()
    , m_uiAmId(uiEventId)
  {
  }

  const SparkyStudios::Audio::Amplitude::AmEventID m_uiAmId;
};

class PLASMA_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeAudioRtpcData : public plAudioSystemRtpcData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioRtpcData, plAudioSystemRtpcData);

public:
  explicit plAmplitudeAudioRtpcData(const SparkyStudios::Audio::Amplitude::AmRtpcID uiRtpcId)
    : plAudioSystemRtpcData()
    , m_uiAmId(uiRtpcId)
  {
  }

  const SparkyStudios::Audio::Amplitude::AmRtpcID m_uiAmId;
};

class PLASMA_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeAudioSwitchStateData : public plAudioSystemSwitchStateData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioSwitchStateData, plAudioSystemSwitchStateData);

public:
  explicit plAmplitudeAudioSwitchStateData(const SparkyStudios::Audio::Amplitude::AmSwitchID uiSwitchId, const SparkyStudios::Audio::Amplitude::AmObjectID uiSwitchStateId)
    : plAudioSystemSwitchStateData()
    , m_uiSwitchId(uiSwitchId)
    , m_uiSwitchStateId(uiSwitchStateId)
  {
  }

  const SparkyStudios::Audio::Amplitude::AmSwitchID m_uiSwitchId;
  const SparkyStudios::Audio::Amplitude::AmObjectID m_uiSwitchStateId;
};

class PLASMA_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeAudioEnvironmentData : public plAudioSystemEnvironmentData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioEnvironmentData, plAudioSystemEnvironmentData);

public:
  explicit plAmplitudeAudioEnvironmentData(const SparkyStudios::Audio::Amplitude::AmEnvironmentID uiEnvironmentId, const SparkyStudios::Audio::Amplitude::AmEffectID uiEffectId)
    : plAudioSystemEnvironmentData()
    , m_uiAmId(uiEnvironmentId)
    , m_uiEffectId(uiEffectId)
  {
  }

  const SparkyStudios::Audio::Amplitude::AmEnvironmentID m_uiAmId;
  const SparkyStudios::Audio::Amplitude::AmEffectID m_uiEffectId;
};

class PLASMA_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeAudioEventData : public plAudioSystemEventData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioEventData, plAudioSystemEventData);

public:
  explicit plAmplitudeAudioEventData(const SparkyStudios::Audio::Amplitude::AmEventID uiEventId)
    : plAudioSystemEventData()
    , m_uiAmId(uiEventId)
  {
  }

  plAudioSystemEventState m_eState{plAudioSystemEventState::Invalid};
  SparkyStudios::Audio::Amplitude::EventCanceler m_EventCanceler;
  const SparkyStudios::Audio::Amplitude::AmEventID m_uiAmId;
};

class PLASMA_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeAudioSoundBankData : public plAudioSystemBankData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioSoundBankData, plAudioSystemBankData);

public:
  explicit plAmplitudeAudioSoundBankData(const SparkyStudios::Audio::Amplitude::AmBankID uiBankId, const plString& sFileName)
    : plAudioSystemBankData()
    , m_uiAmId(uiBankId)
    , m_sFileName(sFileName)
  {
  }

  const SparkyStudios::Audio::Amplitude::AmEventID m_uiAmId;
  const plString m_sFileName;
};
