#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class PLASMA_RENDERERCORE_DLL plRootRotationAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRootRotationAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plRootRotationAnimNode

public:
  plRootRotationAnimNode();
  ~plRootRotationAnimNode();

private:
  plAnimGraphNumberInputPin m_InRotateX; // [ property ]
  plAnimGraphNumberInputPin m_InRotateY; // [ property ]
  plAnimGraphNumberInputPin m_InRotateZ; // [ property ]
};
