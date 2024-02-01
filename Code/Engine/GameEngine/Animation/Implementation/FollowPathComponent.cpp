#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameEngine/Animation/FollowPathComponent.h>
#include <GameEngine/Animation/PathComponent.h>

#include <RendererCore/Debug/DebugRenderer.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plFollowPathMode, 1)
  PL_ENUM_CONSTANTS(plFollowPathMode::OnlyPosition, plFollowPathMode::AlignUpZ, plFollowPathMode::FullRotation)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_COMPONENT_TYPE(plFollowPathComponent, 1, plComponentMode::Dynamic)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Path", DummyGetter, SetPathObject)->AddAttributes(new plGameObjectReferenceAttribute()),
    PL_ACCESSOR_PROPERTY("StartDistance", GetDistanceAlongPath, SetDistanceAlongPath)->AddAttributes(new plClampValueAttribute(0.0f, {})),
    PL_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning)->AddAttributes(new plDefaultValueAttribute(true)), // Whether the animation should start right away.
    PL_ENUM_MEMBER_PROPERTY("Mode", plPropertyAnimMode, m_Mode),
    PL_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_MEMBER_PROPERTY("LookAhead", m_fLookAhead)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 10.0f)),
    PL_MEMBER_PROPERTY("Smoothing", m_fSmoothing)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 1.0f)),
    PL_ENUM_MEMBER_PROPERTY("FollowMode", plFollowPathMode, m_FollowMode),
    PL_MEMBER_PROPERTY("TiltAmount", m_fTiltAmount)->AddAttributes(new plDefaultValueAttribute(5.0f)),
    PL_MEMBER_PROPERTY("MaxTilt", m_MaxTilt)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(30.0f)), new plClampValueAttribute(plAngle::MakeFromDegree(0.0f), plAngle::MakeFromDegree(90.0f))),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(SetDirectionForwards, In, "Forwards"),
    PL_SCRIPT_FUNCTION_PROPERTY(IsDirectionForwards),
    PL_SCRIPT_FUNCTION_PROPERTY(ToggleDirection),
  }
  PL_END_FUNCTIONS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation/Paths"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plFollowPathComponent::plFollowPathComponent() = default;
plFollowPathComponent::~plFollowPathComponent() = default;

void plFollowPathComponent::Update(bool bForce)
{
  if (!bForce && (!m_bIsRunning || m_fSpeed == 0.0f))
    return;

  if (m_hPathObject.IsInvalidated())
    return;

  plWorld* pWorld = GetWorld();

  plGameObject* pPathObject = nullptr;
  if (!pWorld->TryGetObject(m_hPathObject, pPathObject))
  {
    // no need to retry this again
    m_hPathObject.Invalidate();
    return;
  }

  plPathComponent* pPathComponent;
  if (!pPathObject->TryGetComponentOfBaseType(pPathComponent))
    return;

  pPathComponent->EnsureLinearizedRepresentationIsUpToDate();

  auto& clock = pWorld->GetClock();

  float fToAdvance = m_fSpeed * clock.GetTimeDiff().AsFloatInSeconds();

  if (!m_bIsRunningForwards)
  {
    fToAdvance = -fToAdvance;
  }

  {
    if (!pPathComponent->AdvanceLinearSamplerBy(m_PathSampler, fToAdvance) && fToAdvance != 0.0f)
    {
      plMsgAnimationReachedEnd msg;
      m_ReachedEndEvent.SendEventMessage(msg, this, GetOwner());

      if (m_Mode == plPropertyAnimMode::Loop)
      {
        pPathComponent->SetLinearSamplerTo(m_PathSampler, fToAdvance);
      }
      else if (m_Mode == plPropertyAnimMode::BackAndForth)
      {
        m_bIsRunningForwards = !m_bIsRunningForwards;
        fToAdvance = -fToAdvance;
        pPathComponent->AdvanceLinearSamplerBy(m_PathSampler, fToAdvance);
      }
      else
      {
        m_bIsRunning = false;
      }
    }
  }

  plPathComponent::LinearSampler samplerAhead;

  float fLookAhead = plMath::Max(m_fLookAhead, 0.02f);

  {
    samplerAhead = m_PathSampler;
    if (!pPathComponent->AdvanceLinearSamplerBy(samplerAhead, fLookAhead) && fLookAhead != 0.0f)
    {
      if (m_Mode == plPropertyAnimMode::Loop)
      {
        pPathComponent->SetLinearSamplerTo(samplerAhead, fLookAhead);
      }
    }
  }

  auto transform = pPathComponent->SampleLinearizedRepresentation(m_PathSampler);
  auto transformAhead = pPathComponent->SampleLinearizedRepresentation(samplerAhead);

  if (m_bLastStateValid)
  {
    const float fSmoothing = plMath::Clamp(m_fSmoothing, 0.0f, 0.99f);

    transform.m_vPosition = plMath::Lerp(transform.m_vPosition, m_vLastPosition, fSmoothing);
    transform.m_vUpDirection = plMath::Lerp(transform.m_vUpDirection, m_vLastUpDir, fSmoothing);
    transformAhead.m_vPosition = plMath::Lerp(transformAhead.m_vPosition, m_vLastTargetPosition, fSmoothing);
  }

  plVec3 vTarget = transformAhead.m_vPosition - transform.m_vPosition;
  if (m_FollowMode == plFollowPathMode::AlignUpZ)
  {
    const plPlane plane = plPlane::MakeFromNormalAndPoint(plVec3::MakeAxisZ(), transform.m_vPosition);
    vTarget = plane.GetCoplanarDirection(vTarget);
  }
  vTarget.NormalizeIfNotZero(plVec3::MakeAxisX()).IgnoreResult();

  plVec3 vUp = (m_FollowMode == plFollowPathMode::FullRotation) ? transform.m_vUpDirection : plVec3::MakeAxisZ();
  plVec3 vRight = vTarget.CrossRH(vUp);
  vRight.NormalizeIfNotZero(plVec3::MakeAxisY()).IgnoreResult();

  vUp = vRight.CrossRH(vTarget);
  vUp.NormalizeIfNotZero(plVec3::MakeAxisZ()).IgnoreResult();

  // check if we want to tilt the platform when turning
  plAngle deltaAngle = plAngle::MakeFromDegree(0.0f);
  if (m_FollowMode == plFollowPathMode::AlignUpZ && !plMath::IsZero(m_fTiltAmount, 0.0001f) && !plMath::IsZero(m_MaxTilt.GetDegree(), 0.0001f))
  {
    if (m_bLastStateValid)
    {
      plVec3 vLastTarget = m_vLastTargetPosition - m_vLastPosition;
      {
        const plPlane plane = plPlane::MakeFromNormalAndPoint(plVec3::MakeAxisZ(), transform.m_vPosition);
        vLastTarget = plane.GetCoplanarDirection(vLastTarget);
        vLastTarget.NormalizeIfNotZero(plVec3::MakeAxisX()).IgnoreResult();
      }

      const float fTiltStrength = plMath::Sign((vTarget - vLastTarget).Dot(vRight)) * plMath::Sign(m_fTiltAmount);
      plAngle tiltAngle = plMath::Min(vLastTarget.GetAngleBetween(vTarget) * plMath::Abs(m_fTiltAmount), m_MaxTilt);
      deltaAngle = plMath::Lerp(tiltAngle * fTiltStrength, m_LastTiltAngle, 0.85f); // this smooths out the tilting from being jittery

      plQuat rot = plQuat::MakeFromAxisAndAngle(vTarget, deltaAngle);
      vUp = rot * vUp;
      vRight = rot * vRight;
    }
  }

  {
    m_bLastStateValid = true;
    m_vLastPosition = transform.m_vPosition;
    m_vLastUpDir = transform.m_vUpDirection;
    m_vLastTargetPosition = transformAhead.m_vPosition;
    m_LastTiltAngle = deltaAngle;
  }

  plMat3 mRot = plMat3::MakeIdentity();
  if (m_FollowMode != plFollowPathMode::OnlyPosition)
  {
    mRot.SetColumn(0, vTarget);
    mRot.SetColumn(1, -vRight);
    mRot.SetColumn(2, vUp);
  }

  plTransform tFinal;
  tFinal.m_vPosition = transform.m_vPosition;
  tFinal.m_vScale.Set(1);
  tFinal.m_qRotation = plQuat::MakeFromMat3(mRot);

  GetOwner()->SetGlobalTransform(pPathObject->GetGlobalTransform() * tFinal);
}

void plFollowPathComponent::SetPathObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hPathObject = resolver(szReference, GetHandle(), "Path");
}

void plFollowPathComponent::SetDistanceAlongPath(float fDistance)
{
  m_bLastStateValid = false;
  m_fStartDistance = fDistance;

  if (IsActiveAndInitialized())
  {
    if (m_hPathObject.IsInvalidated())
      return;

    plWorld* pWorld = GetWorld();

    plGameObject* pPathObject = nullptr;
    if (!pWorld->TryGetObject(m_hPathObject, pPathObject))
      return;

    plPathComponent* pPathComponent = nullptr;
    if (!pPathObject->TryGetComponentOfBaseType(pPathComponent))
      return;

    pPathComponent->EnsureLinearizedRepresentationIsUpToDate();

    pPathComponent->SetLinearSamplerTo(m_PathSampler, m_fStartDistance);

    Update(true);
  }
}

float plFollowPathComponent::GetDistanceAlongPath() const
{
  return m_fStartDistance;
}

void plFollowPathComponent::SerializeComponent(plWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();

  ref_stream.WriteGameObjectHandle(m_hPathObject);

  s << m_fStartDistance;
  s << m_fSpeed;
  s << m_fLookAhead;
  s << m_Mode;
  s << m_fSmoothing;
  s << m_bIsRunning;
  s << m_bIsRunningForwards;
  s << m_FollowMode;
  s << m_fTiltAmount;
  s << m_MaxTilt;
}

void plFollowPathComponent::DeserializeComponent(plWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();

  m_hPathObject = ref_stream.ReadGameObjectHandle();

  s >> m_fStartDistance;
  s >> m_fSpeed;
  s >> m_fLookAhead;
  s >> m_Mode;
  s >> m_fSmoothing;
  s >> m_bIsRunning;
  s >> m_bIsRunningForwards;
  s >> m_FollowMode;
  s >> m_fTiltAmount;
  s >> m_MaxTilt;
}

void plFollowPathComponent::OnActivated()
{
  SUPER::OnActivated();

  // initialize sampler
  SetDistanceAlongPath(m_fStartDistance);
}

void plFollowPathComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // if no path reference was set, search the parent objects for a path
  if (m_hPathObject.IsInvalidated())
  {
    plGameObject* pParent = GetOwner()->GetParent();
    while (pParent != nullptr)
    {
      plPathComponent* pPath = nullptr;
      if (pParent->TryGetComponentOfBaseType(pPath))
      {
        m_hPathObject = pPath->GetOwner()->GetHandle();
        break;
      }

      pParent = pParent->GetParent();
    }
  }

  // initialize sampler
  SetDistanceAlongPath(m_fStartDistance);
}

bool plFollowPathComponent::IsRunning(void) const
{
  return m_bIsRunning;
}

void plFollowPathComponent::SetRunning(bool b)
{
  m_bIsRunning = b;
}

void plFollowPathComponent::SetDirectionForwards(bool bForwards)
{
  m_bIsRunningForwards = bForwards;
}

void plFollowPathComponent::ToggleDirection()
{
  m_bIsRunningForwards = !m_bIsRunningForwards;
}

bool plFollowPathComponent::IsDirectionForwards() const
{
  return m_bIsRunningForwards;
}


PL_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_FollowPathComponent);

