#include <Core/CorePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plPhysicsWorldModuleInterface, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_BITFLAGS(plPhysicsShapeType, 1)
  PL_BITFLAGS_CONSTANT(plPhysicsShapeType::Static),
  PL_BITFLAGS_CONSTANT(plPhysicsShapeType::Dynamic),
  PL_BITFLAGS_CONSTANT(plPhysicsShapeType::Query),
  PL_BITFLAGS_CONSTANT(plPhysicsShapeType::Trigger),
  PL_BITFLAGS_CONSTANT(plPhysicsShapeType::Character),
  PL_BITFLAGS_CONSTANT(plPhysicsShapeType::Ragdoll),
  PL_BITFLAGS_CONSTANT(plPhysicsShapeType::Rope),
PL_END_STATIC_REFLECTED_BITFLAGS;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgPhysicsAddImpulse);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgPhysicsAddImpulse, 1, plRTTIDefaultAllocator<plMsgPhysicsAddImpulse>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    PL_MEMBER_PROPERTY("Impulse", m_vImpulse),
    PL_MEMBER_PROPERTY("ObjectFilterID", m_uiObjectFilterID),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgPhysicsAddForce);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgPhysicsAddForce, 1, plRTTIDefaultAllocator<plMsgPhysicsAddForce>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    PL_MEMBER_PROPERTY("Force", m_vForce),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgPhysicsJointBroke);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgPhysicsJointBroke, 1, plRTTIDefaultAllocator<plMsgPhysicsJointBroke>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("JointObject", m_hJointObject)
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE

PL_IMPLEMENT_MESSAGE_TYPE(plMsgObjectGrabbed);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgObjectGrabbed, 1, plRTTIDefaultAllocator<plMsgObjectGrabbed>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("GrabbedBy", m_hGrabbedBy),
    PL_MEMBER_PROPERTY("GotGrabbed", m_bGotGrabbed),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgReleaseObjectGrab);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgReleaseObjectGrab, 1, plRTTIDefaultAllocator<plMsgReleaseObjectGrab>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("GrabbedObjectToRelease", m_hGrabbedObjectToRelease),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_MESSAGE_TYPE(plMsgBuildStaticMesh);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgBuildStaticMesh, 1, plRTTIDefaultAllocator<plMsgBuildStaticMesh>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


PL_STATICLINK_FILE(Core, Core_Interfaces_PhysicsWorldModule);
