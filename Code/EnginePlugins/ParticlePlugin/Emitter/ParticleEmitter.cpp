#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEmitterFactory, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEmitter, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleEmitter* plParticleEmitterFactory::CreateEmitter(plParticleSystemInstance* pOwner) const
{
  const plRTTI* pRtti = GetEmitterType();

  plParticleEmitter* pEmitter = pRtti->GetAllocator()->Allocate<plParticleEmitter>();
  pEmitter->Reset(pOwner);

  CopyEmitterProperties(pEmitter, true);
  pEmitter->CreateRequiredStreams();

  return pEmitter;
}

bool plParticleEmitter::IsContinuous() const
{
  return false;
}

void plParticleEmitter::Process(plUInt64 uiNumElements) {}
void plParticleEmitter::ProcessEventQueue(plParticleEventQueue queue) {}


PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter);
