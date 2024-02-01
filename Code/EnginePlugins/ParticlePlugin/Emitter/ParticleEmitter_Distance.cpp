#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Distance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEmitterFactory_Distance, 1, plRTTIDefaultAllocator<plParticleEmitterFactory_Distance>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("DistanceThreshold", m_fDistanceThreshold)->AddAttributes(new plDefaultValueAttribute(0.1f), new plClampValueAttribute(0.01f, 100.0f)),
    PL_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new plDefaultValueAttribute(1)),
    PL_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),
    PL_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEmitter_Distance, 1, plRTTIDefaultAllocator<plParticleEmitter_Distance>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleEmitterFactory_Distance::plParticleEmitterFactory_Distance() = default;

const plRTTI* plParticleEmitterFactory_Distance::GetEmitterType() const
{
  return plGetStaticRTTI<plParticleEmitter_Distance>();
}

void plParticleEmitterFactory_Distance::CopyEmitterProperties(plParticleEmitter* pEmitter0, bool bFirstTime) const
{
  plParticleEmitter_Distance* pEmitter = static_cast<plParticleEmitter_Distance*>(pEmitter0);

  pEmitter->m_fDistanceThresholdSQR = plMath::Square(m_fDistanceThreshold);

  pEmitter->m_uiSpawnCountMin = (plUInt32)(m_uiSpawnCountMin * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountRange = (plUInt32)(m_uiSpawnCountRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());

  pEmitter->m_sSpawnCountScaleParameter = plTempHashedString(m_sSpawnCountScaleParameter.GetData());
}

void plParticleEmitterFactory_Distance::QueryMaxParticleCount(plUInt32& out_uiMaxParticlesAbs, plUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs = 0;
  out_uiMaxParticlesPerSecond = (m_uiSpawnCountMin + m_uiSpawnCountRange) * 10; // assume that this won't fire more than 10 times per second

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterDistanceVersion
{
  Version_1 = 1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void plParticleEmitterFactory_Distance::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = (int)EmitterDistanceVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_fDistanceThreshold;
  inout_stream << m_uiSpawnCountMin;
  inout_stream << m_uiSpawnCountRange;
  inout_stream << m_sSpawnCountScaleParameter;
}

void plParticleEmitterFactory_Distance::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion <= (int)EmitterDistanceVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fDistanceThreshold;
  inout_stream >> m_uiSpawnCountMin;
  inout_stream >> m_uiSpawnCountRange;
  inout_stream >> m_sSpawnCountScaleParameter;
}

void plParticleEmitter_Distance::CreateRequiredStreams() {}
void plParticleEmitter_Distance::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) {}

bool plParticleEmitter_Distance::IsContinuous() const
{
  return true;
}

void plParticleEmitter_Distance::OnFinalize()
{
  // do not use the System transform, because then this would not work with local space simulation
  m_vLastSpawnPosition = GetOwnerEffect()->GetTransform().m_vPosition;
  m_bFirstUpdate = true;

  if (GetOwnerEffect()->IsSharedEffect())
  {
    plLog::Warning("Particle emitters of type 'Distance' do not work for shared particle effect instances.");
  }
}

plParticleEmitterState plParticleEmitter_Distance::IsFinished()
{
  return plParticleEmitterState::Active;
}

plUInt32 plParticleEmitter_Distance::ComputeSpawnCount(const plTime& tDiff)
{
  const plVec3 vCurPos = GetOwnerEffect()->GetTransform().m_vPosition;

  if ((m_vLastSpawnPosition - vCurPos).GetLengthSquared() < m_fDistanceThresholdSQR)
    return 0;

  m_vLastSpawnPosition = vCurPos;

  if (m_bFirstUpdate)
  {
    m_bFirstUpdate = false;
    return 0;
  }

  float fSpawnFactor = 1.0f;

  const float spawnCountScale = plMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  plUInt32 uiSpawn = m_uiSpawnCountMin;

  if (m_uiSpawnCountRange > 0)
    uiSpawn += GetRNG().UIntInRange(m_uiSpawnCountRange);

  uiSpawn = static_cast<plUInt32>((float)uiSpawn * fSpawnFactor);

  return uiSpawn;
}


PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Distance);

