#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

class PLASMA_PARTICLEPLUGIN_DLL plParticleInitializerFactory_VelocityCone final : public plParticleInitializerFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleInitializerFactory_VelocityCone, plParticleInitializerFactory);

public:
  plParticleInitializerFactory_VelocityCone();

  virtual const plRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(plParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

  virtual void QueryFinalizerDependencies(plSet<const plRTTI*>& inout_FinalizerDeps) const override;

public:
  plAngle m_Angle;
  plVarianceTypeFloat m_Speed;
};


class PLASMA_PARTICLEPLUGIN_DLL plParticleInitializer_VelocityCone final : public plParticleInitializer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleInitializer_VelocityCone, plParticleInitializer);

public:
  plAngle m_Angle;
  plVarianceTypeFloat m_Speed;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamVelocity;
};
