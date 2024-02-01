#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Math/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>

class plJoltUserData;
class plSkeletonJoint;
class plJoltWorldModule;
class plJoltMaterial;
struct plMsgRetrieveBoneState;
struct plMsgAnimationPoseUpdated;
struct plMsgPhysicsAddImpulse;
struct plMsgPhysicsAddForce;
struct plSkeletonResourceGeometry;

namespace JPH
{
  class Ragdoll;
  class RagdollSettings;
  class Shape;
} // namespace JPH

using plSkeletonResourceHandle = plTypedResourceHandle<class plSkeletonResource>;
using plSurfaceResourceHandle = plTypedResourceHandle<class plSurfaceResource>;

class PL_JOLTPLUGIN_DLL plJoltRagdollComponentManager : public plComponentManager<class plJoltRagdollComponent, plBlockStorageType::FreeList>
{
public:
  plJoltRagdollComponentManager(plWorld* pWorld);
  ~plJoltRagdollComponentManager();

  virtual void Initialize() override;

private:
  friend class plJoltWorldModule;
  friend class plJoltRagdollComponent;

  void Update(const plWorldModule::UpdateContext& context);
};

/// \brief With which pose a ragdoll should start.
struct plJoltRagdollStartMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    WithBindPose,        ///< The ragdoll uses the bind pose (or rest pose) to start with.
    WithNextAnimPose,    ///< The ragdoll waits for a new pose and then starts simulating with that.
    WithCurrentMeshPose, ///< The ragdoll retrieves the current pose from the animated mesh and starts simulating with that.
    Default = WithBindPose
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_JOLTPLUGIN_DLL, plJoltRagdollStartMode);

//////////////////////////////////////////////////////////////////////////

/// \brief Creates a physics ragdoll for an animated mesh and creates animation poses from the physics simulation.
///
/// By activating this component on an animated mesh, the component creates the necessary physics shapes to simulate a falling body.
/// The component queries the bone transforms from the physics engine and sends plMsgAnimationPoseUpdated with new poses.
///
/// Once this component is active on an animated mesh, no other pose generating component should be active anymore, otherwise
/// multiple components generate conflicting animation poses.
/// The typical way to use this, is to have the component created and configured, but in an inactive state. Once an NPC dies, the component is activated and all other components that generate animation poses should be deactivated.
///
/// The ragdoll shapes are configured through the plSkeletonResource.
///
/// Ragdolls are also used to create fake "breakable" objects. This is achieved by building a skinned object out of several pieces
/// and giving every piece a bone that has no constraint (joint), so that the object just breaks apart.
class PL_JOLTPLUGIN_DLL plJoltRagdollComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plJoltRagdollComponent, plComponent, plJoltRagdollComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltRagdollComponent

public:
  plJoltRagdollComponent();
  ~plJoltRagdollComponent();

  /// \brief Returns the object ID used for all bodies in the ragdoll. This can be used to ignore the entire ragdoll during raycasts and such.
  plUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; } // [ scriptable ]

  /// \brief Adjusts how strongly gravity affects the ragdoll.
  ///
  /// Set this to zero to fully disable gravity.
  void SetGravityFactor(float fFactor);                       // [ property ]
  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]

  /// \brief If true, the ragdoll pieces collide with each other.
  /// This produces more realistic ragdoll behavior, but requires that the ragdoll shapes are well set up.
  /// It may produce jittering and also cost more performance.
  /// Another option is to set up the joint limits such that the ragdoll can't easily intersect itself.
  bool m_bSelfCollision = false; // [ property ]

  /// \brief How easily the joints move. Note that scaling the ragdoll up or down affects the forces and thus stiffness needs to be adjusted as well.
  float m_fStiffnessFactor = 1.0f; // [ property ]

  /// \brief The total weight of the ragdoll. It is distributed across the individual shapes.
  float m_fMass = 50.0f; // [ property ]

  /// \brief Sets with which pose the ragdoll should start simulating.
  void SetStartMode(plEnum<plJoltRagdollStartMode> mode);                     // [ property ]
  plEnum<plJoltRagdollStartMode> GetStartMode() const { return m_StartMode; } // [ property ]

  /// \brief Applies a force to a specific part of the ragdoll.
  void OnMsgPhysicsAddImpulse(plMsgPhysicsAddImpulse& ref_msg); // [ msg handler ]

  /// \brief Applies an impulse to a specific part of the ragdoll.
  ///
  /// If this is called before the ragdoll becomes active, it is added to the 'initial impulse' (see SetInitialImpulse()).
  /// Once the ragdoll is activated, this initial impulse is applied to the closest body part.
  void OnMsgPhysicsAddForce(plMsgPhysicsAddForce& ref_msg); // [ msg handler ]

  /// \brief Call this function BEFORE activating the ragdoll component to specify an impulse that shall be applied to the closest body part when it activates.
  ///
  /// Both position and direction are given in world space.
  ///
  /// This overrides any previously set or accumulated impulses.
  /// If AFTER this call additional impulses are recorded through OnMsgPhysicsAddImpulse(), they are 'added' to the initial impulse.
  ///
  /// Only a single initial impulse is applied after the ragdoll is created.
  /// If multiple impulses are added through OnMsgPhysicsAddImpulse(), their average start position is used to determine the closest body part to apply the impulse on.
  /// Their impulses are accumulated, so the applied impulse can become quite large.
  void SetInitialImpulse(const plVec3& vPosition, const plVec3& vDirectionAndStrength); // [ scriptable ]

  /// \brief Adds to the existing initial impulse. See SetInitialImpulse().
  void AddInitialImpulse(const plVec3& vPosition, const plVec3& vDirectionAndStrength); // [ scriptable ]

  /// \brief How much of the owner object's velocity to transfer to the new ragdoll bodies.
  float m_fOwnerVelocityScale = 1.0f; // [ property ]

  /// If non-zero, when the ragdoll starts, all pieces start out with an outward velocity with this speed.
  ///
  /// This can be used to create breakable objects that should "break apart". Once the ragdoll starts, the bodies will fly
  /// away outward from their center (plus m_vCenterPosition).
  float m_fCenterVelocity = 0.0f; // [ property ]

  /// Similar to m_fCenterVelocity but sets a rotational velocity, so that objects also spin.
  float m_fCenterAngularVelocity = 0.0f; // [ property ]

  /// If center velocity is used, this adds an offset to the object's position to define where the center position should be.
  plVec3 m_vCenterPosition = plVec3::MakeZero(); // [ property ]

  /// \brief Allows to override the type of joint to be used for a bone.
  ///
  /// This has to be called before the component is activated.
  /// Its intended use case is to make certain joints either stiff (by setting the joints to 'fixed'),
  /// or to break pieces off (by setting the type to 'None').
  ///
  /// For example a breakable object could be made up of 10 pieces, but when breaking it, a random number of joints
  /// can be set to 'fixed' or 'none', so that the exact shape of the broken pieces has more variety.
  ///
  /// Similarly, on a animated mesh that is specifically authored to have separable pieces (like an arm on a robot),
  /// one can separate limbs by setting their joint to 'none'.
  void SetJointTypeOverride(plStringView sJointName, plEnum<plSkeletonJointType> type);

  void OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& ref_msg); // [ msg handler ]
  void OnRetrieveBoneState(plMsgRetrieveBoneState& ref_msg) const; // [ msg handler ]

protected:
  plEnum<plJoltRagdollStartMode> m_StartMode; // [ property ]
  float m_fGravityFactor = 1.0f;              // [ property ]

  struct Limb
  {
    plUInt16 m_uiPartIndex = plInvalidJointIndex;
  };

  struct LimbConstructionInfo
  {
    plTransform m_GlobalTransform;
    plUInt16 m_uiJoltPartIndex = plInvalidJointIndex;
  };

  void Update(bool bForce);
  plResult EnsureSkeletonIsKnown();
  void CreateLimbsFromBindPose();
  void CreateLimbsFromCurrentMeshPose();
  void DestroyAllLimbs();
  void CreateLimbsFromPose(const plMsgAnimationPoseUpdated& pose);
  bool HasCreatedLimbs() const;
  plTransform GetRagdollRootTransform() const;
  void UpdateOwnerPosition();
  void RetrieveRagdollPose();
  void SendAnimationPoseMsg();
  void ConfigureRagdollPart(void* pRagdollSettingsPart, const plTransform& globalTransform, plUInt8 uiCollisionLayer, plJoltWorldModule& worldModule);
  void CreateAllLimbs(const plSkeletonResource& skeletonResource, const plMsgAnimationPoseUpdated& pose, plJoltWorldModule& worldModule, float fObjectScale);
  void ComputeLimbModelSpaceTransform(plTransform& transform, const plMsgAnimationPoseUpdated& pose, plUInt32 uiPoseJointIndex);
  void ComputeLimbGlobalTransform(plTransform& transform, const plMsgAnimationPoseUpdated& pose, plUInt32 uiPoseJointIndex);
  void CreateLimb(const plSkeletonResource& skeletonResource, plMap<plUInt16, LimbConstructionInfo>& limbConstructionInfos, plArrayPtr<const plSkeletonResourceGeometry*> geometries, const plMsgAnimationPoseUpdated& pose, plJoltWorldModule& worldModule, float fObjectScale);
  JPH::Shape* CreateLimbGeoShape(const LimbConstructionInfo& limbConstructionInfo, const plSkeletonResourceGeometry& geo, const plJoltMaterial* pJoltMaterial, const plQuat& qBoneDirAdjustment, const plTransform& skeletonRootTransform, plTransform& out_shapeTransform, float fObjectScale);
  void CreateAllLimbGeoShapes(const LimbConstructionInfo& limbConstructionInfo, plArrayPtr<const plSkeletonResourceGeometry*> geometries, const plSkeletonJoint& thisLimbJoint, const plSkeletonResource& skeletonResource, float fObjectScale);
  virtual void ApplyPartInitialVelocity();
  void ApplyBodyMass();
  void ApplyInitialImpulse(plJoltWorldModule& worldModule, float fMaxImpulse);

  plSkeletonResourceHandle m_hSkeleton;
  plDynamicArray<plMat4> m_CurrentLimbTransforms;

  plUInt32 m_uiObjectFilterID = plInvalidIndex;
  plUInt32 m_uiJoltUserDataIndex = plInvalidIndex;
  plJoltUserData* m_pJoltUserData = nullptr;

  JPH::Ragdoll* m_pRagdoll = nullptr;
  JPH::RagdollSettings* m_pRagdollSettings = nullptr;
  plDynamicArray<Limb> m_Limbs;
  plTransform m_RootBodyLocalTransform;
  plTime m_ElapsedTimeSinceUpdate = plTime::MakeZero();

  plVec3 m_vInitialImpulsePosition = plVec3::MakeZero();
  plVec3 m_vInitialImpulseDirection = plVec3::MakeZero();
  plUInt8 m_uiNumInitialImpulses = 0;

  struct JointOverride
  {
    plTempHashedString m_sJointName;
    bool m_bOverrideType = false;
    plEnum<plSkeletonJointType> m_JointType;
  };

  plDynamicArray<JointOverride> m_JointOverrides;

  //////////////////////////////////////////////////////////////////////////

  void SetupLimbJoints(const plSkeletonResource* pSkeleton);
  void CreateLimbJoint(const plSkeletonJoint& thisJoint, void* pParentBodyDesc, void* pThisBodyDesc);
};
