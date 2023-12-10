#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>

class plGameObject;
class plAnimGraph;

using plAnimGraphResourceHandle = plTypedResourceHandle<class plAnimGraphResource>;
using plSkeletonResourceHandle = plTypedResourceHandle<class plSkeletonResource>;

PLASMA_DEFINE_AS_POD_TYPE(ozz::math::SimdFloat4);

struct plAnimGraphPinDataBoneWeights
{
  plUInt16 m_uiOwnIndex = 0xFFFF;
  float m_fOverallWeight = 1.0f;
  const plAnimGraphSharedBoneWeights* m_pSharedBoneWeights = nullptr;
};

struct plAnimGraphPinDataLocalTransforms
{
  plUInt16 m_uiOwnIndex = 0xFFFF;
  plAnimPoseGeneratorCommandID m_CommandID;
  const plAnimGraphPinDataBoneWeights* m_pWeights = nullptr;
  float m_fOverallWeight = 1.0f;
  plVec3 m_vRootMotion = plVec3::MakeZero();
  bool m_bUseRootMotion = false;
};

struct plAnimGraphPinDataModelTransforms
{
  plUInt16 m_uiOwnIndex = 0xFFFF;
  plAnimPoseGeneratorCommandID m_CommandID;
  plVec3 m_vRootMotion = plVec3::MakeZero();
  plAngle m_RootRotationX;
  plAngle m_RootRotationY;
  plAngle m_RootRotationZ;
  bool m_bUseRootMotion = false;
};

class PLASMA_RENDERERCORE_DLL plAnimController
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plAnimController);

public:
  plAnimController();
  ~plAnimController();

  void Initialize(const plSkeletonResourceHandle& hSkeleton, plAnimPoseGenerator& ref_poseGenerator, const plSharedPtr<plBlackboard>& pBlackboard = nullptr);

  void Update(plTime diff, plGameObject* pTarget);

  void GetRootMotion(plVec3& ref_vTranslation, plAngle& ref_rotationX, plAngle& ref_rotationY, plAngle& ref_rotationZ) const;

  const plSharedPtr<plBlackboard>& GetBlackboard() { return m_pBlackboard; }

  plAnimPoseGenerator& GetPoseGenerator() { return *m_pPoseGenerator; }

  static plSharedPtr<plAnimGraphSharedBoneWeights> CreateBoneWeights(const char* szUniqueName, const plSkeletonResource& skeleton, plDelegate<void(plAnimGraphSharedBoneWeights&)> fill);

  void SetOutputModelTransform(plAnimGraphPinDataModelTransforms* pModelTransform);
  void SetRootMotion(const plVec3& vTranslation, plAngle rotationX, plAngle rotationY, plAngle rotationZ);

  void AddOutputLocalTransforms(plAnimGraphPinDataLocalTransforms* pLocalTransforms);

  plAnimGraphPinDataBoneWeights* AddPinDataBoneWeights();
  plAnimGraphPinDataLocalTransforms* AddPinDataLocalTransforms();
  plAnimGraphPinDataModelTransforms* AddPinDataModelTransforms();

  void AddAnimGraph(const plAnimGraphResourceHandle& hGraph);
  // TODO void RemoveAnimGraph(const plAnimGraphResource& hGraph);

  struct AnimClipInfo
  {
    plAnimationClipResourceHandle m_hClip;
  };

  const AnimClipInfo& GetAnimationClipInfo(plTempHashedString sClipName) const;

private:
  void GenerateLocalResultProcessors(const plSkeletonResource* pSkeleton);

  plSkeletonResourceHandle m_hSkeleton;
  plAnimGraphPinDataModelTransforms* m_pCurrentModelTransforms = nullptr;

  plVec3 m_vRootMotion = plVec3::MakeZero();
  plAngle m_RootRotationX;
  plAngle m_RootRotationY;
  plAngle m_RootRotationZ;

  plDynamicArray<ozz::math::SimdFloat4, plAlignedAllocatorWrapper> m_BlendMask;

  plAnimPoseGenerator* m_pPoseGenerator = nullptr;
  plSharedPtr<plBlackboard> m_pBlackboard = nullptr;

  plHybridArray<plUInt32, 8> m_CurrentLocalTransformOutputs;

  static plMutex s_SharedDataMutex;
  static plHashTable<plString, plSharedPtr<plAnimGraphSharedBoneWeights>> s_SharedBoneWeights;

  struct GraphInstance
  {
    plAnimGraphResourceHandle m_hAnimGraph;
    plUniquePtr<plAnimGraphInstance> m_pInstance;
  };

  plHybridArray<GraphInstance, 2> m_Instances;

  AnimClipInfo m_InvalidClipInfo;
  plHashTable<plHashedString, AnimClipInfo> m_AnimationClipMapping;

private:
  friend class plAnimGraphTriggerOutputPin;
  friend class plAnimGraphTriggerInputPin;
  friend class plAnimGraphBoneWeightsInputPin;
  friend class plAnimGraphBoneWeightsOutputPin;
  friend class plAnimGraphLocalPoseInputPin;
  friend class plAnimGraphLocalPoseOutputPin;
  friend class plAnimGraphModelPoseInputPin;
  friend class plAnimGraphModelPoseOutputPin;
  friend class plAnimGraphLocalPoseMultiInputPin;
  friend class plAnimGraphNumberInputPin;
  friend class plAnimGraphNumberOutputPin;
  friend class plAnimGraphBoolInputPin;
  friend class plAnimGraphBoolOutputPin;

  plHybridArray<plAnimGraphPinDataBoneWeights, 4> m_PinDataBoneWeights;
  plHybridArray<plAnimGraphPinDataLocalTransforms, 4> m_PinDataLocalTransforms;
  plHybridArray<plAnimGraphPinDataModelTransforms, 2> m_PinDataModelTransforms;
};
