#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Continuous.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEmitterFactory_Continuous, 1, plRTTIDefaultAllocator<plParticleEmitterFactory_Continuous>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("StartDelay", m_StartDelay),

    PLASMA_MEMBER_PROPERTY("SpawnCountPerSec", m_uiSpawnCountPerSec)->AddAttributes(new plDefaultValueAttribute(10)),
    PLASMA_MEMBER_PROPERTY("SpawnCountPerSecRange", m_uiSpawnCountPerSecRange),
    PLASMA_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),

    PLASMA_ACCESSOR_PROPERTY("CountCurve", GetCountCurveFile, SetCountCurveFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    PLASMA_MEMBER_PROPERTY("CurveDuration", m_CurveDuration)->AddAttributes(new plDefaultValueAttribute(plTime::Seconds(10.0))),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEmitter_Continuous, 1, plRTTIDefaultAllocator<plParticleEmitter_Continuous>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleEmitterFactory_Continuous::plParticleEmitterFactory_Continuous()
{
  m_uiSpawnCountPerSec = 10;
  m_uiSpawnCountPerSecRange = 0;

  m_CurveDuration = plTime::Seconds(10.0);
}


const plRTTI* plParticleEmitterFactory_Continuous::GetEmitterType() const
{
  return plGetStaticRTTI<plParticleEmitter_Continuous>();
}

void plParticleEmitterFactory_Continuous::CopyEmitterProperties(plParticleEmitter* pEmitter0, bool bFirstTime) const
{
  plParticleEmitter_Continuous* pEmitter = static_cast<plParticleEmitter_Continuous*>(pEmitter0);

  pEmitter->m_StartDelay = m_StartDelay;

  pEmitter->m_uiSpawnCountPerSec = (plUInt32)(m_uiSpawnCountPerSec * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountPerSecRange = (plUInt32)(m_uiSpawnCountPerSecRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());

  pEmitter->m_sSpawnCountScaleParameter = plTempHashedString(m_sSpawnCountScaleParameter.GetData());

  pEmitter->m_hCountCurve = m_hCountCurve;
  pEmitter->m_CurveDuration = plMath::Max(m_CurveDuration, plTime::Seconds(1.0));
}

void plParticleEmitterFactory_Continuous::QueryMaxParticleCount(plUInt32& out_uiMaxParticlesAbs, plUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs = 0;
  out_uiMaxParticlesPerSecond = m_uiSpawnCountPerSec + (m_uiSpawnCountPerSecRange * 3 / 4); // don't be too pessimistic

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterContinuousVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4, // added emitter start delay
  Version_5, // added spawn count scale param
  Version_6, // removed duration, switched to particles per second

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void plParticleEmitterFactory_Continuous::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = (int)EmitterContinuousVersion::Version_Current;
  stream << uiVersion;

  // Version 4
  stream << m_StartDelay;

  // Version 6
  stream << m_uiSpawnCountPerSec;
  stream << m_uiSpawnCountPerSecRange;

  // Version 2
  stream << m_hCountCurve;
  stream << m_CurveDuration;

  // Version 5
  stream << m_sSpawnCountScaleParameter;
}

void plParticleEmitterFactory_Continuous::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  PLASMA_ASSERT_DEV(uiVersion <= (int)EmitterContinuousVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 3 && uiVersion < 6)
  {
    plTime duraton;
    stream >> duraton;
  }

  if (uiVersion >= 4)
  {
    stream >> m_StartDelay;
  }

  stream >> m_uiSpawnCountPerSec;
  stream >> m_uiSpawnCountPerSecRange;

  if (uiVersion < 6)
  {
    plVarianceTypeFloat interval;
    stream >> interval.m_Value;
    stream >> interval.m_fVariance;
  }

  if (uiVersion >= 2)
  {
    stream >> m_hCountCurve;
    stream >> m_CurveDuration;
  }

  if (uiVersion >= 5)
  {
    stream >> m_sSpawnCountScaleParameter;
  }
}

void plParticleEmitterFactory_Continuous::SetCountCurveFile(const char* szFile)
{
  plCurve1DResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plCurve1DResource>(szFile);
  }

  m_hCountCurve = hResource;
}

const char* plParticleEmitterFactory_Continuous::GetCountCurveFile() const
{
  if (!m_hCountCurve.IsValid())
    return "";

  return m_hCountCurve.GetResourceID();
}

void plParticleEmitter_Continuous::OnFinalize()
{
  m_CountCurveTime.SetZero();
  m_fCurSpawnPerSec = (float)GetRNG().DoubleInRange(m_uiSpawnCountPerSec, m_uiSpawnCountPerSecRange);
  m_TimeSinceRandom.SetZero();
  m_fCurSpawnCounter = 0;
}

plParticleEmitterState plParticleEmitter_Continuous::IsFinished()
{
  return plParticleEmitterState::Active;
}

plUInt32 plParticleEmitter_Continuous::ComputeSpawnCount(const plTime& tDiff)
{
  PLASMA_PROFILE_SCOPE("PFX: Continuous - Spawn Count ");

  // delay before the emitter becomes active
  if (m_StartDelay.IsPositive())
  {
    m_StartDelay -= tDiff;
    return 0;
  }

  m_TimeSinceRandom += tDiff;
  m_CountCurveTime += tDiff;

  if (m_TimeSinceRandom >= plTime::Milliseconds(200))
  {
    m_TimeSinceRandom.SetZero();
    m_fCurSpawnPerSec = (float)GetRNG().DoubleInRange(m_uiSpawnCountPerSec, m_uiSpawnCountPerSecRange);
  }


  float fSpawnFactor = 1.0f;

  if (m_hCountCurve.IsValid())
  {
    plResourceLock<plCurve1DResource> pCurve(m_hCountCurve, plResourceAcquireMode::BlockTillLoaded);

    if (!pCurve->GetDescriptor().m_Curves.IsEmpty())
    {
      while (m_CountCurveTime > m_CurveDuration)
        m_CountCurveTime -= m_CurveDuration;

      const auto& curve = pCurve->GetDescriptor().m_Curves[0];

      const double normPos = (float)(m_CountCurveTime.GetSeconds() / m_CurveDuration.GetSeconds());
      const double evalPos = curve.ConvertNormalizedPos(normPos);

      fSpawnFactor = (float)plMath::Max(0.0, curve.Evaluate(evalPos));
    }
  }

  const float spawnCountScale = plMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;


  m_fCurSpawnCounter += fSpawnFactor * m_fCurSpawnPerSec * (float)tDiff.GetSeconds();

  const plUInt32 uiSpawn = (plUInt32)m_fCurSpawnCounter;
  m_fCurSpawnCounter -= uiSpawn;

  return uiSpawn;
}



PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Continuous);
