#pragma once

#include <Core/World/ComponentManager.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

class plSkeletonPoseComponentManager : public plComponentManager<class plSkeletonPoseComponent, plBlockStorageType::Compact>
{
public:
  using SUPER = plComponentManager<plSkeletonPoseComponent, plBlockStorageType::Compact>;

  plSkeletonPoseComponentManager(plWorld* pWorld)
    : SUPER(pWorld)
  {
  }

  void Update(const plWorldModule::UpdateContext& context);
  void EnqueueUpdate(plComponentHandle hComponent);

private:
  mutable plMutex m_Mutex;
  plDeque<plComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Which pose to apply to an animated mesh.
struct plSkeletonPoseMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    CustomPose, ///< Set a custom pose on the mesh.
    RestPose,   ///< Set the rest pose (bind pose) on the mesh.
    Disabled,   ///< Don't set any pose.
    Default = CustomPose
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plSkeletonPoseMode);

/// \brief Used in conjunction with an plAnimatedMeshComponent to set a specific pose for the animated mesh.
///
/// This component is used to set one, static pose for an animated mesh. The pose is applied once at startup.
/// This can be used to either just pose a mesh in a certain way, or to set a start pose that is then used
/// by other systems, for example a ragdoll component, to generate further poses.
///
/// The component needs to be attached to the same game object where the animated mesh component is attached.
class PL_RENDERERCORE_DLL plSkeletonPoseComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plSkeletonPoseComponent, plComponent, plSkeletonPoseComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plSkeletonPoseComponent

public:
  plSkeletonPoseComponent();
  ~plSkeletonPoseComponent();

  /// \brief Sets the asset GUID or path for the plSkeletonResource to use.
  void SetSkeletonFile(const char* szFile); // [ property ]
  const char* GetSkeletonFile() const;      // [ property ]

  /// \brief Sets the plSkeletonResource to use.
  void SetSkeleton(const plSkeletonResourceHandle& hResource);
  const plSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

  /// \brief Configures which pose to apply to the animated mesh.
  void SetPoseMode(plEnum<plSkeletonPoseMode> mode);
  plEnum<plSkeletonPoseMode> GetPoseMode() const { return m_PoseMode; }

  const plRangeView<const char*, plUInt32> GetBones() const;   // [ property ] (exposed bones)
  void SetBone(const char* szKey, const plVariant& value);     // [ property ] (exposed bones)
  void RemoveBone(const char* szKey);                          // [ property ] (exposed bones)
  bool GetBone(const char* szKey, plVariant& out_value) const; // [ property ] (exposed bones)

  /// \brief Instructs the component to apply the pose to the animated mesh again.
  void ResendPose();

protected:
  void Update();
  void SendRestPose();
  void SendCustomPose();

  float m_fDummy = 0;
  plUInt8 m_uiResendPose = 0;
  plSkeletonResourceHandle m_hSkeleton;
  plArrayMap<plHashedString, plExposedBone> m_Bones; // [ property ]
  plEnum<plSkeletonPoseMode> m_PoseMode;             // [ property ]
};
