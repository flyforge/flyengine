#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

class PL_PARTICLEPLUGIN_DLL plParticleEmitterFactory_Burst final : public plParticleEmitterFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEmitterFactory_Burst, plParticleEmitterFactory);

public:
  plParticleEmitterFactory_Burst();

  virtual const plRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(plParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void QueryMaxParticleCount(plUInt32& out_uiMaxParticlesAbs, plUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

public:
  plTime m_Duration;
  plTime m_StartDelay;

  plUInt32 m_uiSpawnCountMin;
  plUInt32 m_uiSpawnCountRange;
  plString m_sSpawnCountScaleParameter;
};


class PL_PARTICLEPLUGIN_DLL plParticleEmitter_Burst final : public plParticleEmitter
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEmitter_Burst, plParticleEmitter);

public:
  plTime m_Duration;   // overall duration in which the emitter is considered active, 0 for single frame
  plTime m_StartDelay; // delay before the emitter becomes active, to sync with other systems, only used once, has no effect later on

  plUInt32 m_uiSpawnCountMin;
  plUInt32 m_uiSpawnCountRange;
  plTempHashedString m_sSpawnCountScaleParameter;

  virtual void CreateRequiredStreams() override {}

protected:
  virtual void OnFinalize() override;
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override {}

  virtual plParticleEmitterState IsFinished() override;
  virtual plUInt32 ComputeSpawnCount(const plTime& tDiff) override;

  plUInt32 m_uiSpawnCountLeft = 0;
  float m_fSpawnPerSecond = 0;
  float m_fSpawnAccu = 0;
};
