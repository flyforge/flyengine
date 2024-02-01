#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class plPhysicsWorldModuleInterface;

class PL_PARTICLEPLUGIN_DLL plParticleBehaviorFactory_Gravity final : public plParticleBehaviorFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleBehaviorFactory_Gravity, plParticleBehaviorFactory);

public:
  plParticleBehaviorFactory_Gravity();

  virtual const plRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void QueryFinalizerDependencies(plSet<const plRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

public:
  float m_fGravityFactor;
};


class PL_PARTICLEPLUGIN_DLL plParticleBehavior_Gravity final : public plParticleBehavior
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleBehavior_Gravity, plParticleBehavior);

public:
  float m_fGravityFactor;

  virtual void CreateRequiredStreams() override;

protected:
  friend class plParticleBehaviorFactory_Gravity;

  virtual void Process(plUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(plParticleWorldModule* pParticleModule) override;

  plPhysicsWorldModuleInterface* m_pPhysicsModule;

  plProcessingStream* m_pStreamVelocity;
};
