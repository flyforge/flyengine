#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class PL_PARTICLEPLUGIN_DLL plParticleFinalizerFactory_LastPosition final : public plParticleFinalizerFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleFinalizerFactory_LastPosition, plParticleFinalizerFactory);

public:
  plParticleFinalizerFactory_LastPosition();

  virtual const plRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(plParticleFinalizer* pObject, bool bFirstTime) const override;
};


class PL_PARTICLEPLUGIN_DLL plParticleFinalizer_LastPosition final : public plParticleFinalizer
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleFinalizer_LastPosition, plParticleFinalizer);

public:
  plParticleFinalizer_LastPosition();
  ~plParticleFinalizer_LastPosition();

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamPosition = nullptr;
  plProcessingStream* m_pStreamLastPosition = nullptr;
};
