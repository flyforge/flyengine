#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class PL_RENDERERCORE_DLL plSendEventAnimNode : public plAnimGraphNode
{
  PL_ADD_DYNAMIC_REFLECTION(plSendEventAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plSendEventAnimNode

public:
  void SetEventName(const char* szSz) { m_sEventName.Assign(szSz); }
  const char* GetEventName() const { return m_sEventName.GetString(); }

private:
  plHashedString m_sEventName;             // [ property ]
  plAnimGraphTriggerInputPin m_InActivate; // [ property ]
};
