#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

using plCurve1DResourceHandle = plTypedResourceHandle<class plCurve1DResource>;

class PLASMA_PARTICLEPLUGIN_DLL plParticleEmitterFactory_Continuous final : public plParticleEmitterFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleEmitterFactory_Continuous, plParticleEmitterFactory);

public:
  plParticleEmitterFactory_Continuous();

  virtual const plRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(plParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void QueryMaxParticleCount(plUInt32& out_uiMaxParticlesAbs, plUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

public:
  plTime m_StartDelay;

  plUInt32 m_uiSpawnCountPerSec;
  plUInt32 m_uiSpawnCountPerSecRange;
  plString m_sSpawnCountScaleParameter;

  plCurve1DResourceHandle m_hCountCurve;
  plTime m_CurveDuration;

  void SetCountCurveFile(const char* szFile);
  const char* GetCountCurveFile() const;
};


class PLASMA_PARTICLEPLUGIN_DLL plParticleEmitter_Continuous final : public plParticleEmitter
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleEmitter_Continuous, plParticleEmitter);

public:
  plTime m_StartDelay; // delay before the emitter becomes active, to sync with other systems, only used once, has no effect later on

  plUInt32 m_uiSpawnCountPerSec;
  plUInt32 m_uiSpawnCountPerSecRange;
  plTempHashedString m_sSpawnCountScaleParameter;

  plCurve1DResourceHandle m_hCountCurve;
  plTime m_CurveDuration;


  virtual void CreateRequiredStreams() override {}

protected:
  virtual bool IsContinuous() const override { return true; }

  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override {}
  virtual void OnFinalize() override;

  virtual plParticleEmitterState IsFinished() override;
  virtual plUInt32 ComputeSpawnCount(const plTime& tDiff) override;

  plTime m_CountCurveTime;
  plTime m_TimeSinceRandom;
  float m_fCurSpawnPerSec;
  float m_fCurSpawnCounter;
};
