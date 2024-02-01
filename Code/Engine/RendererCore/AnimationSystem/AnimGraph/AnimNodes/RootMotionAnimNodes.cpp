#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/RootMotionAnimNodes.h>

// clang-format off
 PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRootRotationAnimNode, 1, plRTTIDefaultAllocator<plRootRotationAnimNode>)
{
   PL_BEGIN_PROPERTIES
   {
     PL_MEMBER_PROPERTY("InRotateX", m_InRotateX)->AddAttributes(new plHiddenAttribute),
     PL_MEMBER_PROPERTY("InRotateY", m_InRotateY)->AddAttributes(new plHiddenAttribute),
     PL_MEMBER_PROPERTY("InRotateZ", m_InRotateZ)->AddAttributes(new plHiddenAttribute),
   }
   PL_END_PROPERTIES;
   PL_BEGIN_ATTRIBUTES
   {
     new plCategoryAttribute("Output"),
     new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Grape)),
     new plTitleAttribute("Root Rotation"),
   }
   PL_END_ATTRIBUTES;
 }
 PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRootRotationAnimNode::plRootRotationAnimNode() = default;
plRootRotationAnimNode::~plRootRotationAnimNode() = default;

plResult plRootRotationAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  PL_SUCCEED_OR_RETURN(m_InRotateX.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InRotateY.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InRotateZ.Serialize(stream));

  return PL_SUCCESS;
}

plResult plRootRotationAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  PL_SUCCEED_OR_RETURN(m_InRotateX.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InRotateY.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InRotateZ.Deserialize(stream));

  return PL_SUCCESS;
}

void plRootRotationAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  plVec3 vRootMotion = plVec3::MakeZero();
  plAngle rootRotationX;
  plAngle rootRotationY;
  plAngle rootRotationZ;

  ref_controller.GetRootMotion(vRootMotion, rootRotationX, rootRotationY, rootRotationZ);

  if (m_InRotateX.IsConnected())
  {
    rootRotationX += plAngle::MakeFromDegree(static_cast<float>(m_InRotateX.GetNumber(ref_graph)));
  }
  if (m_InRotateY.IsConnected())
  {
    rootRotationY += plAngle::MakeFromDegree(static_cast<float>(m_InRotateY.GetNumber(ref_graph)));
  }
  if (m_InRotateZ.IsConnected())
  {
    rootRotationZ += plAngle::MakeFromDegree(static_cast<float>(m_InRotateZ.GetNumber(ref_graph)));
  }

  ref_controller.SetRootMotion(vRootMotion, rootRotationX, rootRotationY, rootRotationZ);
}

PL_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_RootMotionAnimNodes);
