#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class PL_PARTICLEPLUGIN_DLL plParticleBehaviorFactory_Flies final : public plParticleBehaviorFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleBehaviorFactory_Flies, plParticleBehaviorFactory);

public:
  plParticleBehaviorFactory_Flies();
  ~plParticleBehaviorFactory_Flies();

  virtual const plRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void QueryFinalizerDependencies(plSet<const plRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

  float m_fSpeed = 0.2f;
  float m_fPathLength = 0.2f;
  float m_fMaxEmitterDistance = 0.5f;
  plAngle m_MaxSteeringAngle = plAngle::MakeFromDegree(30);
};


class PL_PARTICLEPLUGIN_DLL plParticleBehavior_Flies final : public plParticleBehavior
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleBehavior_Flies, plParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fSpeed = 0.2f;
  float m_fPathLength = 0.2f;
  float m_fMaxEmitterDistance = 0.5f;
  plAngle m_MaxSteeringAngle = plAngle::MakeFromDegree(30);

protected:
  virtual void Process(plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamPosition = nullptr;
  plProcessingStream* m_pStreamVelocity = nullptr;

  plTime m_TimeToChangeDir;
};
