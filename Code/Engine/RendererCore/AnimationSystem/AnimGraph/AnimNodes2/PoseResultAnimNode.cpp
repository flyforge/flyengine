#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/PoseResultAnimNode.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plPoseResultAnimNode, 1, plRTTIDefaultAllocator<plPoseResultAnimNode>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("FadeDuration", m_FadeDuration)->AddAttributes(new plDefaultValueAttribute(plTime::MakeFromMilliseconds(200)), new plClampValueAttribute(plTime::MakeZero(), plTime::MakeFromSeconds(10))),
    PL_MEMBER_PROPERTY("InPose", m_InPose)->AddAttributes(new plHiddenAttribute),
    PL_MEMBER_PROPERTY("InTargetWeight", m_InTargetWeight)->AddAttributes(new plHiddenAttribute),
    PL_MEMBER_PROPERTY("InFadeDuration", m_InFadeDuration)->AddAttributes(new plHiddenAttribute),
    PL_MEMBER_PROPERTY("InWeights", m_InWeights)->AddAttributes(new plHiddenAttribute),
    PL_MEMBER_PROPERTY("OutOnFadedOut", m_OutOnFadedOut)->AddAttributes(new plHiddenAttribute),
    PL_MEMBER_PROPERTY("OutOnFadedIn", m_OutOnFadedIn)->AddAttributes(new plHiddenAttribute),
    PL_MEMBER_PROPERTY("OutCurrentWeight", m_OutCurrentWeight)->AddAttributes(new plHiddenAttribute),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Output"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Grape)),
    new plTitleAttribute("Pose Result"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plPoseResultAnimNode::plPoseResultAnimNode() = default;
plPoseResultAnimNode::~plPoseResultAnimNode() = default;

plResult plPoseResultAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_FadeDuration;

  PL_SUCCEED_OR_RETURN(m_InPose.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InTargetWeight.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InFadeDuration.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_InWeights.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnFadedOut.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnFadedIn.Serialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutCurrentWeight.Serialize(stream));

  return PL_SUCCESS;
}

plResult plPoseResultAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_FadeDuration;

  PL_SUCCEED_OR_RETURN(m_InPose.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InTargetWeight.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InFadeDuration.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_InWeights.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnFadedOut.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutOnFadedIn.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(m_OutCurrentWeight.Deserialize(stream));

  return PL_SUCCESS;
}

void plPoseResultAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_InPose.IsConnected())
    return;

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const bool bWasInterpolating = pInstance->m_PlayTime < pInstance->m_EndTime;

  float fCurrentWeight = 1.0f;
  pInstance->m_PlayTime += tDiff;

  if (pInstance->m_PlayTime >= pInstance->m_EndTime)
  {
    fCurrentWeight = pInstance->m_fEndWeight;

    if (bWasInterpolating && fCurrentWeight <= 0.0f)
    {
      m_OutOnFadedOut.SetTriggered(ref_graph);
    }
    if (bWasInterpolating && fCurrentWeight >= 1.0f)
    {
      m_OutOnFadedIn.SetTriggered(ref_graph);
    }
  }
  else
  {
    const float f = (float)(pInstance->m_PlayTime.GetSeconds() / pInstance->m_EndTime.GetSeconds());
    fCurrentWeight = plMath::Lerp(pInstance->m_fStartWeight, pInstance->m_fEndWeight, f);
  }

  const float fNewTargetWeight = m_InTargetWeight.GetNumber(ref_graph, 1.0f);

  if (pInstance->m_fEndWeight != fNewTargetWeight)
  {
    pInstance->m_fStartWeight = fCurrentWeight;
    pInstance->m_fEndWeight = fNewTargetWeight;
    pInstance->m_PlayTime = plTime::MakeZero();
    pInstance->m_EndTime = plTime::MakeFromSeconds(m_InFadeDuration.GetNumber(ref_graph, m_FadeDuration.GetSeconds()));
  }

  m_OutCurrentWeight.SetNumber(ref_graph, fCurrentWeight);

  if (fCurrentWeight <= 0.0f)
    return;

  if (auto pCurrentLocalTransforms = m_InPose.GetPose(ref_controller, ref_graph))
  {
    if (pCurrentLocalTransforms->m_CommandID != plInvalidIndex)
    {
      plAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_CommandID = pCurrentLocalTransforms->m_CommandID;
      pLocalTransforms->m_pWeights = m_InWeights.GetWeights(ref_controller, ref_graph);
      pLocalTransforms->m_fOverallWeight = pCurrentLocalTransforms->m_fOverallWeight * fCurrentWeight;
      pLocalTransforms->m_bUseRootMotion = pCurrentLocalTransforms->m_bUseRootMotion;
      pLocalTransforms->m_vRootMotion = pCurrentLocalTransforms->m_vRootMotion;

      ref_controller.AddOutputLocalTransforms(pLocalTransforms);
    }
  }
  else
  {
    // if we are active, but the incoming pose isn't valid (anymore), use a rest pose as placeholder
    // this assumes that many animations return to the rest pose and if they are played up to the very end before fading out
    // they can be faded out by using the rest pose

    const void* pThis = this;
    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandRestPose();

    {
      plAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_CommandID = cmd.GetCommandID();
      pLocalTransforms->m_pWeights = m_InWeights.GetWeights(ref_controller, ref_graph);
      pLocalTransforms->m_fOverallWeight = fCurrentWeight;
      pLocalTransforms->m_bUseRootMotion = false;

      ref_controller.AddOutputLocalTransforms(pLocalTransforms);
    }
  }
}

bool plPoseResultAnimNode::GetInstanceDataDesc(plInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}


PL_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes2_PoseResultAnimNode);

