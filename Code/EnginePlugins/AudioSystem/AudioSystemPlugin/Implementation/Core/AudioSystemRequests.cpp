#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioSystemRequests.h>

#include <Foundation/Types/VariantTypeRegistry.h>

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequest, plNoBase, 1, plRTTIDefaultAllocator<plAudioSystemRequest>)
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestRegisterEntity);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestRegisterEntity, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestRegisterEntity>)
PLASMA_END_STATIC_REFLECTED_TYPE;

void operator<<(plStreamWriter& Stream, const plAudioSystemRequestRegisterEntity& Value)
{
  Stream << static_cast<const plAudioSystemRequest&>(Value);
  Stream << Value.m_sName;
}

void operator>>(plStreamReader& Stream, plAudioSystemRequestRegisterEntity& Value)
{
  Stream >> static_cast<plAudioSystemRequest&>(Value);
  Stream >> Value.m_sName;
}

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestSetEntityTransform);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestSetEntityTransform, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestSetEntityTransform>)
PLASMA_END_STATIC_REFLECTED_TYPE;

void operator<<(plStreamWriter& Stream, const plAudioSystemRequestSetEntityTransform& Value)
{
  Stream << static_cast<const plAudioSystemRequest&>(Value);
  Stream << Value.m_Transform.m_vPosition;
  Stream << Value.m_Transform.m_vForward;
  Stream << Value.m_Transform.m_vUp;
  Stream << Value.m_Transform.m_vVelocity;
}

void operator>>(plStreamReader& Stream, plAudioSystemRequestSetEntityTransform& Value)
{
  Stream >> static_cast<plAudioSystemRequest&>(Value);
  Stream >> Value.m_Transform.m_vPosition;
  Stream >> Value.m_Transform.m_vForward;
  Stream >> Value.m_Transform.m_vUp;
  Stream >> Value.m_Transform.m_vVelocity;
}

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestUnregisterEntity);
PLASMA_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestUnregisterEntity);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestUnregisterEntity, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestUnregisterEntity>)
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestRegisterListener);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestRegisterListener, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestRegisterListener>)
PLASMA_END_STATIC_REFLECTED_TYPE;

void operator<<(plStreamWriter& Stream, const plAudioSystemRequestRegisterListener& Value)
{
  Stream << static_cast<const plAudioSystemRequest&>(Value);
  Stream << Value.m_sName;
}

void operator>>(plStreamReader& Stream, plAudioSystemRequestRegisterListener& Value)
{
  Stream >> static_cast<plAudioSystemRequest&>(Value);
  Stream >> Value.m_sName;
}

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestSetListenerTransform);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestSetListenerTransform, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestSetListenerTransform>)
PLASMA_END_STATIC_REFLECTED_TYPE;

void operator<<(plStreamWriter& Stream, const plAudioSystemRequestSetListenerTransform& Value)
{
  Stream << static_cast<const plAudioSystemRequest&>(Value);
  Stream << Value.m_Transform.m_vPosition;
  Stream << Value.m_Transform.m_vForward;
  Stream << Value.m_Transform.m_vUp;
  Stream << Value.m_Transform.m_vVelocity;
}

void operator>>(plStreamReader& Stream, plAudioSystemRequestSetListenerTransform& Value)
{
  Stream >> static_cast<plAudioSystemRequest&>(Value);
  Stream >> Value.m_Transform.m_vPosition;
  Stream >> Value.m_Transform.m_vForward;
  Stream >> Value.m_Transform.m_vUp;
  Stream >> Value.m_Transform.m_vVelocity;
}

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestUnregisterListener);
PLASMA_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestUnregisterListener);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestUnregisterListener, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestUnregisterListener>)
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestLoadTrigger);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestLoadTrigger, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestLoadTrigger>)
PLASMA_END_STATIC_REFLECTED_TYPE;

void operator<<(plStreamWriter& Stream, const plAudioSystemRequestLoadTrigger& Value)
{
  Stream << static_cast<const plAudioSystemRequest&>(Value);
  Stream << Value.m_uiEventId;
}

void operator>>(plStreamReader& Stream, plAudioSystemRequestLoadTrigger& Value)
{
  Stream >> static_cast<plAudioSystemRequest&>(Value);
  Stream >> Value.m_uiEventId;
}

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestActivateTrigger);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestActivateTrigger, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestActivateTrigger>)
PLASMA_END_STATIC_REFLECTED_TYPE;

void operator<<(plStreamWriter& Stream, const plAudioSystemRequestActivateTrigger& Value)
{
  Stream << static_cast<const plAudioSystemRequest&>(Value);
  Stream << Value.m_uiEventId;
}

void operator>>(plStreamReader& Stream, plAudioSystemRequestActivateTrigger& Value)
{
  Stream >> static_cast<plAudioSystemRequest&>(Value);
  Stream >> Value.m_uiEventId;
}

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestStopEvent);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestStopEvent, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestStopEvent>)
PLASMA_END_STATIC_REFLECTED_TYPE;

void operator<<(plStreamWriter& Stream, const plAudioSystemRequestStopEvent& Value)
{
  Stream << static_cast<const plAudioSystemRequest&>(Value);
  Stream << Value.m_uiTriggerId;
}

void operator>>(plStreamReader& Stream, plAudioSystemRequestStopEvent& Value)
{
  Stream >> static_cast<plAudioSystemRequest&>(Value);
  Stream >> Value.m_uiTriggerId;
}

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestSetRtpcValue);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestSetRtpcValue, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestSetRtpcValue>)
PLASMA_END_STATIC_REFLECTED_TYPE;

void operator<<(plStreamWriter& Stream, const plAudioSystemRequestSetRtpcValue& Value)
{
  Stream << static_cast<const plAudioSystemRequest&>(Value);
  Stream << Value.m_fValue;
}

void operator>>(plStreamReader& Stream, plAudioSystemRequestSetRtpcValue& Value)
{
  Stream >> static_cast<plAudioSystemRequest&>(Value);
  Stream >> Value.m_fValue;
}

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestUnloadTrigger);
PLASMA_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestUnloadTrigger);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestUnloadTrigger, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestUnloadTrigger>)
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestSetSwitchState);
PLASMA_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestSetSwitchState);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestSetSwitchState, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestSetSwitchState>)
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestSetEnvironmentAmount);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestSetEnvironmentAmount, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestSetEnvironmentAmount>)
PLASMA_END_STATIC_REFLECTED_TYPE;

void operator<<(plStreamWriter& Stream, const plAudioSystemRequestSetEnvironmentAmount& Value)
{
  Stream << static_cast<const plAudioSystemRequest&>(Value);
  Stream << Value.m_fAmount;
}

void operator>>(plStreamReader& Stream, plAudioSystemRequestSetEnvironmentAmount& Value)
{
  Stream >> static_cast<plAudioSystemRequest&>(Value);
  Stream >> Value.m_fAmount;
}

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestSetObstructionOcclusion);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestSetObstructionOcclusion, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestSetObstructionOcclusion>)
PLASMA_END_STATIC_REFLECTED_TYPE;

void operator<<(plStreamWriter& Stream, const plAudioSystemRequestSetObstructionOcclusion& Value)
{
  Stream << static_cast<const plAudioSystemRequest&>(Value);
  Stream << Value.m_fObstruction;
  Stream << Value.m_fOcclusion;
}

void operator>>(plStreamReader& Stream, plAudioSystemRequestSetObstructionOcclusion& Value)
{
  Stream >> static_cast<plAudioSystemRequest&>(Value);
  Stream >> Value.m_fObstruction;
  Stream >> Value.m_fOcclusion;
}

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestLoadBank);
PLASMA_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestLoadBank);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestLoadBank, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestLoadBank>)
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestUnloadBank);
PLASMA_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestUnloadBank);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestUnloadBank, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestUnloadBank>)

PLASMA_END_STATIC_REFLECTED_TYPE;
PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestShutdown);
PLASMA_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestShutdown);
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestShutdown, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestShutdown>)
PLASMA_END_STATIC_REFLECTED_TYPE;

void operator<<(plStreamWriter& Stream, const plAudioSystemRequest& Value)
{
  Stream << Value.m_eStatus.Succeeded();
  Stream << Value.m_uiEntityId;
  Stream << Value.m_uiListenerId;
  Stream << Value.m_uiObjectId;
}

void operator>>(plStreamReader& Stream, plAudioSystemRequest& Value)
{
  bool succeed;
  Stream >> succeed;
  Stream >> Value.m_uiEntityId;
  Stream >> Value.m_uiListenerId;
  Stream >> Value.m_uiObjectId;

  Value.m_eStatus.m_Result = succeed ? PLASMA_SUCCESS : PLASMA_FAILURE;
}
