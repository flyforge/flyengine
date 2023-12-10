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

struct plSkeletonPoseMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    CustomPose,
    RestPose,
    Disabled,
    Default = CustomPose
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plSkeletonPoseMode);


class PLASMA_RENDERERCORE_DLL plSkeletonPoseComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSkeletonPoseComponent, plComponent, plSkeletonPoseComponentManager);

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

  void SetSkeletonFile(const char* szFile); // [ property ]
  const char* GetSkeletonFile() const;      // [ property ]

  void SetSkeleton(const plSkeletonResourceHandle& hResource);
  const plSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

  plEnum<plSkeletonPoseMode> GetPoseMode() const { return m_PoseMode; }
  void SetPoseMode(plEnum<plSkeletonPoseMode> mode);

  void ResendPose();

  const plRangeView<const char*, plUInt32> GetBones() const;   // [ property ] (exposed bones)
  void SetBone(const char* szKey, const plVariant& value);     // [ property ] (exposed bones)
  void RemoveBone(const char* szKey);                          // [ property ] (exposed bones)
  bool GetBone(const char* szKey, plVariant& out_value) const; // [ property ] (exposed bones)

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
