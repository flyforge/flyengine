#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Gameplay/HeadBoneComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plHeadBoneComponent, 1, plComponentMode::Dynamic)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("VerticalRotation", m_MaxVerticalRotation)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(80)), new plClampValueAttribute(plAngle::MakeFromDegree(0.0f), plAngle::MakeFromDegree(89.0f))),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(SetVerticalRotation, In, "Radians"),
    PL_SCRIPT_FUNCTION_PROPERTY(ChangeVerticalRotation, In, "Radians"),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plHeadBoneComponent::plHeadBoneComponent() = default;
plHeadBoneComponent::~plHeadBoneComponent() = default;

void plHeadBoneComponent::Update()
{
  m_NewVerticalRotation = plMath::Clamp(m_NewVerticalRotation, -m_MaxVerticalRotation, m_MaxVerticalRotation);

  plQuat qOld, qNew;
  qOld = plQuat::MakeFromAxisAndAngle(plVec3(0, 1, 0), m_CurVerticalRotation);
  qNew = plQuat::MakeFromAxisAndAngle(plVec3(0, 1, 0), m_NewVerticalRotation);

  const plQuat qChange = qNew * qOld.GetInverse();

  const plQuat qFinalNew = qChange * GetOwner()->GetLocalRotation();

  GetOwner()->SetLocalRotation(qFinalNew);

  m_CurVerticalRotation = m_NewVerticalRotation;
}

void plHeadBoneComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  // Version 1
  s << m_MaxVerticalRotation;
  s << m_CurVerticalRotation;
}

void plHeadBoneComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  // Version 1
  s >> m_MaxVerticalRotation;
  s >> m_CurVerticalRotation;
}

void plHeadBoneComponent::SetVerticalRotation(float fRadians)
{
  m_NewVerticalRotation = plAngle::MakeFromRadian(fRadians);
}

void plHeadBoneComponent::ChangeVerticalRotation(float fRadians)
{
  m_NewVerticalRotation += plAngle::MakeFromRadian(fRadians);
}

