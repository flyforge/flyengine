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

class PLASMA_JOLTPLUGIN_DLL plJoltRagdollComponentManager : public plComponentManager<class plJoltRagdollComponent, plBlockStorageType::FreeList>
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

struct plJoltRagdollStartMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    WithBindPose,
    WithNextAnimPose,
    WithCurrentMeshPose,
    Default = WithBindPose
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_JOLTPLUGIN_DLL, plJoltRagdollStartMode);

//////////////////////////////////////////////////////////////////////////

class PLASMA_JOLTPLUGIN_DLL plJoltRagdollComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltRagdollComponent, plComponent, plJoltRagdollComponentManager);

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

  plUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; } // [ scriptable ]

  void OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& ref_msg); // [ msg handler ]
  void OnMsgAnimationPoseProposal(plMsgAnimationPoseProposal& ref_msg); // [ msg handler ]
  void OnRetrieveBoneState(plMsgRetrieveBoneState& ref_msg) const; // [ msg handler ]

  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]
  void SetGravityFactor(float fFactor);                       // [ property ]

  bool m_bSelfCollision = false;        // [ property ]
  float m_fStiffnessFactor = 1.0f;     // [ property ]
  float m_fMass = 50.0f;                // [ property ]

  void SetStartMode(plEnum<plJoltRagdollStartMode> mode);                     // [ property ]
  plEnum<plJoltRagdollStartMode> GetStartMode() const { return m_StartMode; } // [ property ]

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
  float m_fOwnerVelocityScale = 1.0f;              // [ property ]
  float m_fCenterVelocity = 0.0f;                  // [ property ]
  float m_fCenterAngularVelocity = 0.0f;           // [ property ]
  plVec3 m_vCenterPosition = plVec3::ZeroVector(); // [ property ]

  void SetJointTypeOverride(plStringView sJointName, plEnum<plSkeletonJointType> type);

protected:
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

  plEnum<plJoltRagdollStartMode> m_StartMode; // [ property ]
  float m_fGravityFactor = 1.0f;              // [ property ]

  plSkeletonResourceHandle m_hSkeleton;
  plDynamicArray<plMat4> m_CurrentLimbTransforms;

  plUInt32 m_uiObjectFilterID = plInvalidIndex;
  plUInt32 m_uiJoltUserDataIndex = plInvalidIndex;
  plJoltUserData* m_pJoltUserData = nullptr;

  JPH::Ragdoll* m_pRagdoll = nullptr;
  JPH::RagdollSettings* m_pRagdollSettings = nullptr;
  plDynamicArray<Limb> m_Limbs;
  plTransform m_RootBodyLocalTransform;
  plTime m_ElapsedTimeSinceUpdate = plTime::Zero();

  plVec3 m_vInitialImpulsePosition = plVec3::ZeroVector();
  plVec3 m_vInitialImpulseDirection = plVec3::ZeroVector();
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
