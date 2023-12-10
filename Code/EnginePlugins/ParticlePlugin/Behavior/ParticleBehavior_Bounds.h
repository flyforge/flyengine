#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class plPhysicsWorldModuleInterface;

class PLASMA_PARTICLEPLUGIN_DLL plParticleBehaviorFactory_Bounds final : public plParticleBehaviorFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleBehaviorFactory_Bounds, plParticleBehaviorFactory);

public:
  plParticleBehaviorFactory_Bounds();

  virtual const plRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

  plVec3 m_vPositionOffset;
  plVec3 m_vBoxExtents;
  plEnum<plParticleOutOfBoundsMode> m_OutOfBoundsMode;
};


class PLASMA_PARTICLEPLUGIN_DLL plParticleBehavior_Bounds final : public plParticleBehavior
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleBehavior_Bounds, plParticleBehavior);

public:
  plVec3 m_vPositionOffset;
  plVec3 m_vBoxExtents;
  plEnum<plParticleOutOfBoundsMode> m_OutOfBoundsMode;

protected:
  virtual void Process(plUInt64 uiNumElements) override;

  virtual void CreateRequiredStreams() override;
  virtual void QueryOptionalStreams() override;

  plProcessingStream* m_pStreamPosition = nullptr;
  plProcessingStream* m_pStreamLastPosition = nullptr;
};
