#include <Core/CorePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPhysicsWorldModuleInterface, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_BITFLAGS(plPhysicsShapeType, 1)
  PLASMA_BITFLAGS_CONSTANT(plPhysicsShapeType::Static),
  PLASMA_BITFLAGS_CONSTANT(plPhysicsShapeType::Dynamic),
  PLASMA_BITFLAGS_CONSTANT(plPhysicsShapeType::Query),
  PLASMA_BITFLAGS_CONSTANT(plPhysicsShapeType::Trigger),
  PLASMA_BITFLAGS_CONSTANT(plPhysicsShapeType::Character),
  PLASMA_BITFLAGS_CONSTANT(plPhysicsShapeType::Ragdoll),
  PLASMA_BITFLAGS_CONSTANT(plPhysicsShapeType::Rope),
PLASMA_END_STATIC_REFLECTED_BITFLAGS;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgPhysicsAddImpulse);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgPhysicsAddImpulse, 1, plRTTIDefaultAllocator<plMsgPhysicsAddImpulse>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    PLASMA_MEMBER_PROPERTY("Impulse", m_vImpulse),
    PLASMA_MEMBER_PROPERTY("ObjectFilterID", m_uiObjectFilterID),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgPhysicsAddForce);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgPhysicsAddForce, 1, plRTTIDefaultAllocator<plMsgPhysicsAddForce>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    PLASMA_MEMBER_PROPERTY("Force", m_vForce),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgPhysicsJointBroke);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgPhysicsJointBroke, 1, plRTTIDefaultAllocator<plMsgPhysicsJointBroke>)
{
  PLASMA_BEGIN_PROPERTIES
  {
   PLASMA_MEMBER_PROPERTY("JointObject", m_hJointObject)
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgObjectGrabbed);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgObjectGrabbed, 1, plRTTIDefaultAllocator<plMsgObjectGrabbed>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("GrabbedBy", m_hGrabbedBy),
    PLASMA_MEMBER_PROPERTY("GotGrabbed", m_bGotGrabbed),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgReleaseObjectGrab);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgReleaseObjectGrab, 1, plRTTIDefaultAllocator<plMsgReleaseObjectGrab>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("GrabbedObjectToRelease", m_hGrabbedObjectToRelease),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgBuildStaticMesh);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgBuildStaticMesh, 1, plRTTIDefaultAllocator<plMsgBuildStaticMesh>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


PLASMA_STATICLINK_FILE(Core, Core_Interfaces_PhysicsWorldModule);
