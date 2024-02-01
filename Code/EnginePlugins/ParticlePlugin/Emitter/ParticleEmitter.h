#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class plParticleSystemInstance;
class plProcessingStream;
class plParticleEmitter;

/// \brief Base class for all particle emitters
class PL_PARTICLEPLUGIN_DLL plParticleEmitterFactory : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEmitterFactory, plReflectedClass);

public:
  virtual const plRTTI* GetEmitterType() const = 0;
  virtual void CopyEmitterProperties(plParticleEmitter* pEmitter, bool bFirstTime) const = 0;

  plParticleEmitter* CreateEmitter(plParticleSystemInstance* pOwner) const;
  virtual void QueryMaxParticleCount(plUInt32& out_uiMaxParticlesAbs, plUInt32& out_uiMaxParticlesPerSecond) const = 0;

  virtual void Save(plStreamWriter& inout_stream) const = 0;
  virtual void Load(plStreamReader& inout_stream) = 0;
};

enum class plParticleEmitterState
{
  Active,
  Finished,
  OnlyReacting, //< Doesn't do anything, unless there are events that trigger it. That means it is considered finished, when all other emitters are
                //finished.
};

/// \brief Base class for stream spawners that are used by plParticleEmitter's
class PL_PARTICLEPLUGIN_DLL plParticleEmitter : public plParticleModule
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEmitter, plParticleModule);

  friend class plParticleSystemInstance;
  friend class plParticleEmitterFactory;

protected:
  virtual bool IsContinuous() const;
  virtual void Process(plUInt64 uiNumElements) final override;

  /// \brief Called once per update. Must return how many new particles are to be spawned.
  virtual plUInt32 ComputeSpawnCount(const plTime& tDiff) = 0;

  /// \brief Called before ComputeSpawnCount(). Should return true, if the emitter will never spawn any more particles.
  virtual plParticleEmitterState IsFinished() = 0;

  virtual void ProcessEventQueue(plParticleEventQueue queue);
};
