#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Velocity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehaviorFactory_Velocity, 1, plRTTIDefaultAllocator<plParticleBehaviorFactory_Velocity>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("RiseSpeed", m_fRiseSpeed),
    PLASMA_MEMBER_PROPERTY("Friction", m_fFriction)->AddAttributes(new plClampValueAttribute(0.0f, 100.0f)),
    PLASMA_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new plClampValueAttribute(0.0f, 1.0f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehavior_Velocity, 1, plRTTIDefaultAllocator<plParticleBehavior_Velocity>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleBehaviorFactory_Velocity::plParticleBehaviorFactory_Velocity() = default;
plParticleBehaviorFactory_Velocity::~plParticleBehaviorFactory_Velocity() = default;

const plRTTI* plParticleBehaviorFactory_Velocity::GetBehaviorType() const
{
  return plGetStaticRTTI<plParticleBehavior_Velocity>();
}

void plParticleBehaviorFactory_Velocity::CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const
{
  plParticleBehavior_Velocity* pBehavior = static_cast<plParticleBehavior_Velocity*>(pObject);

  pBehavior->m_fRiseSpeed = m_fRiseSpeed;
  pBehavior->m_fFriction = m_fFriction;
  pBehavior->m_fWindInfluence = m_fWindInfluence;


  pBehavior->m_pPhysicsModule = (plPhysicsWorldModuleInterface*)pBehavior->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(plGetStaticRTTI<plPhysicsWorldModuleInterface>());
}

enum class BehaviorVelocityVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added rise speed and acceleration
  Version_3, // added wind influence

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleBehaviorFactory_Velocity::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = (int)BehaviorVelocityVersion::Version_Current;
  stream << uiVersion;

  stream << m_fRiseSpeed;
  stream << m_fFriction;

  // Version 3
  stream << m_fWindInfluence;
}

void plParticleBehaviorFactory_Velocity::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  PLASMA_ASSERT_DEV(uiVersion <= (int)BehaviorVelocityVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_fRiseSpeed;
  stream >> m_fFriction;

  if (uiVersion >= 3)
  {
    stream >> m_fWindInfluence;
  }
}

void plParticleBehaviorFactory_Velocity::QueryFinalizerDependencies(plSet<const plRTTI*>& inout_FinalizerDeps) const
{
  inout_FinalizerDeps.Insert(plGetStaticRTTI<plParticleFinalizerFactory_ApplyVelocity>());
}

void plParticleBehavior_Velocity::CreateRequiredStreams()
{
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", plProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void plParticleBehavior_Velocity::Process(plUInt64 uiNumElements)
{
  PLASMA_PROFILE_SCOPE("PFX: Velocity");

  const float tDiff = (float)m_TimeDiff.GetSeconds();
  const plVec3 vDown = m_pPhysicsModule != nullptr ? m_pPhysicsModule->GetGravity().GetNormalized() : plVec3(0.0f, 0.0f, -1.0f);
  const plVec3 vRise = vDown * tDiff * -m_fRiseSpeed;

  auto pOwner = GetOwnerEffect();
  plVec3 vWind(0);

  if (m_iWindSampleIdx >= 0)
  {
    plVec3 vCurWind = pOwner->GetWindSampleResult(m_iWindSampleIdx);
    vCurWind = plMath::Lerp(m_vLastWind, vCurWind, tDiff);

    vWind = vCurWind * m_fWindInfluence * tDiff;

    m_vLastWind = vCurWind;

    m_iWindSampleIdx = -1;
  }

  if (m_fWindInfluence > 0)
  {
    m_iWindSampleIdx = pOwner->AddWindSampleLocation(GetOwnerSystem()->GetTransform().m_vPosition);
  }

  const plVec3 vAddPos0 = vRise + vWind;

  plSimdVec4f vAddPos;
  vAddPos.Load<3>(&vAddPos0.x);

  const float fFriction = plMath::Clamp(m_fFriction, 0.0f, 100.0f);
  const float fFrictionFactor = plMath::Pow(0.5f, tDiff * fFriction);

  plProcessingStreamIterator<plSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);
  plProcessingStreamIterator<plVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    itPosition.Current() += vAddPos;
    itVelocity.Current() *= fFrictionFactor;

    itPosition.Advance();
    itVelocity.Advance();
  }
}

void plParticleBehavior_Velocity::RequestRequiredWorldModulesForCache(plParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<plPhysicsWorldModuleInterface>();
  pParticleModule->CacheWorldModule<plWindWorldModuleInterface>();
}


PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Velocity);
