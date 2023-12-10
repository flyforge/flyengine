#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/BoneWeightsAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/skeleton_utils.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plBoneWeightsAnimNode, 1, plRTTIDefaultAllocator<plBoneWeightsAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_ARRAY_ACCESSOR_PROPERTY("RootBones", RootBones_GetCount, RootBones_GetValue, RootBones_SetValue, RootBones_Insert, RootBones_Remove),

    PLASMA_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("InverseWeights", m_InverseWeightsPin)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Weights"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Teal)),
    new plTitleAttribute("Bone Weights '{RootBones[0]}' '{RootBones[1]}' '{RootBones[2]}'"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plBoneWeightsAnimNode::plBoneWeightsAnimNode() = default;
plBoneWeightsAnimNode::~plBoneWeightsAnimNode() = default;

plUInt32 plBoneWeightsAnimNode::RootBones_GetCount() const
{
  return m_RootBones.GetCount();
}

const char* plBoneWeightsAnimNode::RootBones_GetValue(plUInt32 uiIndex) const
{
  return m_RootBones[uiIndex].GetString();
}

void plBoneWeightsAnimNode::RootBones_SetValue(plUInt32 uiIndex, const char* value)
{
  m_RootBones[uiIndex].Assign(value);
}

void plBoneWeightsAnimNode::RootBones_Insert(plUInt32 uiIndex, const char* value)
{
  plHashedString tmp;
  tmp.Assign(value);
  m_RootBones.Insert(tmp, uiIndex);
}

void plBoneWeightsAnimNode::RootBones_Remove(plUInt32 uiIndex)
{
  m_RootBones.RemoveAtAndCopy(uiIndex);
}

plResult plBoneWeightsAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  PLASMA_SUCCEED_OR_RETURN(stream.WriteArray(m_RootBones));

  stream << m_fWeight;

  PLASMA_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InverseWeightsPin.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plBoneWeightsAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  PLASMA_SUCCEED_OR_RETURN(stream.ReadArray(m_RootBones));

  stream >> m_fWeight;

  PLASMA_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InverseWeightsPin.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plBoneWeightsAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_WeightsPin.IsConnected() && !m_InverseWeightsPin.IsConnected())
    return;

  if (m_RootBones.IsEmpty())
  {
    plLog::Warning("No root-bones added to bone weight node in animation controller.");
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  if (pInstance->m_pSharedBoneWeights == nullptr && pInstance->m_pSharedInverseBoneWeights == nullptr)
  {
    const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

    plStringBuilder name;
    name.Format("{}", pSkeleton->GetResourceIDHash());

    for (const auto& rootBone : m_RootBones)
    {
      name.AppendFormat("-{}", rootBone);
    }

    pInstance->m_pSharedBoneWeights = ref_controller.CreateBoneWeights(name, *pSkeleton, [this, pOzzSkeleton](plAnimGraphSharedBoneWeights& ref_bw) {
      for (const auto& rootBone : m_RootBones)
      {
        int iRootBone = -1;
        for (int iBone = 0; iBone < pOzzSkeleton->num_joints(); ++iBone)
        {
          if (plStringUtils::IsEqual(pOzzSkeleton->joint_names()[iBone], rootBone.GetData()))
          {
            iRootBone = iBone;
            break;
          }
        }

        const float fBoneWeight = 1.0f;

        auto setBoneWeight = [&](int iCurrentBone, int) {
          const int iJointIdx0 = iCurrentBone / 4;
          const int iJointIdx1 = iCurrentBone % 4;

          ozz::math::SimdFloat4& soa_weight = ref_bw.m_Weights[iJointIdx0];
          soa_weight = ozz::math::SetI(soa_weight, ozz::math::simd_float4::Load1(fBoneWeight), iJointIdx1);
        };

        ozz::animation::IterateJointsDF(*pOzzSkeleton, setBoneWeight, iRootBone);
      } });

    if (m_InverseWeightsPin.IsConnected())
    {
      name.Append("-inv");

      pInstance->m_pSharedInverseBoneWeights = ref_controller.CreateBoneWeights(name, *pSkeleton, [this, pInstance](plAnimGraphSharedBoneWeights& ref_bw) {
        const ozz::math::SimdFloat4 oneBone = ozz::math::simd_float4::one();

        for (plUInt32 b = 0; b < ref_bw.m_Weights.GetCount(); ++b)
        {
          ref_bw.m_Weights[b] = ozz::math::MSub(oneBone, oneBone, pInstance->m_pSharedBoneWeights->m_Weights[b]);
        } });
    }

    if (!m_WeightsPin.IsConnected())
    {
      pInstance->m_pSharedBoneWeights.Clear();
    }
  }

  if (m_WeightsPin.IsConnected())
  {
    plAnimGraphPinDataBoneWeights* pPinData = ref_controller.AddPinDataBoneWeights();
    pPinData->m_fOverallWeight = m_fWeight;
    pPinData->m_pSharedBoneWeights = pInstance->m_pSharedBoneWeights.Borrow();

    m_WeightsPin.SetWeights(ref_graph, pPinData);
  }

  if (m_InverseWeightsPin.IsConnected())
  {
    plAnimGraphPinDataBoneWeights* pPinData = ref_controller.AddPinDataBoneWeights();
    pPinData->m_fOverallWeight = m_fWeight;
    pPinData->m_pSharedBoneWeights = pInstance->m_pSharedInverseBoneWeights.Borrow();

    m_InverseWeightsPin.SetWeights(ref_graph, pPinData);
  }
}

bool plBoneWeightsAnimNode::GetInstanceDataDesc(plInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_BoneWeightsAnimNode);
