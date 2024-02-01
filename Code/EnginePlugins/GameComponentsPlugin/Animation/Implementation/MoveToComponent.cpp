#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Animation/MoveToComponent.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plMoveToComponent, 3, plComponentMode::Dynamic)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning),
    PL_MEMBER_PROPERTY("TranslationSpeed", m_fMaxTranslationSpeed)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_MEMBER_PROPERTY("TranslationAcceleration", m_fTranslationAcceleration),
    PL_MEMBER_PROPERTY("TranslationDeceleration", m_fTranslationDeceleration),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(SetTargetPosition, In, "position"),
  }
  PL_END_FUNCTIONS;
}
PL_END_COMPONENT_TYPE;
// clang-format on

plMoveToComponent::plMoveToComponent() = default;
plMoveToComponent::~plMoveToComponent() = default;

void plMoveToComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_Flags.GetValue();
  s << m_fCurTranslationSpeed;
  s << m_fMaxTranslationSpeed;
  s << m_fTranslationAcceleration;
  s << m_fTranslationDeceleration;
  s << m_vTargetPosition;
}


void plMoveToComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  auto& s = inout_stream.GetStream();

  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_Flags;
  s >> m_fCurTranslationSpeed;
  s >> m_fMaxTranslationSpeed;
  s >> m_fTranslationAcceleration;
  s >> m_fTranslationDeceleration;
  s >> m_vTargetPosition;
}

void plMoveToComponent::SetRunning(bool bRunning)
{
  m_Flags.AddOrRemove(plMoveToComponentFlags::Running, bRunning);
}

bool plMoveToComponent::IsRunning() const
{
  return m_Flags.IsSet(plMoveToComponentFlags::Running);
}

void plMoveToComponent::SetTargetPosition(const plVec3& vPos)
{
  m_vTargetPosition = vPos;
}

static float CalculateNewSpeed(float fRemainingDistance, float fCurSpeed, float fMaxSpeed, float fAcceleration, float fDeceleration, float fTimeStep)
{
  float fMaxAllowedSpeed = fMaxSpeed;

  if (fDeceleration > 0)
  {
    const float fMaxDecelerationTime = (fMaxSpeed / fDeceleration);
    const float fMaxDecelerationDistance = fMaxDecelerationTime * fMaxSpeed;

    if (fRemainingDistance <= fMaxDecelerationDistance)
    {
      fMaxAllowedSpeed = fMaxSpeed * (fRemainingDistance / fMaxDecelerationDistance);
    }
  }

  float fMaxNewSpeed = fMaxSpeed;

  if (fAcceleration > 0)
  {
    fMaxNewSpeed = fCurSpeed + fTimeStep * fAcceleration;
  }

  return plMath::Clamp(fMaxNewSpeed, 0.0f, fMaxAllowedSpeed);
}

void plMoveToComponent::Update()
{
  if (!m_Flags.IsAnySet(plMoveToComponentFlags::Running))
    return;

  plGameObject* pOwner = GetOwner();

  const plVec3 vCurPos = pOwner->GetGlobalPosition();

  plVec3 vDiff = m_vTargetPosition - vCurPos;
  const float fRemainingLength = vDiff.GetLength();

  if (plMath::IsZero(fRemainingLength, 0.002f))
  {
    SetRunning(false);
    pOwner->SetGlobalPosition(m_vTargetPosition);

    plMsgAnimationReachedEnd msg;
    m_ReachedEndMsgSender.SendEventMessage(msg, this, GetOwner());

    return;
  }

  const plVec3 vDir = vDiff / fRemainingLength;

  m_fCurTranslationSpeed = CalculateNewSpeed(fRemainingLength, m_fCurTranslationSpeed, m_fMaxTranslationSpeed, m_fTranslationAcceleration,
    m_fTranslationDeceleration, GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds());

  const float fTravelDist = plMath::Min<float>(fRemainingLength, m_fCurTranslationSpeed * GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds());

  pOwner->SetGlobalPosition(vCurPos + vDir * fTravelDist);
}

