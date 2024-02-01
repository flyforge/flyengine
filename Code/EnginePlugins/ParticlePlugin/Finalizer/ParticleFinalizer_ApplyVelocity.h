#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class PL_PARTICLEPLUGIN_DLL plParticleFinalizerFactory_ApplyVelocity final : public plParticleFinalizerFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleFinalizerFactory_ApplyVelocity, plParticleFinalizerFactory);

public:
  plParticleFinalizerFactory_ApplyVelocity();

  virtual const plRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(plParticleFinalizer* pObject, bool bFirstTime) const override;
};


class PL_PARTICLEPLUGIN_DLL plParticleFinalizer_ApplyVelocity final : public plParticleFinalizer
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleFinalizer_ApplyVelocity, plParticleFinalizer);

public:
  plParticleFinalizer_ApplyVelocity();
  ~plParticleFinalizer_ApplyVelocity();

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamPosition = nullptr;
  plProcessingStream* m_pStreamVelocity = nullptr;
};
