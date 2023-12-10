#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/RootMotionAnimNodes.h>

// clang-format off
 PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRootRotationAnimNode, 1, plRTTIDefaultAllocator<plRootRotationAnimNode>)
{
   PLASMA_BEGIN_PROPERTIES
   {
     PLASMA_MEMBER_PROPERTY("InRotateX", m_InRotateX)->AddAttributes(new plHiddenAttribute),
     PLASMA_MEMBER_PROPERTY("InRotateY", m_InRotateY)->AddAttributes(new plHiddenAttribute),
     PLASMA_MEMBER_PROPERTY("InRotateZ", m_InRotateZ)->AddAttributes(new plHiddenAttribute),
   }
   PLASMA_END_PROPERTIES;
   PLASMA_BEGIN_ATTRIBUTES
   {
     new plCategoryAttribute("Output"),
     new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Grape)),
     new plTitleAttribute("Root Rotation"),
   }
   PLASMA_END_ATTRIBUTES;
 }
 PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRootRotationAnimNode::plRootRotationAnimNode() = default;
plRootRotationAnimNode::~plRootRotationAnimNode() = default;

plResult plRootRotationAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  PLASMA_SUCCEED_OR_RETURN(m_InRotateX.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InRotateY.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InRotateZ.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plRootRotationAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  PLASMA_SUCCEED_OR_RETURN(m_InRotateX.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InRotateY.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InRotateZ.Deserialize(stream));

  return PLASMA_SUCCESS;
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

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_ModelPoseOutputAnimNode);
