#pragma once

#include <Foundation/Communication/Message.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>

namespace JPH
{
  class SixDOFConstraint;
}

using plJoltGrabObjectComponentManager = plComponentManagerSimple<class plJoltGrabObjectComponent, plComponentUpdateType::WhenSimulating, plBlockStorageType::Compact>;

/// \brief Used to 'grab' physical objects and attach them to an object. For player objects to pick up objects.
///
/// The component does a raycast along its X axis to detect nearby physics objects. If it finds a non-kinematic plJoltDynamicActor
/// it connects a dedicated object with the picked object through a 6DOF joint, which is set up to drag the picked object towards its
/// position and rotation.
/// The grabbed object can be dropped or thrown away.
///
/// If the picked object has a plGrabbableItemComponent, the custom grab points are used to determine how to grab the object.
class PLASMA_JOLTPLUGIN_DLL plJoltGrabObjectComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltGrabObjectComponent, plComponent, plJoltGrabObjectComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltGrabObjectComponent

public:
  plJoltGrabObjectComponent();
  ~plJoltGrabObjectComponent();

  /// \brief Checks whether there is an object nearby. Note that this function reports static and dynamic objects that are within reach.
  /// Whether these objects are interact able or not is up to the caller.
  bool FindNearbyObject(plGameObject*& out_pObject, plTransform& out_localGrabPoint) const;

  /// \brief Grabs the given object at the given grab point if possible.
  bool GrabObject(plGameObject* pObjectToGrab, const plTransform& localGrabPoint);

  /// \brief Tries to find an object to pick up and do so.
  bool GrabNearbyObject(); // [ scriptable ]

  /// \brief Returns whether an object is currently being held.
  bool HasObjectGrabbed() const; // [ scriptable ]

  /// \brief Returns the grabbed object's actor component.
  plComponentHandle GetGrabbedActor() const { return m_hGrabbedActor; }

  /// \brief Returns the grabbed object's mass.
  float GetGrabbedActorMass() const { return m_fGrabbedActorInverseMass > 0.0f ? 1.0f / m_fGrabbedActorInverseMass : 0.0f; }

  /// \brief The grabbed object is dropped in place.
  void DropGrabbedObject(); // [ scriptable ]

  /// \brief Throws the held object away.
  void ThrowGrabbedObject(const plVec3& vRelativeDir); // [ scriptable ]

  /// \brief Similar to DropGrabbedObject() but additionally posts the event message plMsgPhysicsJointBroke.
  ///
  /// This can be used to inform other code that the object was ripped from the hands of the player.
  void BreakObjectGrab(); // [ scriptable ]

  /// If the held actor is pushed out of the hands farther than this, the 'joint' breaks. Set to zero to disable this feature.
  float m_fBreakDistance = 0.5f; // [ property ]

  /// The stiffness of the joint to pull the object towards the player's hands.
  /// Careful, too large values mean the held object can push objects that the player itself cannot push.
  float m_fSpringStiffness = 50.0f; // [ property ]

  /// The damping of the joint, to prevent oscillation when moving around.
  float m_fSpringDamping = 10.0f; // [ property ]

  /// How far grab points are allowed to be away to pick them
  float m_fMaxGrabPointDistance = 2.0f; // [ property ]

  /// The radius of the sphere cast that is used instead of a raycast if the radius is greater than zero.
  /// This can make it easier to pick up very small objects.
  float m_fCastRadius = 0.0f; // [ property ]

  /// The collision layer to use for the raycast.
  plUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// If non-zero, the player can pick up objects that have no plGrabbableItemComponent, if their bounding box extents are below this value.
  float m_fAllowGrabAnyObjectWithSize = 0.75f; // [ property ]

  void SetAttachToReference(const char* szReference); // [ property ]

  /// Which other game object to attach the grabbed object to.
  /// It is expected to hold a kinematic plJoltDynamicActorComponent that an plJoltJointComponent can be attached to.
  plGameObjectHandle m_hAttachTo;

protected:
  void Update();
  void ReleaseGrabbedObject();

  plJoltDynamicActorComponent* GetAttachToActor();
  plResult DetermineGrabPoint(const plComponent* pActor, plTransform& out_LocalGrabPoint) const;
  void CreateJoint(plJoltDynamicActorComponent* pParent, plJoltDynamicActorComponent* pChild);
  void DetectDistanceViolation(plJoltDynamicActorComponent* pGrabbedActor);
  bool IsCharacterStandingOnObject(plGameObjectHandle hActorToGrab) const;

  void OnMsgReleaseObjectGrab(plMsgReleaseObjectGrab& msg); // [ message handler ]

  plComponentHandle m_hGrabbedActor;
  float m_fGrabbedActorGravity = 1.0f;
  float m_fGrabbedActorInverseMass = 0.0f;

  plTime m_LastValidTime;
  plTransform m_ChildAnchorLocal;
  plComponentHandle m_hCharacterControllerComponent;
  JPH::SixDOFConstraint* m_pConstraint = nullptr;

private:
  const char* DummyGetter() const { return nullptr; }
};
