#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Effects/Wind/SimpleWindComponent.h>
#include <GameEngine/Effects/Wind/SimpleWindWorldModule.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plSimpleWindComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("MinWindStrength", plWindStrength, m_MinWindStrength),
    PL_ENUM_MEMBER_PROPERTY("MaxWindStrength", plWindStrength, m_MaxWindStrength),
    PL_MEMBER_PROPERTY("MaxDeviation", m_Deviation)->AddAttributes(new plClampValueAttribute(plAngle::MakeFromDegree(0), plAngle::MakeFromDegree(180))),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Effects/Wind"),
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 0.5f, plColor::DodgerBlue),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSimpleWindComponent::plSimpleWindComponent() = default;
plSimpleWindComponent::~plSimpleWindComponent() = default;

void plSimpleWindComponent::Update()
{
  plSimpleWindWorldModule* pWindModule = GetWorld()->GetModule<plSimpleWindWorldModule>();

  if (pWindModule == nullptr)
    return;

  const plTime tCur = GetWorld()->GetClock().GetAccumulatedTime();
  const float fLerp = static_cast<float>((tCur - m_LastChange).GetSeconds() / (m_NextChange - m_LastChange).GetSeconds());

  plVec3 vCurWind;

  if (fLerp >= 1.0f)
  {
    ComputeNextState();

    vCurWind = m_vLastDirection * m_fLastStrength;
  }
  else
  {
    const float fCurStrength = plMath::Lerp(m_fLastStrength, m_fNextStrength, fLerp);
    const plVec3 vCurDir = plMath::Lerp(m_vLastDirection, m_vNextDirection, fLerp);

    vCurWind = vCurDir * fCurStrength;
  }

  pWindModule->SetFallbackWind(vCurWind);
}

void plSimpleWindComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_MinWindStrength;
  s << m_MaxWindStrength;
  s << m_Deviation;
}

void plSimpleWindComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  if (uiVersion == 1)
  {
    float m_fWindStrengthMin, m_fWindStrengthMax;
    s >> m_fWindStrengthMin;
    s >> m_fWindStrengthMax;
  }
  else
  {
    s >> m_MinWindStrength;
    s >> m_MaxWindStrength;
  }

  s >> m_Deviation;
}

void plSimpleWindComponent::OnActivated()
{
  SUPER::OnActivated();

  m_fNextStrength = plWindStrength::GetInMetersPerSecond(m_MinWindStrength);
  m_vNextDirection = GetOwner()->GetGlobalDirForwards();
  m_NextChange = GetWorld()->GetClock().GetAccumulatedTime();
  m_LastChange = m_NextChange - plTime::MakeFromSeconds(1);

  ComputeNextState();
}

void plSimpleWindComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  plSimpleWindWorldModule* pWindModule = GetWorld()->GetModule<plSimpleWindWorldModule>();

  if (pWindModule == nullptr)
    return;

  pWindModule->SetFallbackWind(plVec3::MakeZero());
}

void plSimpleWindComponent::ComputeNextState()
{
  m_fLastStrength = m_fNextStrength;
  m_vLastDirection = m_vNextDirection;
  m_LastChange = GetWorld()->GetClock().GetAccumulatedTime();

  auto& rng = GetWorld()->GetRandomNumberGenerator();

  const plEnum<plWindStrength> minWind = plMath::Min(m_MinWindStrength, m_MaxWindStrength);
  const plEnum<plWindStrength> maxWind = plMath::Max(m_MinWindStrength, m_MaxWindStrength);

  const float fMinStrength = plWindStrength::GetInMetersPerSecond(minWind);
  const float fMaxStrength = plWindStrength::GetInMetersPerSecond(maxWind);

  float fStrengthDiff = fMaxStrength - fMinStrength;
  float fStrengthChange = fStrengthDiff * 0.2f;

  m_NextChange = m_LastChange + plTime::MakeFromSeconds(rng.DoubleMinMax(2.0f, 5.0f));
  m_fNextStrength = plMath::Clamp<float>(m_fLastStrength + (float)rng.DoubleMinMax(-fStrengthChange, +fStrengthChange), fMinStrength, fMaxStrength);

  const plVec3 vMainDir = GetOwner()->GetGlobalDirForwards();

  if (m_Deviation < plAngle::MakeFromDegree(1))
    m_vNextDirection = vMainDir;
  else
    m_vNextDirection = plVec3::MakeRandomDeviation(rng, m_Deviation, vMainDir);

  plCoordinateSystem cs;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), cs);
  const float fRemoveUp = m_vNextDirection.Dot(cs.m_vUpDir);

  m_vNextDirection -= cs.m_vUpDir * fRemoveUp;
  m_vNextDirection.NormalizeIfNotZero(plVec3::MakeZero()).IgnoreResult();
}

void plSimpleWindComponent::Initialize()
{
  SUPER::Initialize();

  // make sure to query the wind interface before any simulation starts
  /*plWindWorldModuleInterface* pWindInterface =*/GetWorld()->GetOrCreateModule<plSimpleWindWorldModule>();
}



PL_STATICLINK_FILE(GameEngine, GameEngine_Effects_Wind_Implementation_SimpleWindComponent);
