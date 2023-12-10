#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/JointOverrideComponent.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJointOverrideComponent, 1, plComponentMode::Dynamic);
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("JointName", GetJointName, SetJointName),
    PLASMA_MEMBER_PROPERTY("OverridePosition", m_bOverridePosition)->AddAttributes(new plDefaultValueAttribute(false)),
    PLASMA_MEMBER_PROPERTY("OverrideRotation", m_bOverrideRotation)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("OverrideScale", m_bOverrideScale)->AddAttributes(new plDefaultValueAttribute(false)),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation"),
  }
  PLASMA_END_ATTRIBUTES;

  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgAnimationPosePreparing, OnAnimationPosePreparing)
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plJointOverrideComponent::plJointOverrideComponent() = default;
plJointOverrideComponent::~plJointOverrideComponent() = default;

void plJointOverrideComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_sJointToOverride;
  s << m_bOverridePosition;
  s << m_bOverrideRotation;
  s << m_bOverrideScale;
}

void plJointOverrideComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_sJointToOverride;
  s >> m_bOverridePosition;
  s >> m_bOverrideRotation;
  s >> m_bOverrideScale;

  m_uiJointIndex = plInvalidJointIndex;
}

void plJointOverrideComponent::SetJointName(const char* szName)
{
  m_sJointToOverride.Assign(szName);
  m_uiJointIndex = plInvalidJointIndex;
}

const char* plJointOverrideComponent::GetJointName() const
{
  return m_sJointToOverride.GetData();
}

void plJointOverrideComponent::OnAnimationPosePreparing(plMsgAnimationPosePreparing& msg) const
{
  using namespace ozz::math;

  if (m_uiJointIndex == plInvalidJointIndex)
  {
    m_uiJointIndex = msg.m_pSkeleton->FindJointByName(m_sJointToOverride);
  }

  if (m_uiJointIndex == plInvalidJointIndex)
    return;

  const int soaIdx = m_uiJointIndex / 4;
  const int soaSubIdx = m_uiJointIndex % 4;

  const plTransform t = GetOwner()->GetLocalTransform();

  if (m_bOverridePosition)
  {
    SimdFloat4 vx = ozz::math::simd_float4::Load1(t.m_vPosition.x);
    SimdFloat4 vy = ozz::math::simd_float4::Load1(t.m_vPosition.y);
    SimdFloat4 vz = ozz::math::simd_float4::Load1(t.m_vPosition.z);

    auto val = msg.m_LocalTransforms[soaIdx].translation;

    val.x = ozz::math::SetI(val.x, vx, soaSubIdx);
    val.y = ozz::math::SetI(val.y, vy, soaSubIdx);
    val.z = ozz::math::SetI(val.z, vz, soaSubIdx);

    msg.m_LocalTransforms[soaIdx].translation = val;
  }

  if (m_bOverrideRotation)
  {
    SimdFloat4 vx = ozz::math::simd_float4::Load1(t.m_qRotation.x);
    SimdFloat4 vy = ozz::math::simd_float4::Load1(t.m_qRotation.y);
    SimdFloat4 vz = ozz::math::simd_float4::Load1(t.m_qRotation.z);
    SimdFloat4 vw = ozz::math::simd_float4::Load1(t.m_qRotation.w);

    SoaQuaternion val = msg.m_LocalTransforms[soaIdx].rotation;

    val.x = ozz::math::SetI(val.x, vx, soaSubIdx);
    val.y = ozz::math::SetI(val.y, vy, soaSubIdx);
    val.z = ozz::math::SetI(val.z, vz, soaSubIdx);
    val.w = ozz::math::SetI(val.w, vw, soaSubIdx);

    msg.m_LocalTransforms[soaIdx].rotation = val;
  }

  if (m_bOverrideScale)
  {
    SimdFloat4 vx = ozz::math::simd_float4::Load1(t.m_vScale.x);
    SimdFloat4 vy = ozz::math::simd_float4::Load1(t.m_vScale.y);
    SimdFloat4 vz = ozz::math::simd_float4::Load1(t.m_vScale.z);

    auto val = msg.m_LocalTransforms[soaIdx].scale;

    val.x = ozz::math::SetI(val.x, vx, soaSubIdx);
    val.y = ozz::math::SetI(val.y, vy, soaSubIdx);
    val.z = ozz::math::SetI(val.z, vz, soaSubIdx);

    msg.m_LocalTransforms[soaIdx].scale = val;
  }
}
