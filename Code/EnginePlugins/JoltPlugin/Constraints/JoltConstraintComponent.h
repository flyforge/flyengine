#pragma once

#include <Core/World/ComponentManager.h>
#include <JoltPlugin/Declarations.h>

class plJoltDynamicActorComponent;

namespace JPH
{
  class Body;
}

namespace JPH
{
  class Constraint;
}

/// \brief Configures how a physics constraint's limit acts.
struct plJoltConstraintLimitMode
{
  using StorageType = plInt8;

  enum Enum
  {
    NoLimit,   ///< The constraint has no limit.
    HardLimit, ///< The constraint has a hard limit, no soft spring is used to prevent further movement.
    // SoftLimit,

    Default = NoLimit
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_JOLTPLUGIN_DLL, plJoltConstraintLimitMode);

/// \brief Configures how a drive on a constraint works.
struct plJoltConstraintDriveMode
{
  using StorageType = plInt8;

  enum Enum
  {
    NoDrive,       ///< The constraint has no drive.
    DriveVelocity, ///< The drive attempts to reach a target velocity (of rotation or motion).
    DrivePosition, ///< The drive attempts to reach a target position (or angle).

    Default = NoDrive
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_JOLTPLUGIN_DLL, plJoltConstraintDriveMode);

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for all Jolt physics joints (constraints).
///
/// A constraint always limits the movement of a dynamic actor.
/// The actor may be joined to another actor. If the other actor is static or kinematic,
/// the dynamic actor is restricted to follow those actors.
/// If the other actor is also dynamic, both actors' freedom of movement is restricted.
///
/// A constraint can also only affect a single dynamic actor, in which case it is joined "to the world",
/// which is an imaginary static actor.
///
/// Derived types implement different constraints.
///
/// All constraints can be breakable, meaning that they get disabled when a sufficiently large force or torque
/// acts on the constraint.
/// By default two joined actors still collide with each other, but it is often convenient to disable
/// collisions between only those two actors. For example a door that is joined to a door frame may not work
/// right, if the edge of the door still collides with the door frame. It is easier to make the door work smoothly
/// if it doesn't collide with the frame and its movement is mainly limited by the constraint.
class PL_JOLTPLUGIN_DLL plJoltConstraintComponent : public plComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plJoltConstraintComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltConstraintComponent

public:
  plJoltConstraintComponent();
  ~plJoltConstraintComponent();

  /// \brief Removes the connection between the joined bodies. This cannot be reversed.
  void BreakConstraint();

  /// \brief If set to larger than zero, the constraint will break when a linear force larger than this acts onto the constraint.
  void SetBreakForce(float value);                      // [ property ]
  float GetBreakForce() const { return m_fBreakForce; } // [ property ]

  /// \brief If set to larger than zero, the constraint will break when a rotational force larger than this acts onto the constraint.
  void SetBreakTorque(float value);                       // [ property ]
  float GetBreakTorque() const { return m_fBreakTorque; } // [ property ]

  /// \brief If disabled, the two joined actors pass through each other, rather than colliding.
  void SetPairCollision(bool value);                         // [ property ]
  bool GetPairCollision() const { return m_bPairCollision; } // [ property ]

  void SetParentActorReference(const char* szReference);      // [ property ]
  void SetChildActorReference(const char* szReference);       // [ property ]
  void SetChildActorAnchorReference(const char* szReference); // [ property ]

  /// \brief Sets which actor to attach the constraint to.
  void SetParentActor(plGameObjectHandle hActor);
  /// \brief Sets which actor to attach to the constraint.
  void SetChildActor(plGameObjectHandle hActor);
  /// \brief Sets an actor as a reference frame so that the constraint can start in a non-default configuration.
  void SetChildActorAnchor(plGameObjectHandle hActor);

  /// \brief For manually providing actors and local frames to configure the start state. This is for advanced uses.
  void SetActors(plGameObjectHandle hActorA, const plTransform& localFrameA, plGameObjectHandle hActorB, const plTransform& localFrameB);

  /// \brief Forwards to BreakConstraint().
  void OnJoltMsgDisconnectConstraints(plJoltMsgDisconnectConstraints& ref_msg); // [ msg handler ]

protected:
  friend class plJoltWorldModule;

  virtual bool ExceededBreakingPoint() = 0;
  virtual void ApplySettings() = 0;

  plResult FindParentBody(plUInt32& out_uiJoltBodyID, plJoltDynamicActorComponent*& pRbComp);
  plResult FindChildBody(plUInt32& out_uiJoltBodyID, plJoltDynamicActorComponent*& pRbComp);

  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) = 0;

  plTransform ComputeParentBodyGlobalFrame() const;
  plTransform ComputeChildBodyGlobalFrame() const;

  void QueueApplySettings();

  plGameObjectHandle m_hActorA;
  plGameObjectHandle m_hActorB;
  plGameObjectHandle m_hActorBAnchor;

  // UserFlag0 specifies whether m_localFrameA is already set
  plTransform m_LocalFrameA;
  // UserFlag1 specifies whether m_localFrameB is already set
  plTransform m_LocalFrameB;

  JPH::Constraint* m_pConstraint = nullptr;

  float m_fBreakForce = 0.0f;
  float m_fBreakTorque = 0.0f;
  bool m_bPairCollision = true;

private:
  const char* DummyGetter() const { return nullptr; }
};
