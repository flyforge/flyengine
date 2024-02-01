#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class plPhysicsWorldModuleInterface;

class PL_PARTICLEPLUGIN_DLL plParticleBehaviorFactory_Bounds final : public plParticleBehaviorFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleBehaviorFactory_Bounds, plParticleBehaviorFactory);

public:
  plParticleBehaviorFactory_Bounds();

  virtual const plRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

  plVec3 m_vPositionOffset;
  plVec3 m_vBoxExtents;
  plEnum<plParticleOutOfBoundsMode> m_OutOfBoundsMode;
};


class PL_PARTICLEPLUGIN_DLL plParticleBehavior_Bounds final : public plParticleBehavior
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleBehavior_Bounds, plParticleBehavior);

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
