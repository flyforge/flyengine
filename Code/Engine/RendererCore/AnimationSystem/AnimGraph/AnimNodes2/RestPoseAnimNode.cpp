#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/RestPoseAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRestPoseAnimNode, 1, plRTTIDefaultAllocator<plRestPoseAnimNode>)
  {
    PLASMA_BEGIN_PROPERTIES
    {
      PLASMA_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new plHiddenAttribute()),
    }
    PLASMA_END_PROPERTIES;
    PLASMA_BEGIN_ATTRIBUTES
    {
      new plCategoryAttribute("Pose Generation"),
      new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue)),
      new plTitleAttribute("Rest Pose"),
    }
    PLASMA_END_ATTRIBUTES;
  }
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plRestPoseAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  PLASMA_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plRestPoseAnimNode::DeserializeNode(plStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  PLASMA_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plRestPoseAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
    return;

  const void* pThis = this;
  auto& cmd = ref_controller.GetPoseGenerator().AllocCommandRestPose();

  {
    plAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

    pLocalTransforms->m_pWeights = nullptr;
    pLocalTransforms->m_bUseRootMotion = false;
    pLocalTransforms->m_fOverallWeight = 1.0f;
    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }
}
