#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/ResetTransformComponent.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plResetTransformComponent, 1, plComponentMode::Dynamic)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ResetPositionX", m_bResetLocalPositionX)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("ResetPositionY", m_bResetLocalPositionY)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("ResetPositionZ", m_bResetLocalPositionZ)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("LocalPosition", m_vLocalPosition),
    PL_MEMBER_PROPERTY("ResetRotation", m_bResetLocalRotation)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("LocalRotation", m_qLocalRotation)->AddAttributes(new plDefaultValueAttribute(plQuat::MakeIdentity())),
    PL_MEMBER_PROPERTY("ResetScaling", m_bResetLocalScaling)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("LocalScaling", m_vLocalScaling)->AddAttributes(new plDefaultValueAttribute(plVec3(1))),
    PL_MEMBER_PROPERTY("LocalUniformScaling", m_fLocalUniformScaling)->AddAttributes(new plDefaultValueAttribute(1)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResetTransformComponent::plResetTransformComponent() = default;
plResetTransformComponent::~plResetTransformComponent() = default;

void plResetTransformComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  plVec3 vLocalPos = GetOwner()->GetLocalPosition();

  if (m_bResetLocalPositionX)
    vLocalPos.x = m_vLocalPosition.x;
  if (m_bResetLocalPositionY)
    vLocalPos.y = m_vLocalPosition.y;
  if (m_bResetLocalPositionZ)
    vLocalPos.z = m_vLocalPosition.z;

  GetOwner()->SetLocalPosition(vLocalPos);

  if (m_bResetLocalRotation)
  {
    GetOwner()->SetLocalRotation(m_qLocalRotation);
  }

  if (m_bResetLocalScaling)
  {
    GetOwner()->SetLocalScaling(m_vLocalScaling);
    GetOwner()->SetLocalUniformScaling(m_fLocalUniformScaling);
  }

  // update the global transform right away
  GetOwner()->UpdateGlobalTransform();
}

void plResetTransformComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_vLocalPosition;
  s << m_qLocalRotation;
  s << m_vLocalScaling;
  s << m_bResetLocalPositionX;
  s << m_bResetLocalPositionY;
  s << m_bResetLocalPositionZ;
  s << m_bResetLocalRotation;
  s << m_bResetLocalScaling;
  s << m_fLocalUniformScaling;
}

void plResetTransformComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_vLocalPosition;
  s >> m_qLocalRotation;
  s >> m_vLocalScaling;
  s >> m_bResetLocalPositionX;
  s >> m_bResetLocalPositionY;
  s >> m_bResetLocalPositionZ;
  s >> m_bResetLocalRotation;
  s >> m_bResetLocalScaling;
  s >> m_fLocalUniformScaling;
}


PL_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_ResetTransformComponent);

