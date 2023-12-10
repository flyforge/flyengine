#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct plMsgPhysicsAddImpulse;
struct plMsgPhysicsAddForce;
namespace JPH
{
  class Constraint;
}

using plSurfaceResourceHandle = plTypedResourceHandle<class plSurfaceResource>;

struct plJoltRopeAnchorConstraintMode
{
  using StorageType = plInt8;

  enum Enum
  {
    None,
    Point,
    Fixed,
    Cone,

    Default = Point
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_JOLTPLUGIN_DLL, plJoltRopeAnchorConstraintMode);

//////////////////////////////////////////////////////////////////////////

class PLASMA_JOLTPLUGIN_DLL plJoltRopeComponentManager : public plComponentManager<class plJoltRopeComponent, plBlockStorageType::Compact>
{
public:
  plJoltRopeComponentManager(plWorld* pWorld);
  ~plJoltRopeComponentManager();

  virtual void Initialize() override;

private:
  void Update(const plWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_JOLTPLUGIN_DLL plJoltRopeComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltRopeComponent, plComponent, plJoltRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltRopeComponent

public:
  plJoltRopeComponent();
  ~plJoltRopeComponent();

  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]
  void SetGravityFactor(float fGravity);                      // [ property ]

  void SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;      // [ property ]

  plUInt8 m_uiCollisionLayer = 0;           // [ property ]
  plUInt16 m_uiPieces = 16;                 // [ property ]
  float m_fThickness = 0.05f;               // [ property ]
  float m_fSlack = 0.3f;                    // [ property ]
  bool m_bCCD = false;                      // [ property ]
  plAngle m_MaxBend = plAngle::MakeFromDegree(30);  // [ property ]
  plAngle m_MaxTwist = plAngle::MakeFromDegree(15); // [ property ]

  void SetAnchor1Reference(const char* szReference); // [ property ]
  void SetAnchor2Reference(const char* szReference); // [ property ]

  void SetAnchor1(plGameObjectHandle hActor);
  void SetAnchor2(plGameObjectHandle hActor);

  void AddForceAtPos(plMsgPhysicsAddForce& ref_msg);
  void AddImpulseAtPos(plMsgPhysicsAddImpulse& ref_msg);

  void SetAnchor1ConstraintMode(plEnum<plJoltRopeAnchorConstraintMode> mode); // [ property ]
  void SetAnchor2ConstraintMode(plEnum<plJoltRopeAnchorConstraintMode> mode); // [ property ]

  plEnum<plJoltRopeAnchorConstraintMode> GetAnchor1ConstraintMode() const { return m_Anchor1ConstraintMode; } // [ property ]
  plEnum<plJoltRopeAnchorConstraintMode> GetAnchor2ConstraintMode() const { return m_Anchor2ConstraintMode; } // [ property ]

  /// \brief Makes sure that the rope's connection to a removed body also gets removed.
  void OnJoltMsgDisconnectConstraints(plJoltMsgDisconnectConstraints& ref_msg); // [ msg handler ]

private:
  void CreateRope();
  plResult CreateSegmentTransforms(plDynamicArray<plTransform>& transforms, float& out_fPieceLength, plGameObjectHandle hAnchor1, plGameObjectHandle hAnchor2);
  void DestroyPhysicsShapes();
  void Update();
  void SendPreviewPose();
  const plJoltMaterial* GetJoltMaterial();
  JPH::Constraint* CreateConstraint(const plGameObjectHandle& hTarget, const plTransform& dstLoc, plUInt32 uiBodyID, plJoltRopeAnchorConstraintMode::Enum mode, plUInt32& out_uiConnectedToBodyID);
  void UpdatePreview();

  plSurfaceResourceHandle m_hSurface;

  plGameObjectHandle m_hAnchor1;
  plGameObjectHandle m_hAnchor2;

  plEnum<plJoltRopeAnchorConstraintMode> m_Anchor1ConstraintMode; // [ property ]
  plEnum<plJoltRopeAnchorConstraintMode> m_Anchor2ConstraintMode; // [ property ]

  float m_fTotalMass = 1.0f;
  float m_fMaxForcePerFrame = 0.0f;
  float m_fBendStiffness = 0.0f;
  plUInt32 m_uiObjectFilterID = plInvalidIndex;
  plUInt32 m_uiUserDataIndex = plInvalidIndex;
  bool m_bSelfCollision = false;
  float m_fGravityFactor = 1.0f;
  plUInt32 m_uiPreviewHash = 0;

  JPH::Ragdoll* m_pRagdoll = nullptr;
  JPH::Constraint* m_pConstraintAnchor1 = nullptr;
  JPH::Constraint* m_pConstraintAnchor2 = nullptr;
  plUInt32 m_uiAnchor1BodyID = plInvalidIndex;
  plUInt32 m_uiAnchor2BodyID = plInvalidIndex;


private:
  const char* DummyGetter() const { return nullptr; }
};
