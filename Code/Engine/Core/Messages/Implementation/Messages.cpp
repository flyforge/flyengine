#include <Core/CorePCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

// clang-format off
PL_IMPLEMENT_MESSAGE_TYPE(plMsgCollision);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgCollision, 1, plRTTIDefaultAllocator<plMsgCollision>)
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_STATIC_REFLECTED_ENUM(plTriggerState, 1)
  PL_ENUM_CONSTANTS(plTriggerState::Activated, plTriggerState::Continuing, plTriggerState::Deactivated)
PL_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_MESSAGE_TYPE(plMsgDeleteGameObject);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgDeleteGameObject, 1, plRTTIDefaultAllocator<plMsgDeleteGameObject>)
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_MESSAGE_TYPE(plMsgComponentInternalTrigger);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgComponentInternalTrigger, 1, plRTTIDefaultAllocator<plMsgComponentInternalTrigger>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Message", m_sMessage),
    PL_MEMBER_PROPERTY("Payload", m_iPayload),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_MESSAGE_TYPE(plMsgUpdateLocalBounds);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgUpdateLocalBounds, 1, plRTTIDefaultAllocator<plMsgUpdateLocalBounds>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_MESSAGE_TYPE(plMsgSetPlaying);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgSetPlaying, 1, plRTTIDefaultAllocator<plMsgSetPlaying>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Play", m_bPlay)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_MESSAGE_TYPE(plMsgParentChanged);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgParentChanged, 1, plRTTIDefaultAllocator<plMsgParentChanged>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_MESSAGE_TYPE(plMsgChildrenChanged);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgChildrenChanged, 1, plRTTIDefaultAllocator<plMsgChildrenChanged>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_MESSAGE_TYPE(plMsgComponentsChanged);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgComponentsChanged, 1, plRTTIDefaultAllocator<plMsgComponentsChanged>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_MESSAGE_TYPE(plMsgTransformChanged);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgTransformChanged, 1, plRTTIDefaultAllocator<plMsgTransformChanged>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_MESSAGE_TYPE(plMsgSetFloatParameter);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgSetFloatParameter, 1, plRTTIDefaultAllocator<plMsgSetFloatParameter>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sParameterName),
    PL_MEMBER_PROPERTY("Value", m_fValue),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_MESSAGE_TYPE(plMsgGenericEvent);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgGenericEvent, 1, plRTTIDefaultAllocator<plMsgGenericEvent>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Message", m_sMessage),
    PL_MEMBER_PROPERTY("Value", m_Value)->AddAttributes(new plDefaultValueAttribute(0))
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_MESSAGE_TYPE(plMsgAnimationReachedEnd);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAnimationReachedEnd, 1, plRTTIDefaultAllocator<plMsgAnimationReachedEnd>)
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_MESSAGE_TYPE(plMsgTriggerTriggered)
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgTriggerTriggered, 1, plRTTIDefaultAllocator<plMsgTriggerTriggered>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Message", m_sMessage),
    PL_ENUM_MEMBER_PROPERTY("TriggerState", plTriggerState, m_TriggerState),
    PL_MEMBER_PROPERTY("GameObject", m_hTriggeringObject),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

// clang-format on

PL_STATICLINK_FILE(Core, Core_Messages_Implementation_Messages);
