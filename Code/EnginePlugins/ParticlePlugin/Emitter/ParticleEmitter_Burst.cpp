#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Burst.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEmitterFactory_Burst, 1, plRTTIDefaultAllocator<plParticleEmitterFactory_Burst>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Duration", m_Duration),
    PL_MEMBER_PROPERTY("StartDelay", m_StartDelay),

    PL_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new plDefaultValueAttribute(10)),
    PL_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),
    PL_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEmitter_Burst, 1, plRTTIDefaultAllocator<plParticleEmitter_Burst>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleEmitterFactory_Burst::plParticleEmitterFactory_Burst()
{
  m_uiSpawnCountMin = 10;
  m_uiSpawnCountRange = 0;
}

const plRTTI* plParticleEmitterFactory_Burst::GetEmitterType() const
{
  return plGetStaticRTTI<plParticleEmitter_Burst>();
}

void plParticleEmitterFactory_Burst::CopyEmitterProperties(plParticleEmitter* pEmitter0, bool bFirstTime) const
{
  plParticleEmitter_Burst* pEmitter = static_cast<plParticleEmitter_Burst*>(pEmitter0);

  pEmitter->m_Duration = m_Duration;
  pEmitter->m_StartDelay = m_StartDelay;

  pEmitter->m_uiSpawnCountMin = (plUInt32)(m_uiSpawnCountMin * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountRange = (plUInt32)(m_uiSpawnCountRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_sSpawnCountScaleParameter = plTempHashedString(m_sSpawnCountScaleParameter.GetData());
}


void plParticleEmitterFactory_Burst::QueryMaxParticleCount(plUInt32& out_uiMaxParticlesAbs, plUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs = m_uiSpawnCountMin + m_uiSpawnCountRange;
  out_uiMaxParticlesPerSecond = 0;

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterBurstVersion
{
  Version_1 = 1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void plParticleEmitterFactory_Burst::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = (int)EmitterBurstVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_Duration;
  inout_stream << m_StartDelay;
  inout_stream << m_uiSpawnCountMin;
  inout_stream << m_uiSpawnCountRange;
  inout_stream << m_sSpawnCountScaleParameter;
}

void plParticleEmitterFactory_Burst::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion <= (int)EmitterBurstVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_Duration;
  inout_stream >> m_StartDelay;
  inout_stream >> m_uiSpawnCountMin;
  inout_stream >> m_uiSpawnCountRange;
  inout_stream >> m_sSpawnCountScaleParameter;
}

void plParticleEmitter_Burst::OnFinalize()
{
  float fSpawnFactor = 1.0f;

  const float spawnCountScale = plMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  plRandom& rng = GetRNG();

  m_uiSpawnCountLeft = (plUInt32)(rng.IntInRange(m_uiSpawnCountMin, 1 + m_uiSpawnCountRange) * fSpawnFactor);

  m_fSpawnAccu = 0;
  m_fSpawnPerSecond = 0;

  if (!m_Duration.IsZero())
  {
    m_fSpawnPerSecond = m_uiSpawnCountLeft / (float)m_Duration.GetSeconds();
  }
}

plParticleEmitterState plParticleEmitter_Burst::IsFinished()
{
  return (m_uiSpawnCountLeft == 0) ? plParticleEmitterState::Finished : plParticleEmitterState::Active;
}

plUInt32 plParticleEmitter_Burst::ComputeSpawnCount(const plTime& tDiff)
{
  PL_PROFILE_SCOPE("PFX: Burst - Spawn Count ");

  // delay before the emitter becomes active
  if (m_StartDelay.IsPositive())
  {
    m_StartDelay -= tDiff;
    return 0;
  }

  plUInt32 uiSpawn = 0;

  if (m_Duration.IsZero())
  {
    uiSpawn = m_uiSpawnCountLeft;
    m_uiSpawnCountLeft = 0;
  }
  else
  {
    m_fSpawnAccu += (float)tDiff.GetSeconds() * m_fSpawnPerSecond;
    uiSpawn = (plUInt32)m_fSpawnAccu;
    uiSpawn = plMath::Min(uiSpawn, m_uiSpawnCountLeft);

    m_fSpawnAccu -= uiSpawn;
    m_uiSpawnCountLeft -= uiSpawn;
  }

  return uiSpawn;
}


PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Burst);

