#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>

using plSkeletonResourceHandle = plTypedResourceHandle<class plSkeletonResource>;

class PLASMA_GAMEENGINE_DLL plAnimatedMeshComponentManager : public plComponentManager<class plAnimatedMeshComponent, plBlockStorageType::Compact>
{
public:
  plAnimatedMeshComponentManager(plWorld* pWorld);
  ~plAnimatedMeshComponentManager();

  virtual void Initialize() override;

  void Update(const plWorldModule::UpdateContext& context);
  void AddToUpdateList(plAnimatedMeshComponent* pComponent);

private:
  void ResourceEventHandler(const plResourceEvent& e);

  plDeque<plComponentHandle> m_ComponentsToUpdate;
};

class PLASMA_GAMEENGINE_DLL plAnimatedMeshComponent : public plMeshComponentBase
{
  PLASMA_DECLARE_COMPONENT_TYPE(plAnimatedMeshComponent, plMeshComponentBase, plAnimatedMeshComponentManager);


  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plMeshComponentBase

protected:
  virtual plMeshRenderData* CreateRenderData() const override;
  virtual plResult GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg) override;

  //////////////////////////////////////////////////////////////////////////
  // plAnimatedMeshComponent

public:
  plAnimatedMeshComponent();
  ~plAnimatedMeshComponent();

  void RetrievePose(plDynamicArray<plMat4>& out_modelTransforms, plTransform& out_rootTransform, const plSkeleton& skeleton);

protected:
  void OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& msg);     // [ msg handler ]
  void OnQueryAnimationSkeleton(plMsgQueryAnimationSkeleton& msg); // [ msg handler ]

  void InitializeAnimationPose();

  void MapModelSpacePoseToSkinningSpace(const plHashTable<plHashedString, plMeshResourceDescriptor::BoneData>& bones, const plSkeleton& skeleton, plArrayPtr<const plMat4> modelSpaceTransforms, plBoundingBox* bounds);

  plTransform m_RootTransform = plTransform::IdentityTransform();
  plBoundingBox m_MaxBounds;
  plSkinningState m_SkinningState;
  plSkeletonResourceHandle m_hDefaultSkeleton;
};


struct plRootMotionMode
{
  using StorageType = plInt8;

  enum Enum
  {
    Ignore,
    ApplyToOwner,
    SendMoveCharacterMsg,

    Default = Ignore
  };

  PLASMA_GAMEENGINE_DLL static void Apply(plRootMotionMode::Enum mode, plGameObject* pObject, const plVec3& vTranslation, plAngle rotationX, plAngle rotationY, plAngle rotationZ);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plRootMotionMode);
