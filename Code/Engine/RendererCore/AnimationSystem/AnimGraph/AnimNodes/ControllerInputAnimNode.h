#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class PL_RENDERERCORE_DLL plControllerInputAnimNode : public plAnimGraphNode
{
  PL_ADD_DYNAMIC_REFLECTION(plControllerInputAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plControllerInputAnimNode

private:
  plAnimGraphNumberOutputPin m_OutLeftStickX;  // [ property ]
  plAnimGraphNumberOutputPin m_OutLeftStickY;  // [ property ]
  plAnimGraphNumberOutputPin m_OutRightStickX; // [ property ]
  plAnimGraphNumberOutputPin m_OutRightStickY; // [ property ]

  plAnimGraphNumberOutputPin m_OutLeftTrigger;  // [ property ]
  plAnimGraphNumberOutputPin m_OutRightTrigger; // [ property ]

  plAnimGraphBoolOutputPin m_OutButtonA; // [ property ]
  plAnimGraphBoolOutputPin m_OutButtonB; // [ property ]
  plAnimGraphBoolOutputPin m_OutButtonX; // [ property ]
  plAnimGraphBoolOutputPin m_OutButtonY; // [ property ]

  plAnimGraphBoolOutputPin m_OutLeftShoulder;  // [ property ]
  plAnimGraphBoolOutputPin m_OutRightShoulder; // [ property ]

  plAnimGraphBoolOutputPin m_OutPadLeft;  // [ property ]
  plAnimGraphBoolOutputPin m_OutPadRight; // [ property ]
  plAnimGraphBoolOutputPin m_OutPadUp;    // [ property ]
  plAnimGraphBoolOutputPin m_OutPadDown;  // [ property ]
};
