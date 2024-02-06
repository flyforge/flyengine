#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioSystemRequests.h>

#include <Foundation/Types/VariantTypeRegistry.h>

PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequest, plNoBase, 1, plRTTIDefaultAllocator<plAudioSystemRequest>)
PL_END_STATIC_REFLECTED_TYPE;

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestRegisterEntity);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestRegisterEntity, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestRegisterEntity>)
PL_END_STATIC_REFLECTED_TYPE;

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

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestSetEntityTransform);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestSetEntityTransform, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestSetEntityTransform>)
PL_END_STATIC_REFLECTED_TYPE;

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

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestUnregisterEntity);
PL_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestUnregisterEntity);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestUnregisterEntity, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestUnregisterEntity>)
PL_END_STATIC_REFLECTED_TYPE;

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestRegisterListener);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestRegisterListener, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestRegisterListener>)
PL_END_STATIC_REFLECTED_TYPE;

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

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestSetListenerTransform);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestSetListenerTransform, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestSetListenerTransform>)
PL_END_STATIC_REFLECTED_TYPE;

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

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestUnregisterListener);
PL_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestUnregisterListener);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestUnregisterListener, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestUnregisterListener>)
PL_END_STATIC_REFLECTED_TYPE;

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestLoadTrigger);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestLoadTrigger, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestLoadTrigger>)
PL_END_STATIC_REFLECTED_TYPE;

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

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestActivateTrigger);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestActivateTrigger, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestActivateTrigger>)
PL_END_STATIC_REFLECTED_TYPE;

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

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestStopEvent);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestStopEvent, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestStopEvent>)
PL_END_STATIC_REFLECTED_TYPE;

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

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestSetRtpcValue);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestSetRtpcValue, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestSetRtpcValue>)
PL_END_STATIC_REFLECTED_TYPE;

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

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestUnloadTrigger);
PL_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestUnloadTrigger);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestUnloadTrigger, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestUnloadTrigger>)
PL_END_STATIC_REFLECTED_TYPE;

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestSetSwitchState);
PL_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestSetSwitchState);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestSetSwitchState, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestSetSwitchState>)
PL_END_STATIC_REFLECTED_TYPE;

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestSetEnvironmentAmount);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestSetEnvironmentAmount, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestSetEnvironmentAmount>)
PL_END_STATIC_REFLECTED_TYPE;

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

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestSetObstructionOcclusion);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestSetObstructionOcclusion, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestSetObstructionOcclusion>)
PL_END_STATIC_REFLECTED_TYPE;

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

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestLoadBank);
PL_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestLoadBank);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestLoadBank, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestLoadBank>)
PL_END_STATIC_REFLECTED_TYPE;

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestUnloadBank);
PL_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestUnloadBank);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestUnloadBank, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestUnloadBank>)
PL_END_STATIC_REFLECTED_TYPE;

PL_DEFINE_CUSTOM_VARIANT_TYPE(plAudioSystemRequestShutdown);
PL_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(plAudioSystemRequestShutdown);
PL_BEGIN_STATIC_REFLECTED_TYPE(plAudioSystemRequestShutdown, plAudioSystemRequest, 1, plRTTIDefaultAllocator<plAudioSystemRequestShutdown>)
PL_END_STATIC_REFLECTED_TYPE;

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

  Value.m_eStatus.m_Result = succeed ? PL_SUCCESS : PL_FAILURE;
}
