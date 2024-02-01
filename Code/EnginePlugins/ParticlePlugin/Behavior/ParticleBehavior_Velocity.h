#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class plPhysicsWorldModuleInterface;
class plWindWorldModuleInterface;

class PL_PARTICLEPLUGIN_DLL plParticleBehaviorFactory_Velocity final : public plParticleBehaviorFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleBehaviorFactory_Velocity, plParticleBehaviorFactory);

public:
  plParticleBehaviorFactory_Velocity();
  ~plParticleBehaviorFactory_Velocity();

  virtual const plRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

  virtual void QueryFinalizerDependencies(plSet<const plRTTI*>& inout_finalizerDeps) const override;

  float m_fRiseSpeed = 0;
  float m_fFriction = 0;
  float m_fWindInfluence = 0;
};


class PL_PARTICLEPLUGIN_DLL plParticleBehavior_Velocity final : public plParticleBehavior
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleBehavior_Velocity, plParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fRiseSpeed = 0;
  float m_fFriction = 0;
  float m_fWindInfluence = 0;

protected:
  friend class plParticleBehaviorFactory_Velocity;

  virtual void Process(plUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(plParticleWorldModule* pParticleModule) override;

  // used to rise/fall along the gravity vector
  plPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;
  //plWindWorldModuleInterface* m_pWindModule = nullptr;
  plInt32 m_iWindSampleIdx = -1;

  plProcessingStream* m_pStreamPosition;
  plProcessingStream* m_pStreamVelocity;

  plVec3 m_vLastWind = plVec3::MakeZero();
};
