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

/// \brief How a rope end gets attached to the anchor point.
struct plJoltRopeAnchorConstraintMode
{
  using StorageType = plInt8;

  enum Enum
  {
    None,  ///< The rope is not attached at this anchor and will fall down here once simulation starts.
    Point, ///< The rope end can rotate freely around the anchor point.
    Fixed, ///< The rope end can neither move nor rotate at the anchor point.
    Cone,  ///< The rope end can rotate up to a maximum angle at the anchor point.

    Default = Point
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_JOLTPLUGIN_DLL, plJoltRopeAnchorConstraintMode);

//////////////////////////////////////////////////////////////////////////

class PL_JOLTPLUGIN_DLL plJoltRopeComponentManager : public plComponentManager<class plJoltRopeComponent, plBlockStorageType::Compact>
{
public:
  plJoltRopeComponentManager(plWorld* pWorld);
  ~plJoltRopeComponentManager();

  virtual void Initialize() override;

private:
  void Update(const plWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

/// \brief Creates a physically simulated rope that is made up of multiple segments.
///
/// The rope will be created between two anchor points. The component requires at least one anchor to be provided,
/// if no second anchor is given, the rope's owner is used as the second.
///
/// If the anchors themselves are physically simulated bodies, the rope will attach to those bodies,
/// making it possible to constrain physics objects with a rope.
class PL_JOLTPLUGIN_DLL plJoltRopeComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plJoltRopeComponent, plComponent, plJoltRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltRopeComponent

public:
  plJoltRopeComponent();
  ~plJoltRopeComponent();

  /// \brief How strongly gravity pulls the rope down.
  void SetGravityFactor(float fGravity);                      // [ property ]
  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]

  /// \brief The plSurfaceResource to be used on the rope physics bodies.
  void SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;      // [ property ]

  /// Defines which other physics objects the rope collides with.
  plUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// Of how many pieces the rope is made up.
  plUInt16 m_uiPieces = 16; // [ property ]

  /// How thick the simulated rope is. This is independent of the rope render thickness.
  float m_fThickness = 0.05f; // [ property ]

  /// How much the rope should sag. A value of 0 means it should be absolutely straight.
  float m_fSlack = 0.3f; // [ property ]

  /// If enabled, a more precise simulation method is used, preventing the rope from tunneling through walls.
  /// This comes at an extra performance cost.
  bool m_bCCD = false; // [ property ]

  /// How much each rope segment may bend.
  plAngle m_MaxBend = plAngle::MakeFromDegree(30); // [ property ]

  /// How much each rope segment may twist.
  plAngle m_MaxTwist = plAngle::MakeFromDegree(15); // [ property ]

  /// \brief Sets the anchor 1 references by object GUID.
  void SetAnchor1Reference(const char* szReference); // [ property ]

  /// \brief Sets the anchor 2 references by object GUID.
  void SetAnchor2Reference(const char* szReference); // [ property ]

  /// \brief Sets the anchor 1 reference.
  void SetAnchor1(plGameObjectHandle hActor);

  /// \brief Sets the anchor 2 reference.
  void SetAnchor2(plGameObjectHandle hActor);

  /// \brief Adds a force (like wind) to the rope.
  void AddForceAtPos(plMsgPhysicsAddForce& ref_msg);

  /// \brief Adds an impulse (like an impact) to the rope.
  void AddImpulseAtPos(plMsgPhysicsAddImpulse& ref_msg);

  /// \brief Configures how the rope is attached at anchor 1.
  void SetAnchor1ConstraintMode(plEnum<plJoltRopeAnchorConstraintMode> mode);                                 // [ property ]
  plEnum<plJoltRopeAnchorConstraintMode> GetAnchor1ConstraintMode() const { return m_Anchor1ConstraintMode; } // [ property ]

  /// \brief Configures how the rope is attached at anchor 2.
  void SetAnchor2ConstraintMode(plEnum<plJoltRopeAnchorConstraintMode> mode);                                 // [ property ]
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
