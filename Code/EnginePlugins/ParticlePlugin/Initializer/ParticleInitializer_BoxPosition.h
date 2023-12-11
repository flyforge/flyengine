#pragma once

#include <ParticlePlugin/Initializer/ParticleInitializer.h>

class PLASMA_PARTICLEPLUGIN_DLL plParticleInitializerFactory_BoxPosition final : public plParticleInitializerFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleInitializerFactory_BoxPosition, plParticleInitializerFactory);

public:
  plParticleInitializerFactory_BoxPosition();

  virtual const plRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(plParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float GetSpawnCountMultiplier(const plParticleEffectInstance* pEffect) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

public:
  plVec3 m_vPositionOffset;
  plVec3 m_vSize;
  plString m_sScaleXParameter;
  plString m_sScaleYParameter;
  plString m_sScaleZParameter;
};


class PLASMA_PARTICLEPLUGIN_DLL plParticleInitializer_BoxPosition final : public plParticleInitializer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleInitializer_BoxPosition, plParticleInitializer);

public:
  plVec3 m_vPositionOffset;
  plVec3 m_vSize;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamPosition;
};