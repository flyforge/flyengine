#pragma once

#include <JoltPlugin/Actors/JoltActorComponent.h>

//////////////////////////////////////////////////////////////////////////

class PLASMA_JOLTPLUGIN_DLL plJoltDynamicActorComponentManager : public plComponentManager<class plJoltDynamicActorComponent, plBlockStorageType::FreeList>
{
public:
  plJoltDynamicActorComponentManager(plWorld* pWorld);
  ~plJoltDynamicActorComponentManager();

private:
  friend class plJoltWorldModule;
  friend class plJoltDynamicActorComponent;

  void UpdateKinematicActors(plTime deltaTime);
  void UpdateDynamicActors();

  plDynamicArray<plJoltDynamicActorComponent*> m_KinematicActorComponents;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_JOLTPLUGIN_DLL plJoltDynamicActorComponent : public plJoltActorComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltDynamicActorComponent, plJoltActorComponent, plJoltDynamicActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltDynamicActorComponent

public:
  plJoltDynamicActorComponent();
  ~plJoltDynamicActorComponent();

  plUInt32 GetJoltBodyID() const { return m_uiJoltBodyID; }

  void AddImpulseAtPos(plMsgPhysicsAddImpulse& ref_msg); // [ message ]
  void AddForceAtPos(plMsgPhysicsAddForce& ref_msg);     // [ message ]

  bool GetKinematic() const { return m_bKinematic; } // [ property ]
  void SetKinematic(bool b);                         // [ property ]

  void SetGravityFactor(float fFactor);                       // [ property ]
  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]

  void SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;      // [ property ]

  bool m_bCCD = false;                                    // [ property ]
  bool m_bStartAsleep = false;                            // [ property ]
  float m_fMass = 0.0f;                                   // [ property ]
  float m_fDensity = 1.0f;                                // [ property ]
  float m_fLinearDamping = 0.1f;                          // [ property ]
  float m_fAngularDamping = 0.05f;                        // [ property ]
  plSurfaceResourceHandle m_hSurface;                     // [ property ]
  plBitflags<plOnJoltContact> m_OnContact;                // [ property ]
  plVec3 m_vCenterOfMass = plVec3::ZeroVector();          // [ property ]
  bool GetUseCustomCoM() const { return GetUserFlag(0); } // [ property ]
  void SetUseCustomCoM(bool b) { SetUserFlag(0, b); }     // [ property ]

  void AddLinearForce(const plVec3& vForce);      // [ scriptable ]
  void AddLinearImpulse(const plVec3& vImpulse);  // [ scriptable ]
  void AddAngularForce(const plVec3& vForce);     // [ scriptable ]
  void AddAngularImpulse(const plVec3& vImpulse); // [ scriptable ]

  /// \brief Should be called by components that add Jolt constraints to this body.
  ///
  /// All registered components receive plJoltMsgDisconnectConstraints in case the body is deleted.
  /// It is necessary to react to that by removing the Jolt constraint, otherwise Jolt will crash during the next update.
  void AddConstraint(plComponentHandle hComponent);

  /// \brief Should be called when a constraint is removed (though not strictly required) to prevent unnecessary message sending.
  void RemoveConstraint(plComponentHandle hComponent);

protected:
  const plJoltMaterial* GetJoltMaterial() const;

  bool m_bKinematic = false;
  float m_fGravityFactor = 1.0f; // [ property ]

  plDynamicArray<plComponentHandle> m_Constraints;
};
