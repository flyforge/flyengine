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

struct plJoltConstraintLimitMode
{
  using StorageType = plInt8;

  enum Enum
  {
    NoLimit,
    HardLimit,
    // SoftLimit,

    Default = NoLimit
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_JOLTPLUGIN_DLL, plJoltConstraintLimitMode);

struct plJoltConstraintDriveMode
{
  using StorageType = plInt8;

  enum Enum
  {
    NoDrive,
    DriveVelocity,
    DrivePosition,

    Default = NoDrive
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_JOLTPLUGIN_DLL, plJoltConstraintDriveMode);

//////////////////////////////////////////////////////////////////////////

class PLASMA_JOLTPLUGIN_DLL plJoltConstraintComponent : public plComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plJoltConstraintComponent, plComponent);

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

  void SetBreakForce(float value);                      // [ property ]
  float GetBreakForce() const { return m_fBreakForce; } // [ property ]

  void SetBreakTorque(float value);                       // [ property ]
  float GetBreakTorque() const { return m_fBreakTorque; } // [ property ]

  void SetPairCollision(bool value);                         // [ property ]
  bool GetPairCollision() const { return m_bPairCollision; } // [ property ]

  void SetParentActorReference(const char* szReference);      // [ property ]
  void SetChildActorReference(const char* szReference);       // [ property ]
  void SetChildActorAnchorReference(const char* szReference); // [ property ]

  void SetParentActor(plGameObjectHandle hActor);
  void SetChildActor(plGameObjectHandle hActor);
  void SetChildActorAnchor(plGameObjectHandle hActor);

  void SetActors(plGameObjectHandle hActorA, const plTransform& localFrameA, plGameObjectHandle hActorB, const plTransform& localFrameB);

  virtual void ApplySettings() = 0;

  virtual bool ExceededBreakingPoint() = 0;

  /// \brief Forwards to BreakConstraint().
  void OnJoltMsgDisconnectConstraints(plJoltMsgDisconnectConstraints& msg); // [ msg handler ]

protected:
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
