#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class plPhysicsWorldModuleInterface;

class PL_PARTICLEPLUGIN_DLL plParticleBehaviorFactory_PullAlong final : public plParticleBehaviorFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleBehaviorFactory_PullAlong, plParticleBehaviorFactory);

public:
  plParticleBehaviorFactory_PullAlong();

  virtual const plRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

  float m_fStrength;
};


class PL_PARTICLEPLUGIN_DLL plParticleBehavior_PullAlong final : public plParticleBehavior
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleBehavior_PullAlong, plParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fStrength = 0.5;

protected:
  virtual void Process(plUInt64 uiNumElements) override;
  virtual void StepParticleSystem(const plTime& tDiff, plUInt32 uiNumNewParticles) override;

  bool m_bFirstTime = true;
  plVec3 m_vLastEmitterPosition;
  plVec3 m_vApplyPull;
  plProcessingStream* m_pStreamPosition;
};
