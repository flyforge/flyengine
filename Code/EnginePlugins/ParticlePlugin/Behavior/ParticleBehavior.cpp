#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehaviorFactory, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehavior, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;


plParticleBehavior* plParticleBehaviorFactory::CreateBehavior(plParticleSystemInstance* pOwner) const
{
  const plRTTI* pRtti = GetBehaviorType();

  plParticleBehavior* pBehavior = pRtti->GetAllocator()->Allocate<plParticleBehavior>();
  pBehavior->Reset(pOwner);

  CopyBehaviorProperties(pBehavior, true);
  pBehavior->CreateRequiredStreams();

  return pBehavior;
}

plParticleBehavior::plParticleBehavior()
{
  // run after the initializers, before the types
  m_fPriority = 0.0f;
}

PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior);
