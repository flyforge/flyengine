#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/LocalToModelPoseAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

//// clang-format off
// PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLocalToModelPoseAnimNode, 1, plRTTIDefaultAllocator<plLocalToModelPoseAnimNode>)
//{
//   PL_BEGIN_PROPERTIES
//   {
//     PL_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new plHiddenAttribute),
//     PL_MEMBER_PROPERTY("ModelPose", m_ModelPosePin)->AddAttributes(new plHiddenAttribute),
//   }
//   PL_END_PROPERTIES;
//   PL_BEGIN_ATTRIBUTES
//   {
//     new plCategoryAttribute("Pose Processing"),
//     new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue)),
//     new plTitleAttribute("Local To Model Space"),
//   }
//   PL_END_ATTRIBUTES;
// }
// PL_END_DYNAMIC_REFLECTED_TYPE;
//// clang-format on
//
// plLocalToModelPoseAnimNode::plLocalToModelPoseAnimNode() = default;
// plLocalToModelPoseAnimNode::~plLocalToModelPoseAnimNode() = default;
//
// plResult plLocalToModelPoseAnimNode::SerializeNode(plStreamWriter& stream) const
//{
//  stream.WriteVersion(1);
//
//  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));
//
//  PL_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
//  PL_SUCCEED_OR_RETURN(m_ModelPosePin.Serialize(stream));
//
//  return PL_SUCCESS;
//}
//
// plResult plLocalToModelPoseAnimNode::DeserializeNode(plStreamReader& stream)
//{
//  stream.ReadVersion(1);
//
//  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));
//
//  PL_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
//  PL_SUCCEED_OR_RETURN(m_ModelPosePin.Deserialize(stream));
//
//  return PL_SUCCESS;
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


PL_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_LocalToModelPoseAnimNode);
