#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class plSkeletonResource;
class plStreamWriter;
class plStreamReader;

class PL_RENDERERCORE_DLL plBoneWeightsAnimNode : public plAnimGraphNode
{
  PL_ADD_DYNAMIC_REFLECTION(plBoneWeightsAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // plBoneWeightsAnimNode

public:
  plBoneWeightsAnimNode();
  ~plBoneWeightsAnimNode();

  float m_fWeight = 1.0f; // [ property ]

  plUInt32 RootBones_GetCount() const;                          // [ property ]
  const char* RootBones_GetValue(plUInt32 uiIndex) const;       // [ property ]
  void RootBones_SetValue(plUInt32 uiIndex, const char* value); // [ property ]
  void RootBones_Insert(plUInt32 uiIndex, const char* value);   // [ property ]
  void RootBones_Remove(plUInt32 uiIndex);                      // [ property ]

private:
  plAnimGraphBoneWeightsOutputPin m_WeightsPin;        // [ property ]
  plAnimGraphBoneWeightsOutputPin m_InverseWeightsPin; // [ property ]

  plHybridArray<plHashedString, 2> m_RootBones;

  struct InstanceData
  {
    plSharedPtr<plAnimGraphSharedBoneWeights> m_pSharedBoneWeights;
    plSharedPtr<plAnimGraphSharedBoneWeights> m_pSharedInverseBoneWeights;
  };
};
