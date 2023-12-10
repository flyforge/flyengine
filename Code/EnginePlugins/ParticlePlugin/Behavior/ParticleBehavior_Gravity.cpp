#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Gravity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehaviorFactory_Gravity, 1, plRTTIDefaultAllocator<plParticleBehaviorFactory_Gravity>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("GravityFactor", m_fGravityFactor)->AddAttributes(new plDefaultValueAttribute(1.0f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehavior_Gravity, 1, plRTTIDefaultAllocator<plParticleBehavior_Gravity>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleBehaviorFactory_Gravity::plParticleBehaviorFactory_Gravity()
{
  m_fGravityFactor = 1.0f;
}

const plRTTI* plParticleBehaviorFactory_Gravity::GetBehaviorType() const
{
  return plGetStaticRTTI<plParticleBehavior_Gravity>();
}

void plParticleBehaviorFactory_Gravity::CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const
{
  plParticleBehavior_Gravity* pBehavior = static_cast<plParticleBehavior_Gravity*>(pObject);

  pBehavior->m_fGravityFactor = m_fGravityFactor;

  pBehavior->m_pPhysicsModule = (plPhysicsWorldModuleInterface*)pBehavior->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(plGetStaticRTTI<plPhysicsWorldModuleInterface>());
}

void plParticleBehaviorFactory_Gravity::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_fGravityFactor;
}

void plParticleBehaviorFactory_Gravity::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_fGravityFactor;
}

void plParticleBehaviorFactory_Gravity::QueryFinalizerDependencies(plSet<const plRTTI*>& inout_FinalizerDeps) const
{
  inout_FinalizerDeps.Insert(plGetStaticRTTI<plParticleFinalizerFactory_ApplyVelocity>());
}

//////////////////////////////////////////////////////////////////////////

void plParticleBehavior_Gravity::CreateRequiredStreams()
{
  CreateStream("Velocity", plProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void plParticleBehavior_Gravity::Process(plUInt64 uiNumElements)
{
  PLASMA_PROFILE_SCOPE("PFX: Gravity");

  const plVec3 vGravity = m_pPhysicsModule != nullptr ? m_pPhysicsModule->GetGravity() : plVec3(0.0f, 0.0f, -10.0f);

  const float tDiff = (float)m_TimeDiff.GetSeconds();
  const plVec3 addGravity = vGravity * m_fGravityFactor * tDiff;

  plProcessingStreamIterator<plVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  while (!itVelocity.HasReachedEnd())
  {
    itVelocity.Current() += addGravity;

    itVelocity.Advance();
  }
}

void plParticleBehavior_Gravity::RequestRequiredWorldModulesForCache(plParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<plPhysicsWorldModuleInterface>();
}


PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Gravity);
