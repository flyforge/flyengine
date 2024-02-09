#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Types.h>

using plAudioSystemDataID = plUInt64;
using plAudioSystemControlID = plUInt64;

/// \brief Represents the value which marks an audio system data as invalid.
/// Valid audio system data should be greater than this number.
constexpr plAudioSystemDataID kInvalidAudioSystemId = 0;

/// \brief Stores the transformation data for an audio entity.
struct PL_AUDIOSYSTEMPLUGIN_DLL plAudioSystemTransform
{
  /// \brief The position of the entity in world space.
  plVec3 m_vPosition{0, 0, 0};

  /// \brief The velocity of the entity.
  plVec3 m_vVelocity{0, 0, 0};

  /// \brief The forward direction of the entity in world space.
  plVec3 m_vForward{0, 0, 0};

  /// \brief The up direction of the entity in world space.
  plVec3 m_vUp{0, 0, 0};

  bool operator==(const plAudioSystemTransform& rhs) const
  {
    return m_vPosition == rhs.m_vPosition && m_vForward == rhs.m_vForward && m_vUp == rhs.m_vUp && m_vVelocity == rhs.m_vVelocity;
  }

  bool operator!=(const plAudioSystemTransform& rhs) const
  {
    return !(*this == rhs);
  }
};

template <>
struct plHashHelper<plAudioSystemTransform>
{
  PL_ALWAYS_INLINE static plUInt32 Hash(const plAudioSystemTransform& value)
  {
    return plHashingUtils::CombineHashValues32(
      plHashHelper<plUInt64>::Hash(plMath::FloatToInt(value.m_vForward.x * value.m_vForward.y * value.m_vForward.z)),
      plHashingUtils::CombineHashValues32(
        plHashHelper<plUInt64>::Hash(plMath::FloatToInt(value.m_vPosition.x * value.m_vPosition.y * value.m_vPosition.z)),
        plHashingUtils::CombineHashValues32(
          plHashHelper<plUInt64>::Hash(plMath::FloatToInt(value.m_vUp.x * value.m_vUp.y * value.m_vUp.z)),
          plHashHelper<plUInt64>::Hash(plMath::FloatToInt(value.m_vVelocity.x * value.m_vVelocity.y * value.m_vVelocity.z)))));
  }

  PL_ALWAYS_INLINE static bool Equal(const plAudioSystemTransform& a, const plAudioSystemTransform& b)
  {
    return a == b;
  }
};

PL_DECLARE_REFLECTABLE_TYPE(PL_AUDIOSYSTEMPLUGIN_DLL, plAudioSystemTransform);

/// \brief The obstruction type applied to a sound. This affects the way that
/// ray casting works for an audio source.
struct PL_AUDIOSYSTEMPLUGIN_DLL plAudioSystemSoundObstructionType
{
  using StorageType = plUInt8;

  enum Enum : StorageType
  {
    /// \brief No ray casting is done for this sound.
    /// The sound is neither obstructed nor occluded.
    None = 0,

    /// \brief Only one ray is shot at each frame.
    /// The occlusion value will be averaged over time.
    /// The sound will not be obstructed, since only one ray is not enough
    /// to compute this information.
    SingleRay,

    /// \brief Multiple rays are shot at each frame.
    /// The occlusion and obstructions values will be averaged over time.
    MultipleRay,

    Default = SingleRay
  };

  PL_ENUM_TO_STRING(None, SingleRay, MultipleRay);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_AUDIOSYSTEMPLUGIN_DLL, plAudioSystemSoundObstructionType);

/// \brief The state of an audio trigger.
struct PL_AUDIOSYSTEMPLUGIN_DLL plAudioSystemTriggerState
{
  using StorageType = plUInt8;

  enum Enum : StorageType
  {
    /// \brief The trigger have no state, this means it has not been loaded yet
    /// by the audio middleware.
    Invalid = 0,

    /// \brief The trigger is activated and currently playing an event.
    Playing = 1,

    /// \brief The trigger is ready to be activated. This state is set after the
    /// trigger is loaded through LoadTrigger.
    Ready = 2,

    /// \brief The trigger is being loaded.
    Loading = 3,

    /// \brief The trigger is being unloaded.
    Unloading = 4,

    /// \brief The trigger is being activated.
    Starting = 5,

    /// \brief The trigger is being stopped.
    Stopping = 6,

    /// \brief The trigger is stopped, and not playing an event.
    Stopped = 7,

    Default = Invalid,
  };

  PL_ENUM_TO_STRING(Invalid, Playing, Ready, Loading, Unloading, Starting, Stopping, Stopped);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_AUDIOSYSTEMPLUGIN_DLL, plAudioSystemTriggerState);

/// \brief The state of an audio source.
enum class plAudioSystemEventState : plUInt8
{
  /// \brief The event have no state, this means it has not been loaded yet,
  /// nor triggered by any trigger.
  Invalid = 0,

  /// \brief The event is currently playing audio.
  Playing = 1,

  /// \brief The event is loading.
  Loading = 2,

  /// \brief The event is being unloaded.
  Unloading = 3,
};

/// \brief Base class for an audio middleware entity.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioSystemEntityData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAudioSystemEntityData, plReflectedClass);

public:
  ~plAudioSystemEntityData() override = default;
};

/// \brief Base class for an audio middleware listener.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioSystemListenerData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAudioSystemListenerData, plReflectedClass);

public:
  ~plAudioSystemListenerData() override = default;
};

/// \brief Base class for an audio middleware trigger.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioSystemTriggerData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAudioSystemTriggerData, plReflectedClass);

public:
  ~plAudioSystemTriggerData() override = default;
};

/// \brief Base class for an audio middleware RTPC.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRtpcData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAudioSystemRtpcData, plReflectedClass);

public:
  ~plAudioSystemRtpcData() override = default;
};

/// \brief Base class for an audio middleware switch state.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioSystemSwitchStateData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAudioSystemSwitchStateData, plReflectedClass);

public:
  ~plAudioSystemSwitchStateData() override = default;
};

/// \brief Base class for an audio middleware environment.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioSystemEnvironmentData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAudioSystemEnvironmentData, plReflectedClass);

public:
  ~plAudioSystemEnvironmentData() override = default;
};

/// \brief Base class for an audio middleware event.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioSystemEventData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAudioSystemEventData, plReflectedClass);

  ~plAudioSystemEventData() override = default;
};

/// \brief Base class for an audio middleware source.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioSystemSourceData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAudioSystemSourceData, plReflectedClass);

public:
  ~plAudioSystemSourceData() override = default;
};

/// \brief Base class for an audio middleware bank file.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioSystemBankData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAudioSystemBankData, plReflectedClass);

public:
  ~plAudioSystemBankData() override = default;
};
