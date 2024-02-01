#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/JointAttachmentComponent.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plJointAttachmentComponent, 1, plComponentMode::Dynamic);
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("JointName", GetJointName, SetJointName),
    PL_MEMBER_PROPERTY("PositionOffset", m_vLocalPositionOffset),
    PL_MEMBER_PROPERTY("RotationOffset", m_vLocalRotationOffset),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_ATTRIBUTES
  {
      new plCategoryAttribute("Animation"),
  }
  PL_END_ATTRIBUTES;

  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgAnimationPoseUpdated, OnAnimationPoseUpdated)
  }
  PL_END_MESSAGEHANDLERS;
}
PL_END_COMPONENT_TYPE
// clang-format on

plJointAttachmentComponent::plJointAttachmentComponent() = default;
plJointAttachmentComponent::~plJointAttachmentComponent() = default;

void plJointAttachmentComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sJointToAttachTo;
  s << m_vLocalPositionOffset;
  s << m_vLocalRotationOffset;
}

void plJointAttachmentComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_sJointToAttachTo;
  s >> m_vLocalPositionOffset;
  s >> m_vLocalRotationOffset;

  m_uiJointIndex = plInvalidJointIndex;
}

void plJointAttachmentComponent::SetJointName(const char* szName)
{
  m_sJointToAttachTo.Assign(szName);
  m_uiJointIndex = plInvalidJointIndex;
}

const char* plJointAttachmentComponent::GetJointName() const
{
  return m_sJointToAttachTo.GetData();
}

void plJointAttachmentComponent::OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& msg)
{
  if (m_uiJointIndex == plInvalidJointIndex)
  {
    m_uiJointIndex = msg.m_pSkeleton->FindJointByName(m_sJointToAttachTo);
  }

  if (m_uiJointIndex == plInvalidJointIndex)
    return;

  plMat4 bone;
  plQuat boneRot;

  msg.ComputeFullBoneTransform(m_uiJointIndex, bone, boneRot);

  plGameObject* pOwner = GetOwner();
  pOwner->SetLocalPosition(bone.GetTranslationVector() + bone.TransformDirection(m_vLocalPositionOffset));
  pOwner->SetLocalRotation(boneRot * m_vLocalRotationOffset);
}

PL_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_JointAttachmentComponent);
