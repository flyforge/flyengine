#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializerFactory, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializer, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleInitializer* plParticleInitializerFactory::CreateInitializer(plParticleSystemInstance* pOwner) const
{
  const plRTTI* pRtti = GetInitializerType();

  plParticleInitializer* pInitializer = pRtti->GetAllocator()->Allocate<plParticleInitializer>();
  pInitializer->Reset(pOwner);

  CopyInitializerProperties(pInitializer, true);
  pInitializer->CreateRequiredStreams();

  return pInitializer;
}

float plParticleInitializerFactory::GetSpawnCountMultiplier(const plParticleEffectInstance* pEffect) const
{
  return 1.0f;
}

plParticleInitializer::plParticleInitializer()
{
  // run these early, but after the stream default initializers
  m_fPriority = -500.0f;
}

PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer);
