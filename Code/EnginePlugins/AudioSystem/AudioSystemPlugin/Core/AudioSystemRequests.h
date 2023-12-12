#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>

#include <Foundation/Types/Status.h>
#include <Foundation/Types/VariantType.h>


/// \brief Helper macro the declare the callback value of an audio request.
#define PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_CALLBACK(name) \
  using CallbackType = plDelegate<void(const name&)>; \
  CallbackType m_Callback


/// \brief Helper macro to declare a new audio system request.
#define PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE_SIMPLE(name)                                      \
  PLASMA_DECLARE_POD_TYPE();                                                                      \
  bool operator==(const name& rhs) const                                                          \
  {                                                                                               \
    return static_cast<plAudioSystemRequest>(*this) == static_cast<plAudioSystemRequest>(rhs);    \
  }                                                                                               \
  bool operator!=(const name& rhs) const                                                          \
  {                                                                                               \
    return !(*this == rhs);                                                                       \
  }                                                                                               \
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_CALLBACK(name)


/// \brief Helper macro to declare a new audio system request.
#define PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(name, eq_ex)                                                  \
  PLASMA_DECLARE_POD_TYPE();                                                                                  \
  bool operator==(const name& rhs) const                                                                      \
  {                                                                                                           \
    return static_cast<plAudioSystemRequest>(*this) == static_cast<plAudioSystemRequest>(rhs) && (eq_ex);     \
  }                                                                                                           \
  bool operator!=(const name& rhs) const                                                                      \
  {                                                                                                           \
    return !(*this == rhs);                                                                                   \
  }                                                                                                           \
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_CALLBACK(name)


/// \brief Helper macro to declare an plHashHelper implementation of an audio system request.
#define PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_HASH_SIMPLE(name)      \
  template <>                                                     \
  struct plHashHelper<name>                                       \
  {                                                               \
    PLASMA_ALWAYS_INLINE static plUInt32 Hash(const name& value)  \
    {                                                             \
      return plHashHelper<plAudioSystemRequest>::Hash(value);     \
    }                                                             \
    PLASMA_ALWAYS_INLINE static bool                              \
    Equal(const name& a, const name& b)                           \
    {                                                             \
      return a == b;                                              \
    }                                                             \
  }


/// \brief Helper macro to declare an plHashHelper implementation of an audio system request.
#define PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_HASH(name, hash_ex)                                                    \
  template <>                                                                                                     \
  struct plHashHelper<name>                                                                                       \
  {                                                                                                               \
    PLASMA_ALWAYS_INLINE static plUInt32 Hash(const name& value)                                                  \
    {                                                                                                             \
      return plHashingUtils::CombineHashValues32(plHashHelper<plAudioSystemRequest>::Hash(value), (hash_ex));     \
    }                                                                                                             \
    PLASMA_ALWAYS_INLINE static bool                                                                              \
    Equal(const name& a, const name& b)                                                                           \
    {                                                                                                             \
      return a == b;                                                                                              \
    }                                                                                                             \
  }


/// \brief Helper macro to declare stream operators for an audio system request.
#define PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_STREAM_OPERATORS(name)                          \
  PLASMA_AUDIOSYSTEMPLUGIN_DLL void operator<<(plStreamWriter& Stream, const name& Value); \
  PLASMA_AUDIOSYSTEMPLUGIN_DLL void operator>>(plStreamReader& Stream, name& Value)


/// \brief Helper macro to declare a new audio system request.
#define PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(name)            \
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_HASH_SIMPLE(name);            \
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_STREAM_OPERATORS(name);       \
  PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_AUDIOSYSTEMPLUGIN_DLL, name); \
  PLASMA_DECLARE_CUSTOM_VARIANT_TYPE(name)


/// \brief Helper macro to declare a new audio system request.
#define PLASMA_DECLARE_AUDIOSYSTEM_REQUEST(name, hash_ex)          \
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_HASH(name, hash_ex);          \
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_STREAM_OPERATORS(name);       \
  PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_AUDIOSYSTEMPLUGIN_DLL, name); \
  PLASMA_DECLARE_CUSTOM_VARIANT_TYPE(name)


/// \brief Helper macro to define stream operators for audio system
/// requests which doesn't add more data.
#define PLASMA_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(name)   \
  void operator<<(plStreamWriter& Stream, const name& Value)      \
  {                                                               \
    Stream << static_cast<const plAudioSystemRequest&>(Value);    \
  }                                                               \
  void operator>>(plStreamReader& Stream, name& Value)            \
  {                                                               \
    Stream >> static_cast<plAudioSystemRequest&>(Value);          \
  }


/// \brief Base class for all audio system requests.
///
/// An audio request is a message sent to the audio system. It contains a type and a payload.
/// The payload depend on the audio request purpose (load trigger, update listener, shutdown system, etc.).
///
/// To send an audio request, use the plAudioSystem::SendRequest() function. Audio requests sent this way
/// will be executed asynchronously in the audio thread.
///
/// Each audio requests can have callbacks, this can be useful when you want to do some logic after an
/// asynchronous audio request as been sent to the audio system. The callbacks will be executed in the
/// main thread.
///
/// If you need to send an audio request synchronously, use the plAudioSystem::SendRequestSync() function.
/// This will block the main thread until the request has been processed.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequest
{
  PLASMA_DECLARE_POD_TYPE();

  /// \brief The audio entity which is being manipulated, if any.
  plAudioSystemDataID m_uiEntityId{0};

  /// \brief The audio listener which is being manipulated, if any.
  plAudioSystemDataID m_uiListenerId{0};

  /// \brief The audio object (trigger, rtpc, environment, etc.) which is being manipulated, if any.
  plAudioSystemDataID m_uiObjectId{0};

  /// \brief The status of the audio request.
  plStatus m_eStatus{PLASMA_FAILURE};

  bool operator==(const plAudioSystemRequest& rhs) const
  {
    return m_uiEntityId == rhs.m_uiEntityId && m_uiListenerId == rhs.m_uiListenerId && m_uiObjectId == rhs.m_uiObjectId && m_eStatus.Succeeded() == rhs.m_eStatus.Succeeded();
  }

  bool operator!=(const plAudioSystemRequest& rhs) const
  {
    return !(*this == rhs);
  }
};

template <>
struct plHashHelper<plAudioSystemRequest>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(const plAudioSystemRequest& value)
  {
    return plHashHelper<plUInt64>::Hash(value.m_uiEntityId) * plHashHelper<plUInt64>::Hash(value.m_uiListenerId) * plHashHelper<plUInt64>::Hash(value.m_uiObjectId);
  }

  PLASMA_ALWAYS_INLINE static bool Equal(const plAudioSystemRequest& a, const plAudioSystemRequest& b)
  {
    return a == b;
  }
};

/// \brief Audio request to register a new entity in the audio system.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestRegisterEntity : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(plAudioSystemRequestRegisterEntity, m_sName == rhs.m_sName);

  /// \brief A friendly name for the entity. Not very used by most of the audio engines for
  /// other purposes than debugging.
  plString m_sName;
};

/// \brief Audio request to set the transform and velocity of an entity.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestSetEntityTransform : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(plAudioSystemRequestSetEntityTransform, m_Transform == rhs.m_Transform);

  plAudioSystemTransform m_Transform;
};

/// \brief Audio request to unregister an entity from the audio system.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestUnregisterEntity : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE_SIMPLE(plAudioSystemRequestUnregisterEntity);
};

/// \brief Audio request to register a new listener in the audio system.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestRegisterListener : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(plAudioSystemRequestRegisterListener, m_sName == rhs.m_sName);

  /// \brief A friendly name for the listener. Not very used by most of the audio engines for
  /// other purposes than debugging.
  plString m_sName;
};

/// \brief Audio request to set the transform and velocity of a listener.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestSetListenerTransform : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(plAudioSystemRequestSetListenerTransform, m_Transform == rhs.m_Transform);

  plAudioSystemTransform m_Transform;
};

/// \brief Audio request to unregister a listener from the audio system.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestUnregisterListener : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE_SIMPLE(plAudioSystemRequestUnregisterListener);
};

/// \brief Audio request to load data needed to activate a trigger.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestLoadTrigger : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(plAudioSystemRequestLoadTrigger, m_uiEventId == rhs.m_uiEventId);

  /// \brief The event that this trigger should start.
  plAudioSystemDataID m_uiEventId{0};
};

/// \brief Audio request to activate a trigger.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestActivateTrigger : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(plAudioSystemRequestActivateTrigger, m_uiEventId == rhs.m_uiEventId);

  /// \brief The event that this trigger should start.
  plAudioSystemDataID m_uiEventId{0};
};

/// \brief Audio request to stop an event.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestStopEvent : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(plAudioSystemRequestStopEvent, m_uiTriggerId == rhs.m_uiTriggerId);

  /// \brief The trigger which have started this event.
  plAudioSystemDataID m_uiTriggerId{0};
};

/// \brief Audio request to unload data loaded by a trigger.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestUnloadTrigger : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE_SIMPLE(plAudioSystemRequestUnloadTrigger);
};

/// \brief Audio request to set the value of a real-time parameter.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestSetRtpcValue : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(plAudioSystemRequestSetRtpcValue, m_fValue == rhs.m_fValue);

  /// \brief The new parameter's value.
  float m_fValue{0.0f};
};

/// \brief Audio request to set the active state of a switch.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestSetSwitchState : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE_SIMPLE(plAudioSystemRequestSetSwitchState);
};

/// \brief Audio request to set the current amount of an environment on the specified entity.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestSetEnvironmentAmount : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(plAudioSystemRequestSetEnvironmentAmount, m_fAmount == rhs.m_fAmount);

  /// \brief The new environment amount.
  float m_fAmount{0.0f};
};

/// \brief Audio request to set the current amount of an environment on the specified entity.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestSetObstructionOcclusion : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(plAudioSystemRequestSetObstructionOcclusion, m_fObstruction == rhs.m_fObstruction && m_fOcclusion == rhs.m_fOcclusion);

  /// \brief The new obstruction value.
  float m_fObstruction{0.0f};

  /// \brief The new occlusion value.
  float m_fOcclusion{0.0f};
};

/// \brief Audio request to load a sound bank.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestLoadBank : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE_SIMPLE(plAudioSystemRequestLoadBank);
};

/// \brief Audio request to unload a sound bank.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestUnloadBank : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE_SIMPLE(plAudioSystemRequestUnloadBank);
};

/// \brief Audio request to shutdown the audio system. Used internally only. Sending this request
/// at runtime will lead to unspecified behaviors.
struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemRequestShutdown : public plAudioSystemRequest
{
  PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_TYPE_SIMPLE(plAudioSystemRequestShutdown);
};

PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_STREAM_OPERATORS(plAudioSystemRequest);
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_AUDIOSYSTEMPLUGIN_DLL, plAudioSystemRequest);

PLASMA_DECLARE_AUDIOSYSTEM_REQUEST(plAudioSystemRequestRegisterEntity, plHashHelper<plString>::Hash(value.m_sName));
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST(plAudioSystemRequestSetEntityTransform, plHashHelper<plAudioSystemTransform>::Hash(value.m_Transform));
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(plAudioSystemRequestUnregisterEntity);
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST(plAudioSystemRequestRegisterListener, plHashHelper<plString>::Hash(value.m_sName));
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST(plAudioSystemRequestSetListenerTransform, plHashHelper<plAudioSystemTransform>::Hash(value.m_Transform));
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(plAudioSystemRequestUnregisterListener);
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST(plAudioSystemRequestLoadTrigger, plHashHelper<plAudioSystemDataID>::Hash(value.m_uiEventId));
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST(plAudioSystemRequestActivateTrigger, plHashHelper<plAudioSystemDataID>::Hash(value.m_uiEventId));
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST(plAudioSystemRequestStopEvent, plHashHelper<plAudioSystemDataID>::Hash(value.m_uiTriggerId));
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(plAudioSystemRequestUnloadTrigger);
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST(plAudioSystemRequestSetRtpcValue, plHashHelper<plInt32>::Hash(plMath::FloatToInt(value.m_fValue * 1000.0f)));
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(plAudioSystemRequestSetSwitchState);
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST(plAudioSystemRequestSetEnvironmentAmount, plHashHelper<plInt32>::Hash(plMath::FloatToInt(value.m_fAmount * 1000.0f)));
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST(plAudioSystemRequestSetObstructionOcclusion, plHashHelper<plInt32>::Hash(plMath::FloatToInt(value.m_fObstruction * 1000.0f)) * plHashHelper<plInt32>::Hash(plMath::FloatToInt(value.m_fOcclusion * 1000.0f)));
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(plAudioSystemRequestLoadBank);
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(plAudioSystemRequestUnloadBank);
PLASMA_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(plAudioSystemRequestShutdown);

/// \brief A functor used by plVariant to call an audio request callback.
struct CallRequestCallbackFunc
{
  explicit CallRequestCallbackFunc(const plVariant& value)
    : m_Value(value)
  {
  }

  template <typename T>
  PLASMA_ALWAYS_INLINE void Call() const
  {
    plResult conversionStatus = PLASMA_SUCCESS;
    const auto& request = m_Value.ConvertTo<T>(&conversionStatus);

    if (conversionStatus.Succeeded())
    {
      request.m_Callback(request);
    }
  }

  template <typename T>
  void operator()()
  {
    if (!m_Value.IsValid())
      return;

    if (m_Value.IsA<plAudioSystemRequestRegisterEntity>())
    {
      Call<plAudioSystemRequestRegisterEntity>();
    }
    else if (m_Value.IsA<plAudioSystemRequestSetEntityTransform>())
    {
      Call<plAudioSystemRequestSetEntityTransform>();
    }
    else if (m_Value.IsA<plAudioSystemRequestUnregisterEntity>())
    {
      Call<plAudioSystemRequestUnregisterEntity>();
    }
    else if (m_Value.IsA<plAudioSystemRequestRegisterListener>())
    {
      Call<plAudioSystemRequestRegisterListener>();
    }
    else if (m_Value.IsA<plAudioSystemRequestSetListenerTransform>())
    {
      Call<plAudioSystemRequestSetListenerTransform>();
    }
    else if (m_Value.IsA<plAudioSystemRequestUnregisterListener>())
    {
      Call<plAudioSystemRequestUnregisterListener>();
    }
    else if (m_Value.IsA<plAudioSystemRequestLoadTrigger>())
    {
      Call<plAudioSystemRequestLoadTrigger>();
    }
    else if (m_Value.IsA<plAudioSystemRequestActivateTrigger>())
    {
      Call<plAudioSystemRequestActivateTrigger>();
    }
    else if (m_Value.IsA<plAudioSystemRequestStopEvent>())
    {
      Call<plAudioSystemRequestStopEvent>();
    }
    else if (m_Value.IsA<plAudioSystemRequestSetRtpcValue>())
    {
      Call<plAudioSystemRequestSetRtpcValue>();
    }
    else if (m_Value.IsA<plAudioSystemRequestSetSwitchState>())
    {
      Call<plAudioSystemRequestSetSwitchState>();
    }
    else if (m_Value.IsA<plAudioSystemRequestSetEnvironmentAmount>())
    {
      Call<plAudioSystemRequestSetEnvironmentAmount>();
    }
    else if (m_Value.IsA<plAudioSystemRequestSetObstructionOcclusion>())
    {
      Call<plAudioSystemRequestSetObstructionOcclusion>();
    }
    else if (m_Value.IsA<plAudioSystemRequestLoadBank>())
    {
      Call<plAudioSystemRequestLoadBank>();
    }
    else if (m_Value.IsA<plAudioSystemRequestUnloadBank>())
    {
      Call<plAudioSystemRequestUnloadBank>();
    }
    else if (m_Value.IsA<plAudioSystemRequestShutdown>())
    {
      Call<plAudioSystemRequestShutdown>();
    }
    else
    {
      plLog::Error("[AudioSystem] Received an Audio Request with an invalid type");
    }
  }

  const plVariant& m_Value;
};
