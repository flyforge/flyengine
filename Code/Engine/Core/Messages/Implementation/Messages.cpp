#include <Core/CorePCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

// clang-format off
PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgCollision);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgCollision, 1, plRTTIDefaultAllocator<plMsgCollision>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plTriggerState, 1)
  PLASMA_ENUM_CONSTANTS(plTriggerState::Activated, plTriggerState::Continuing, plTriggerState::Deactivated)
PLASMA_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgDeleteGameObject);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgDeleteGameObject, 1, plRTTIDefaultAllocator<plMsgDeleteGameObject>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgComponentInternalTrigger);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgComponentInternalTrigger, 1, plRTTIDefaultAllocator<plMsgComponentInternalTrigger>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Message", m_sMessage),
    PLASMA_MEMBER_PROPERTY("Payload", m_iPayload),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgUpdateLocalBounds);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgUpdateLocalBounds, 1, plRTTIDefaultAllocator<plMsgUpdateLocalBounds>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgSetPlaying);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgSetPlaying, 1, plRTTIDefaultAllocator<plMsgSetPlaying>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Play", m_bPlay)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgParentChanged);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgParentChanged, 1, plRTTIDefaultAllocator<plMsgParentChanged>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgChildrenChanged);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgChildrenChanged, 1, plRTTIDefaultAllocator<plMsgChildrenChanged>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgComponentsChanged);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgComponentsChanged, 1, plRTTIDefaultAllocator<plMsgComponentsChanged>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgTransformChanged);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgTransformChanged, 1, plRTTIDefaultAllocator<plMsgTransformChanged>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgSetFloatParameter);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgSetFloatParameter, 1, plRTTIDefaultAllocator<plMsgSetFloatParameter>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Name", m_sParameterName),
    PLASMA_MEMBER_PROPERTY("Value", m_fValue),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgGenericEvent);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgGenericEvent, 1, plRTTIDefaultAllocator<plMsgGenericEvent>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Message", m_sMessage),
    PLASMA_MEMBER_PROPERTY("Value", m_Value)->AddAttributes(new plDefaultValueAttribute(0))
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgAnimationReachedEnd);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgAnimationReachedEnd, 1, plRTTIDefaultAllocator<plMsgAnimationReachedEnd>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgTriggerTriggered)
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgTriggerTriggered, 1, plRTTIDefaultAllocator<plMsgTriggerTriggered>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Message", m_sMessage),
    PLASMA_ENUM_MEMBER_PROPERTY("TriggerState", plTriggerState, m_TriggerState),
    PLASMA_MEMBER_PROPERTY("GameObject", m_hTriggeringObject),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

// clang-format on

PLASMA_STATICLINK_FILE(Core, Core_Messages_Implementation_Messages);
