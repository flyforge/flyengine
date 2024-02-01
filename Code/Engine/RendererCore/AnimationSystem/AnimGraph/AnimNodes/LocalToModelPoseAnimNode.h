#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

//class PL_RENDERERCORE_DLL plLocalToModelPoseAnimNode : public plAnimGraphNode
//{
//  PL_ADD_DYNAMIC_REFLECTION(plLocalToModelPoseAnimNode, plAnimGraphNode);
//
//  //////////////////////////////////////////////////////////////////////////
//  // plAnimGraphNode
//
//protected:
//  virtual plResult SerializeNode(plStreamWriter& stream) const override;
//  virtual plResult DeserializeNode(plStreamReader& stream) override;
//
//  virtual void Step(plAnimGraphExecutor& executor, plAnimGraphInstance& graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;
//
//  //////////////////////////////////////////////////////////////////////////
//  // plLocalToModelPoseAnimNode
//
//public:
//  plLocalToModelPoseAnimNode();
//  ~plLocalToModelPoseAnimNode();
//
//private:
//  plAnimGraphLocalPoseInputPin m_LocalPosePin;  // [ property ]
//  plAnimGraphModelPoseOutputPin m_ModelPosePin; // [ property ]
//};
