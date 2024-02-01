#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleFinalizerFactory, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleFinalizer, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleFinalizer* plParticleFinalizerFactory::CreateFinalizer(plParticleSystemInstance* pOwner) const
{
  const plRTTI* pRtti = GetFinalizerType();

  plParticleFinalizer* pFinalizer = pRtti->GetAllocator()->Allocate<plParticleFinalizer>();
  pFinalizer->Reset(pOwner);

  CopyFinalizerProperties(pFinalizer, true);
  pFinalizer->CreateRequiredStreams();

  return pFinalizer;
}

plParticleFinalizer::plParticleFinalizer()
{
  // run after the behaviors, before the types
  m_fPriority = +500.0f;
}


PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer);

