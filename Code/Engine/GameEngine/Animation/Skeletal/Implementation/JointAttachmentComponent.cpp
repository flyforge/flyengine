#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/JointAttachmentComponent.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJointAttachmentComponent, 1, plComponentMode::Dynamic);
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("JointName", GetJointName, SetJointName),
    PLASMA_MEMBER_PROPERTY("PositionOffset", m_vLocalPositionOffset),
    PLASMA_MEMBER_PROPERTY("RotationOffset", m_vLocalRotationOffset),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation"),
  }
  PLASMA_END_ATTRIBUTES;

  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgAnimationPoseUpdated, OnAnimationPoseUpdated)
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plJointAttachmentComponent::plJointAttachmentComponent() = default;
plJointAttachmentComponent::~plJointAttachmentComponent() = default;

void plJointAttachmentComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_sJointToAttachTo;
  s << m_vLocalPositionOffset;
  s << m_vLocalRotationOffset;
}

void plJointAttachmentComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

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

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_JointAttachmentComponent);
