#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class plGameObject;
class plAnimGraph;
class plAnimController;

class PL_RENDERERCORE_DLL plAnimGraphInstance
{
  PL_DISALLOW_COPY_AND_ASSIGN(plAnimGraphInstance);

public:
  plAnimGraphInstance();
  ~plAnimGraphInstance();

  void Configure(const plAnimGraph& animGraph);

  void Update(plAnimController& ref_controller, plTime diff, plGameObject* pTarget, const plSkeletonResource* pSekeltonResource);

  template <typename T>
  T* GetAnimNodeInstanceData(const plAnimGraphNode& node)
  {
    return reinterpret_cast<T*>(plInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), node.m_uiInstanceDataOffset));
  }


private:
  const plAnimGraph* m_pAnimGraph = nullptr;

  plBlob m_InstanceData;

  // EXTEND THIS if a new type is introduced
  plInt8* m_pTriggerInputPinStates = nullptr;
  double* m_pNumberInputPinStates = nullptr;
  bool* m_pBoolInputPinStates = nullptr;
  plUInt16* m_pBoneWeightInputPinStates = nullptr;
  plDynamicArray<plHybridArray<plUInt16, 1>> m_LocalPoseInputPinStates;
  plUInt16* m_pModelPoseInputPinStates = nullptr;

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
};
