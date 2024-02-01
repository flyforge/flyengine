#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Effects/Shake/CameraShakeComponent.h>
#include <GameComponentsPlugin/Effects/Shake/CameraShakeVolumeComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plCameraShakeComponent, 1, plComponentMode::Dynamic)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("MinShake", m_MinShake),
    PL_MEMBER_PROPERTY("MaxShake", m_MaxShake)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(5))),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Effects/CameraShake"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCameraShakeComponent::plCameraShakeComponent() = default;
plCameraShakeComponent::~plCameraShakeComponent() = default;

void plCameraShakeComponent::Update()
{
  const plTime tDuration = plTime::MakeFromSeconds(1.0 / 30.0); // 30 Hz vibration seems to work well

  const plTime tNow = plTime::Now();

  if (tNow >= m_ReferenceTime + tDuration)
  {
    GetOwner()->SetLocalRotation(m_qNextTarget);
    GenerateKeyframe();
  }
  else
  {
    const float fLerp = plMath::Clamp((tNow - m_ReferenceTime).AsFloatInSeconds() / tDuration.AsFloatInSeconds(), 0.0f, 1.0f);

    plQuat q = plQuat::MakeSlerp(m_qPrevTarget, m_qNextTarget, fLerp);

    GetOwner()->SetLocalRotation(q);
  }
}

void plCameraShakeComponent::GenerateKeyframe()
{
  m_qPrevTarget = m_qNextTarget;

  m_ReferenceTime = plTime::Now();

  plWorld* pWorld = GetWorld();

  // fade out shaking over a second, if the vibration stopped
  m_fLastStrength -= pWorld->GetClock().GetTimeDiff().AsFloatInSeconds();

  const float fShake = plMath::Clamp(GetStrengthAtPosition(), 0.0f, 1.0f);

  m_fLastStrength = plMath::Max(m_fLastStrength, fShake);

  plAngle deviation;
  deviation = plMath::Lerp(m_MinShake, m_MaxShake, m_fLastStrength);

  if (deviation > plAngle())
  {
    m_Rotation += plAngle::MakeFromRadian(pWorld->GetRandomNumberGenerator().FloatMinMax(plAngle::MakeFromDegree(120).GetRadian(), plAngle::MakeFromDegree(240).GetRadian()));
    m_Rotation.NormalizeRange();

    plQuat qRot;
    qRot = plQuat::MakeFromAxisAndAngle(plVec3::MakeAxisX(), m_Rotation);

    const plVec3 tiltAxis = qRot * plVec3::MakeAxisZ();

    m_qNextTarget = plQuat::MakeFromAxisAndAngle(tiltAxis, deviation);
  }
  else
  {
    m_qNextTarget.SetIdentity();
  }
}

float plCameraShakeComponent::GetStrengthAtPosition() const
{
  float force = 0;

  if (auto pSpatial = GetWorld()->GetSpatialSystem())
  {
    const plVec3 vPosition = GetOwner()->GetGlobalPosition();

    plHybridArray<plGameObject*, 16> volumes;

    plSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = plCameraShakeVolumeComponent::SpatialDataCategory.GetBitmask();

    pSpatial->FindObjectsInSphere(plBoundingSphere::MakeFromCenterAndRadius(vPosition, 0.5f), queryParams, volumes);

    const plSimdVec4f pos = plSimdConversion::ToVec3(vPosition);

    for (plGameObject* pObj : volumes)
    {
      plCameraShakeVolumeComponent* pVol;
      if (pObj->TryGetComponentOfBaseType(pVol))
      {
        force = plMath::Max(force, pVol->ComputeForceAtGlobalPosition(pos));
      }
    }
  }

  return force;
}

void plCameraShakeComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_MinShake;
  s << m_MaxShake;
}

void plCameraShakeComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_MinShake;
  s >> m_MaxShake;
}
