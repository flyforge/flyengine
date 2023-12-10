#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/LocalToModelPoseAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

//// clang-format off
// PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLocalToModelPoseAnimNode, 1, plRTTIDefaultAllocator<plLocalToModelPoseAnimNode>)
//{
//   PLASMA_BEGIN_PROPERTIES
//   {
//     PLASMA_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new plHiddenAttribute),
//     PLASMA_MEMBER_PROPERTY("ModelPose", m_ModelPosePin)->AddAttributes(new plHiddenAttribute),
//   }
//   PLASMA_END_PROPERTIES;
//   PLASMA_BEGIN_ATTRIBUTES
//   {
//     new plCategoryAttribute("Pose Processing"),
//     new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue)),
//     new plTitleAttribute("Local To Model Space"),
//   }
//   PLASMA_END_ATTRIBUTES;
// }
// PLASMA_END_DYNAMIC_REFLECTED_TYPE;
//// clang-format on
//
// plLocalToModelPoseAnimNode::plLocalToModelPoseAnimNode() = default;
// plLocalToModelPoseAnimNode::~plLocalToModelPoseAnimNode() = default;
//
// plResult plLocalToModelPoseAnimNode::SerializeNode(plStreamWriter& stream) const
//{
//  stream.WriteVersion(1);
//
//  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));
//
//  PLASMA_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
//  PLASMA_SUCCEED_OR_RETURN(m_ModelPosePin.Serialize(stream));
//
//  return PLASMA_SUCCESS;
//}
//
// plResult plLocalToModelPoseAnimNode::DeserializeNode(plStreamReader& stream)
//{
//  stream.ReadVersion(1);
//
//  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));
//
//  PLASMA_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
//  PLASMA_SUCCEED_OR_RETURN(m_ModelPosePin.Deserialize(stream));
//
//  return PLASMA_SUCCESS;
//}
//
// void plLocalToModelPoseAnimNode::Step(plAnimGraphExecutor& executor, plAnimGraphInstance& graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
//{
//  if (!m_LocalPosePin.IsConnected() || !m_ModelPosePin.IsConnected())
//    return;
//
//  auto pLocalPose = m_LocalPosePin.GetPose(graph);
//  if (pLocalPose == nullptr)
//    return;
//
//  plAnimGraphPinDataModelTransforms* pModelTransform = graph.AddPinDataModelTransforms();
//
//  if (pLocalPose->m_bUseRootMotion)
//  {
//    pModelTransform->m_bUseRootMotion = true;
//    pModelTransform->m_vRootMotion = pLocalPose->m_vRootMotion;
//  }
//
//  auto& cmd = graph.GetPoseGenerator().AllocCommandLocalToModelPose();
//  cmd.m_Inputs.PushBack(m_LocalPosePin.GetPose(graph)->m_CommandID);
//
//  pModelTransform->m_CommandID = cmd.GetCommandID();
//
//  m_ModelPosePin.SetPose(graph, pModelTransform);
//}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_LocalToModelPoseAnimNode);
