#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class PLASMA_PARTICLEPLUGIN_DLL plParticleFinalizerFactory_LastPosition final : public plParticleFinalizerFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleFinalizerFactory_LastPosition, plParticleFinalizerFactory);

public:
  plParticleFinalizerFactory_LastPosition();

  virtual const plRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(plParticleFinalizer* pObject, bool bFirstTime) const override;
};


class PLASMA_PARTICLEPLUGIN_DLL plParticleFinalizer_LastPosition final : public plParticleFinalizer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleFinalizer_LastPosition, plParticleFinalizer);

public:
  plParticleFinalizer_LastPosition();
  ~plParticleFinalizer_LastPosition();

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamPosition = nullptr;
  plProcessingStream* m_pStreamLastPosition = nullptr;
};
