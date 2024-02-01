#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

class PL_PARTICLEPLUGIN_DLL plParticleEmitterFactory_Distance final : public plParticleEmitterFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEmitterFactory_Distance, plParticleEmitterFactory);

public:
  plParticleEmitterFactory_Distance();

  virtual const plRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(plParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void QueryMaxParticleCount(plUInt32& out_uiMaxParticlesAbs, plUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

public:
  float m_fDistanceThreshold = 0.1f;
  plUInt32 m_uiSpawnCountMin = 1;
  plUInt32 m_uiSpawnCountRange = 0;
  plString m_sSpawnCountScaleParameter;
};


class PL_PARTICLEPLUGIN_DLL plParticleEmitter_Distance final : public plParticleEmitter
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEmitter_Distance, plParticleEmitter);

public:
  float m_fDistanceThresholdSQR;
  plUInt32 m_uiSpawnCountMin;
  plUInt32 m_uiSpawnCountRange;
  plTempHashedString m_sSpawnCountScaleParameter;

  virtual void CreateRequiredStreams() override;

protected:
  virtual bool IsContinuous() const override;
  virtual void OnFinalize() override;
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;

  virtual plParticleEmitterState IsFinished() override;
  virtual plUInt32 ComputeSpawnCount(const plTime& tDiff) override;

  bool m_bFirstUpdate = true;
  plVec3 m_vLastSpawnPosition;
};
