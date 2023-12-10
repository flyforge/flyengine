#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/Deque.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Events/ParticleEvent.h>

class PLASMA_PARTICLEPLUGIN_DLL plParticleEmitterFactory_OnEvent final : public plParticleEmitterFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleEmitterFactory_OnEvent, plParticleEmitterFactory);

public:
  plParticleEmitterFactory_OnEvent();
  ~plParticleEmitterFactory_OnEvent();

  virtual const plRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(plParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void QueryMaxParticleCount(plUInt32& out_uiMaxParticlesAbs, plUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

  plString m_sEventName;
  plUInt32 m_uiSpawnCountMin = 1;
  plUInt32 m_uiSpawnCountRange = 0;
  plString m_sSpawnCountScaleParameter;
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleEmitter_OnEvent final : public plParticleEmitter
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleEmitter_OnEvent, plParticleEmitter);

public:
  plTempHashedString m_sEventName;
  plUInt32 m_uiSpawnCountMin = 1;
  plUInt32 m_uiSpawnCountRange = 0;
  plTempHashedString m_sSpawnCountScaleParameter;

  virtual void CreateRequiredStreams() override {}

protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override {}

  virtual plParticleEmitterState IsFinished() override;
  virtual plUInt32 ComputeSpawnCount(const plTime& tDiff) override;

  virtual void ProcessEventQueue(plParticleEventQueue queue) override;

  bool m_bSpawn = false;
};
