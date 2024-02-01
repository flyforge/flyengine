//#pragma once
//
//#include <GameEngine/GameEngineDLL.h>
//#include <RendererCore/AnimationSystem/AnimationGraph/AnimationClipSampler.h>
//#include <RendererCore/AnimationSystem/AnimationPose.h>
//#include <RendererCore/Meshes/SkinnedMeshComponent.h>
//
//using plAnimationClipResourceHandle = plTypedResourceHandle<class plAnimationClipResource>;
//using plSkeletonResourceHandle = plTypedResourceHandle<class plSkeletonResource>;
//
//using plMotionMatchingComponentManager = plComponentManagerSimple<class plMotionMatchingComponent, plComponentUpdateType::WhenSimulating> ;
//
//class PL_GAMEENGINE_DLL plMotionMatchingComponent : public plSkinnedMeshComponent
//{
//  PL_DECLARE_COMPONENT_TYPE(plMotionMatchingComponent, plSkinnedMeshComponent, plMotionMatchingComponentManager);
//
//  //////////////////////////////////////////////////////////////////////////
//  // plComponent
//
//public:
//  virtual void SerializeComponent(plWorldWriter& stream) const override;
//  virtual void DeserializeComponent(plWorldReader& stream) override;
//
//protected:
//  virtual void OnSimulationStarted() override;
//
//
//  //////////////////////////////////////////////////////////////////////////
//  // plMotionMatchingComponent
//
//public:
//  plMotionMatchingComponent();
//  ~plMotionMatchingComponent();
//
//  void SetAnimation(plUInt32 uiIndex, const plAnimationClipResourceHandle& hResource);
//  plAnimationClipResourceHandle GetAnimation(plUInt32 uiIndex) const;
//
//protected:
//  void Update();
//
//  plUInt32 Animations_GetCount() const;                          // [ property ]
//  const char* Animations_GetValue(plUInt32 uiIndex) const;       // [ property ]
//  void Animations_SetValue(plUInt32 uiIndex, const char* value); // [ property ]
//  void Animations_Insert(plUInt32 uiIndex, const char* value);   // [ property ]
//  void Animations_Remove(plUInt32 uiIndex);                      // [ property ]
//
//  void ConfigureInput();
//  plVec3 GetInputDirection() const;
//  plQuat GetInputRotation() const;
//
//  plAnimationPose m_AnimationPose;
//  plSkeletonResourceHandle m_hSkeleton;
//
//  plDynamicArray<plAnimationClipResourceHandle> m_Animations;
//
//  plVec3 m_vLeftFootPos;
//  plVec3 m_vRightFootPos;
//
//  struct MotionData
//  {
//    plUInt16 m_uiAnimClipIndex;
//    plUInt16 m_uiKeyframeIndex;
//    plVec3 m_vLeftFootPosition;
//    plVec3 m_vLeftFootVelocity;
//    plVec3 m_vRightFootPosition;
//    plVec3 m_vRightFootVelocity;
//    plVec3 m_vRootVelocity;
//  };
//
//  struct TargetKeyframe
//  {
//    plUInt16 m_uiAnimClip;
//    plUInt16 m_uiKeyframe;
//  };
//
//  TargetKeyframe m_Keyframe0;
//  TargetKeyframe m_Keyframe1;
//  float m_fKeyframeLerp = 0.0f;
//
//  TargetKeyframe FindNextKeyframe(const TargetKeyframe& current, const plVec3& vTargetDir) const;
//
//  plDynamicArray<MotionData> m_MotionData;
//
//  static void PrecomputeMotion(plDynamicArray<MotionData>& motionData, plTempHashedString jointName1, plTempHashedString jointName2,
//    const plAnimationClipResourceDescriptor& animClip, plUInt16 uiAnimClipIndex, const plSkeleton& skeleton);
//
//  plUInt32 FindBestKeyframe(const TargetKeyframe& current, plVec3 vLeftFootPosition, plVec3 vRightFootPosition, plVec3 vTargetDir) const;
//};
