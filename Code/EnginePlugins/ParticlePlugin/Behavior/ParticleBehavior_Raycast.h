#pragma once

#include <Foundation/Strings/String.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class plPhysicsWorldModuleInterface;

struct PLASMA_PARTICLEPLUGIN_DLL plParticleRaycastHitReaction
{
  typedef plUInt8 StorageType;

  enum Enum
  {
    Bounce,
    Die,
    Stop,

    Default = Bounce
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_PARTICLEPLUGIN_DLL, plParticleRaycastHitReaction);

class PLASMA_PARTICLEPLUGIN_DLL plParticleBehaviorFactory_Raycast final : public plParticleBehaviorFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleBehaviorFactory_Raycast, plParticleBehaviorFactory);

public:
  plParticleBehaviorFactory_Raycast();
  ~plParticleBehaviorFactory_Raycast();

  virtual const plRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void QueryFinalizerDependencies(plSet<const plRTTI*>& inout_FinalizerDeps) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

  plEnum<plParticleRaycastHitReaction> m_Reaction;
  plUInt8 m_uiCollisionLayer = 0;
  plString m_sOnCollideEvent;
  float m_fBounceFactor = 0.6f;
};


class PLASMA_PARTICLEPLUGIN_DLL plParticleBehavior_Raycast final : public plParticleBehavior
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleBehavior_Raycast, plParticleBehavior);

public:
  plParticleBehavior_Raycast();

  virtual void CreateRequiredStreams() override;

  plEnum<plParticleRaycastHitReaction> m_Reaction;
  plUInt8 m_uiCollisionLayer = 0;
  plTempHashedString m_sOnCollideEvent;
  float m_fBounceFactor = 0.6f;

protected:
  friend class plParticleBehaviorFactory_Raycast;

  virtual void Process(plUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(plParticleWorldModule* pParticleModule) override;

  plPhysicsWorldModuleInterface* m_pPhysicsModule;

  plProcessingStream* m_pStreamPosition = nullptr;
  plProcessingStream* m_pStreamLastPosition = nullptr;
  plProcessingStream* m_pStreamVelocity = nullptr;
};
