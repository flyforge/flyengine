#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

class PLASMA_PARTICLEPLUGIN_DLL plParticleInitializerFactory_CylinderPosition final : public plParticleInitializerFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleInitializerFactory_CylinderPosition, plParticleInitializerFactory);

public:
  plParticleInitializerFactory_CylinderPosition();

  virtual const plRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(plParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float GetSpawnCountMultiplier(const plParticleEffectInstance* pEffect) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

  virtual void QueryFinalizerDependencies(plSet<const plRTTI*>& inout_FinalizerDeps) const override;

public:
  plVec3 m_vPositionOffset;
  float m_fRadius;
  float m_fHeight;
  bool m_bSpawnOnSurface;
  bool m_bSetVelocity;
  plVarianceTypeFloat m_Speed;
  plString m_sScaleRadiusParameter;
  plString m_sScaleHeightParameter;
};


class PLASMA_PARTICLEPLUGIN_DLL plParticleInitializer_CylinderPosition final : public plParticleInitializer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleInitializer_CylinderPosition, plParticleInitializer);

public:
  plVec3 m_vPositionOffset;
  float m_fRadius;
  float m_fHeight;
  bool m_bSpawnOnSurface;
  bool m_bSetVelocity;
  plVarianceTypeFloat m_Speed;

protected:
  virtual void CreateRequiredStreams() override;
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamPosition;
  plProcessingStream* m_pStreamVelocity;
};
