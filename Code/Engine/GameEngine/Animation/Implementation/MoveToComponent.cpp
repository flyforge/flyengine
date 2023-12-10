#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/MoveToComponent.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plMoveToComponent, 3, plComponentMode::Dynamic)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning),
    PLASMA_MEMBER_PROPERTY("TranslationSpeed", m_fMaxTranslationSpeed)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("TranslationAcceleration", m_fTranslationAcceleration),
    PLASMA_MEMBER_PROPERTY("TranslationDeceleration", m_fTranslationDeceleration),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetTargetPosition, In, "position"),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

plMoveToComponent::plMoveToComponent() = default;
plMoveToComponent::~plMoveToComponent() = default;

void plMoveToComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_Flags.GetValue();
  s << m_fCurTranslationSpeed;
  s << m_fMaxTranslationSpeed;
  s << m_fTranslationAcceleration;
  s << m_fTranslationDeceleration;
  s << m_vTargetPosition;
}


void plMoveToComponent::DeserializeComponent(plWorldReader& stream)
{
  auto& s = stream.GetStream();

  SUPER::DeserializeComponent(stream);
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

void plMoveToComponent::SetTargetPosition(const plVec3& pos)
{
  m_vTargetPosition = pos;
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


PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_MoveToComponent);
