#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

class PLASMA_PARTICLEPLUGIN_DLL plParticleInitializerFactory_SpherePosition final : public plParticleInitializerFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleInitializerFactory_SpherePosition, plParticleInitializerFactory);

public:
  plParticleInitializerFactory_SpherePosition();

  virtual const plRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(plParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float GetSpawnCountMultiplier(const plParticleEffectInstance* pEffect) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

  virtual void QueryFinalizerDependencies(plSet<const plRTTI*>& inout_FinalizerDeps) const override;

public:
  plVec3 m_vPositionOffset;
  float m_fRadius;
  bool m_bSpawnOnSurface;
  bool m_bSetVelocity;
  plVarianceTypeFloat m_Speed;
  plString m_sScaleRadiusParameter;
};


class PLASMA_PARTICLEPLUGIN_DLL plParticleInitializer_SpherePosition final : public plParticleInitializer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleInitializer_SpherePosition, plParticleInitializer);

public:
  plVec3 m_vPositionOffset;
  float m_fRadius;
  bool m_bSpawnOnSurface;
  bool m_bSetVelocity;
  plVarianceTypeFloat m_Speed;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamPosition;
  plProcessingStream* m_pStreamVelocity;
};
