#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class plPhysicsWorldModuleInterface;

class PL_PARTICLEPLUGIN_DLL plParticleFinalizerFactory_Volume final : public plParticleFinalizerFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleFinalizerFactory_Volume, plParticleFinalizerFactory);

public:
  plParticleFinalizerFactory_Volume();
  ~plParticleFinalizerFactory_Volume();

  virtual const plRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(plParticleFinalizer* pObject, bool bFirstTime) const override;
};


class PL_PARTICLEPLUGIN_DLL plParticleFinalizer_Volume final : public plParticleFinalizer
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleFinalizer_Volume, plParticleFinalizer);

public:
  plParticleFinalizer_Volume();
  ~plParticleFinalizer_Volume();

  virtual void CreateRequiredStreams() override;
  virtual void QueryOptionalStreams() override;

protected:
  virtual void Process(plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamPosition = nullptr;
  const plProcessingStream* m_pStreamSize = nullptr;
};
