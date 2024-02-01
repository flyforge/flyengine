#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/SpawnBoxComponent.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plSpawnBoxComponent, 1, plComponentMode::Dynamic)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new plDefaultValueAttribute(plVec3(2.0f, 2.0f, 0.25f)), new plClampValueAttribute(plVec3(0), plVariant())),
    PL_ACCESSOR_PROPERTY("Prefab", GetPrefabFile, SetPrefabFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab", plDependencyFlags::Package)),
    PL_ACCESSOR_PROPERTY("SpawnAtStart", GetSpawnAtStart, SetSpawnAtStart),
    PL_ACCESSOR_PROPERTY("SpawnContinuously", GetSpawnContinuously, SetSpawnContinuously),
    PL_MEMBER_PROPERTY("MinSpawnCount", m_uiMinSpawnCount)->AddAttributes(new plDefaultValueAttribute(10)),
    PL_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange)->AddAttributes(new plDefaultValueAttribute(0)),
    PL_MEMBER_PROPERTY("Duration", m_SpawnDuration)->AddAttributes(new plDefaultValueAttribute(plTime::MakeFromSeconds(5))),
    PL_MEMBER_PROPERTY("MaxRotationZ", m_MaxRotationZ),
    PL_MEMBER_PROPERTY("MaxTiltZ", m_MaxTiltZ),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgComponentInternalTrigger, OnTriggered),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
    new plBoxManipulatorAttribute("HalfExtents", 2.0f, true),
    new plBoxVisualizerAttribute("HalfExtents", 2.0f),
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 0.5f, plColorScheme::LightUI(plColorScheme::Lime)),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(StartSpawning),
  }
  PL_END_FUNCTIONS;
}
PL_END_COMPONENT_TYPE;
// clang-format on

void plSpawnBoxComponent::SetHalfExtents(const plVec3& value)
{
  m_vHalfExtents = value.CompMax(plVec3::MakeZero());

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plSpawnBoxComponent::SetPrefabFile(const char* szFile)
{
  plPrefabResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plPrefabResource>(szFile);
  }

  m_hPrefab = hResource;
}

const char* plSpawnBoxComponent::GetPrefabFile() const
{
  if (!m_hPrefab.IsValid())
    return "";

  return m_hPrefab.GetResourceID();
}

bool plSpawnBoxComponent::GetSpawnAtStart() const
{
  return m_Flags.IsAnySet(plSpawnBoxComponentFlags::SpawnAtStart);
}

void plSpawnBoxComponent::SetSpawnAtStart(bool b)
{
  m_Flags.AddOrRemove(plSpawnBoxComponentFlags::SpawnAtStart, b);
}

bool plSpawnBoxComponent::GetSpawnContinuously() const
{
  return m_Flags.IsAnySet(plSpawnBoxComponentFlags::SpawnContinuously);
}

void plSpawnBoxComponent::SetSpawnContinuously(bool b)
{
  m_Flags.AddOrRemove(plSpawnBoxComponentFlags::SpawnContinuously, b);
}

void plSpawnBoxComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_vHalfExtents;
  s << m_hPrefab;
  s << m_Flags;
  s << m_SpawnDuration;
  s << m_uiMinSpawnCount;
  s << m_uiSpawnCountRange;
  s << m_MaxRotationZ;
  s << m_MaxTiltZ;
}

void plSpawnBoxComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s >> m_vHalfExtents;
  s >> m_hPrefab;
  s >> m_Flags;
  s >> m_SpawnDuration;
  s >> m_uiMinSpawnCount;
  s >> m_uiSpawnCountRange;
  s >> m_MaxRotationZ;
  s >> m_MaxTiltZ;
}

void plSpawnBoxComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (GetSpawnAtStart())
  {
    StartSpawning();
  }
}

void plSpawnBoxComponent::StartSpawning()
{
  InternalStartSpawning(true);
}

void plSpawnBoxComponent::InternalStartSpawning(bool bFirstTime)
{

  m_uiSpawned = 0;
  m_uiTotalToSpawn = m_uiMinSpawnCount;
  m_StartTime = GetWorld()->GetClock().GetAccumulatedTime();

  if (m_uiSpawnCountRange > 0)
  {
    m_uiTotalToSpawn = GetWorld()->GetRandomNumberGenerator().IntInRange(m_uiMinSpawnCount, m_uiSpawnCountRange);
  }

  if (m_uiTotalToSpawn == 0)
    return;

  if (m_SpawnDuration.IsZeroOrNegative())
  {
    Spawn(m_uiTotalToSpawn);
  }
  else
  {
    if (bFirstTime)
    {
      // this guarantees that next time OnTriggered() is called, one object gets spawned right away
      m_StartTime -= m_SpawnDuration / m_uiTotalToSpawn;
    }

    plMsgComponentInternalTrigger msg;
    PostMessage(msg, plTime::MakeZero());
  }
}

void plSpawnBoxComponent::OnTriggered(plMsgComponentInternalTrigger& msg)
{
  const plTime tNow = GetWorld()->GetClock().GetAccumulatedTime();
  const plTime tActive = tNow - m_StartTime;
  const plTime tEnd = m_StartTime + m_SpawnDuration;

  if (tNow >= tEnd)
  {
    if (m_uiSpawned < m_uiTotalToSpawn)
    {
      Spawn(m_uiTotalToSpawn - m_uiSpawned);
    }

    if (GetSpawnContinuously())
    {
      InternalStartSpawning(false);
    }

    return;
  }

  const auto uiTargetSpawnCount = plMath::Clamp<plUInt16>(static_cast<plUInt16>(((tActive.GetSeconds() / m_SpawnDuration.GetSeconds()) * m_uiTotalToSpawn)), 0, m_uiTotalToSpawn);

  if (m_uiSpawned < uiTargetSpawnCount)
  {
    Spawn(uiTargetSpawnCount - m_uiSpawned);
  }

  if (m_uiSpawned < m_uiTotalToSpawn)
  {
    // remaining time divided equally for the remaining spawns
    // this is to prevent a lot of unnecessary message sending at low spawn counts
    plTime tDelay = (tEnd - tNow) / (m_uiTotalToSpawn - m_uiSpawned);

    // prevent unnecessary high number of updates, rather spawn multiple objects within one frame
    tDelay = plMath::Max(tDelay, plTime::MakeFromMilliseconds(40)); // max 25 Hz

    plMsgComponentInternalTrigger msg;
    PostMessage(msg, tDelay);
  }
  else if (GetSpawnContinuously())
  {
    InternalStartSpawning(false);
  }
}

void plSpawnBoxComponent::Spawn(plUInt32 uiCount)
{
  if (uiCount == 0)
    return;

  m_uiSpawned += uiCount;

  if (!m_hPrefab.IsValid())
    return;

  plResourceLock<plPrefabResource> pResource(m_hPrefab, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pResource.GetAcquireResult() == plResourceAcquireResult::None)
    return;

  plPrefabInstantiationOptions options;
  options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

  plRandom& rnd = GetWorld()->GetRandomNumberGenerator();
  const plTransform tOwner = GetOwner()->GetGlobalTransform();

  for (plUInt32 i = 0; i < uiCount; ++i)
  {
    plTransform tLocal = plTransform::MakeIdentity();
    tLocal.m_vPosition.x = rnd.DoubleMinMax(-m_vHalfExtents.x, m_vHalfExtents.x);
    tLocal.m_vPosition.y = rnd.DoubleMinMax(-m_vHalfExtents.y, m_vHalfExtents.y);
    tLocal.m_vPosition.z = rnd.DoubleMinMax(-m_vHalfExtents.z, m_vHalfExtents.z);

    if (m_MaxRotationZ.GetRadian() > 0)
    {
      const plAngle rotationAngle = plAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(-m_MaxRotationZ.GetRadian(), +m_MaxRotationZ.GetRadian()));
      const plQuat qRot = plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), rotationAngle);

      tLocal.m_qRotation = qRot;
    }

    if (m_MaxTiltZ.GetRadian() > 0)
    {
      const plAngle tiltTurnAngle = plAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, plMath::Pi<double>() * 2.0));
      const plQuat qTiltTurn = plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), tiltTurnAngle);

      const plVec3 vTiltAxis = qTiltTurn * plVec3(1, 0, 0);

      const plAngle tiltAngle = plAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, (double)m_MaxTiltZ.GetRadian()));
      const plQuat qTilt = plQuat::MakeFromAxisAndAngle(vTiltAxis, tiltAngle);

      tLocal.m_qRotation = tLocal.m_qRotation * qTilt;
    }

    const plTransform tGlobal = plTransform::MakeGlobalTransform(tOwner, tLocal);

    pResource->InstantiatePrefab(*GetWorld(), tGlobal, options);
  }
}


PL_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_SpawnBoxComponent);

