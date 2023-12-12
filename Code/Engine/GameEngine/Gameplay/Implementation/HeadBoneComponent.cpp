#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/HeadBoneComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plHeadBoneComponent, 1, plComponentMode::Dynamic)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("VerticalRotation", m_MaxVerticalRotation)->AddAttributes(new plDefaultValueAttribute(plAngle::Degree(80)), new plClampValueAttribute(plAngle::Degree(0.0f), plAngle::Degree(89.0f))),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Transform"),
    new plColorAttribute(plColorScheme::Gameplay),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetVerticalRotation, In, "Radians"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(ChangeVerticalRotation, In, "Radians"),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plHeadBoneComponent::plHeadBoneComponent() = default;
plHeadBoneComponent::~plHeadBoneComponent() = default;

void plHeadBoneComponent::Update()
{
  m_NewVerticalRotation = plMath::Clamp(m_NewVerticalRotation, -m_MaxVerticalRotation, m_MaxVerticalRotation);

  plQuat qOld, qNew;
  qOld.SetFromAxisAndAngle(plVec3(0, 1, 0), m_CurVerticalRotation);
  qNew.SetFromAxisAndAngle(plVec3(0, 1, 0), m_NewVerticalRotation);

  const plQuat qChange = qNew * -qOld;

  const plQuat qFinalNew = qChange * GetOwner()->GetLocalRotation();

  GetOwner()->SetLocalRotation(qFinalNew);

  m_CurVerticalRotation = m_NewVerticalRotation;
}

void plHeadBoneComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  // Version 1
  s << m_MaxVerticalRotation;
  s << m_CurVerticalRotation;
}

void plHeadBoneComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  // Version 1
  s >> m_MaxVerticalRotation;
  s >> m_CurVerticalRotation;
}

void plHeadBoneComponent::SetVerticalRotation(float radians)
{
  m_NewVerticalRotation = plAngle::Radian(radians);
}

void plHeadBoneComponent::ChangeVerticalRotation(float radians)
{
  m_NewVerticalRotation += plAngle::Radian(radians);
}

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_HeadBoneComponent);
