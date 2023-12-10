#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class plPhysicsWorldModuleInterface;

class PLASMA_PARTICLEPLUGIN_DLL plParticleFinalizerFactory_Age final : public plParticleFinalizerFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleFinalizerFactory_Age, plParticleFinalizerFactory);

public:
  plParticleFinalizerFactory_Age();

  virtual const plRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(plParticleFinalizer* pObject, bool bFirstTime) const override;

  plVarianceTypeTime m_LifeTime;
  plString m_sOnDeathEvent;
  plString m_sLifeScaleParameter;
};


class PLASMA_PARTICLEPLUGIN_DLL plParticleFinalizer_Age final : public plParticleFinalizer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleFinalizer_Age, plParticleFinalizer);

public:
  plParticleFinalizer_Age();
  ~plParticleFinalizer_Age();

  virtual void CreateRequiredStreams() override;

  plVarianceTypeTime m_LifeTime;
  plTempHashedString m_sOnDeathEvent;
  plTempHashedString m_sLifeScaleParameter;

protected:
  friend class plParticleFinalizerFactory_Age;

  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;
  virtual void Process(plUInt64 uiNumElements) override;
  void OnParticleDeath(const plStreamGroupElementRemovedEvent& e);

  bool m_bHasOnDeathEventHandler = false;
  plProcessingStream* m_pStreamLifeTime = nullptr;
  plProcessingStream* m_pStreamPosition = nullptr;
  plProcessingStream* m_pStreamVelocity = nullptr;
};
