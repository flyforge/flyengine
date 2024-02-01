#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class plProcessingStream;
class plParticleSystemInstance;
class plParticleBehavior;

/// \brief Base class for all particle behaviors
class PL_PARTICLEPLUGIN_DLL plParticleBehaviorFactory : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleBehaviorFactory, plReflectedClass);

public:
  virtual const plRTTI* GetBehaviorType() const = 0;
  virtual void CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const = 0;

  plParticleBehavior* CreateBehavior(plParticleSystemInstance* pOwner) const;

  virtual void Save(plStreamWriter& inout_stream) const = 0;
  virtual void Load(plStreamReader& inout_stream) = 0;

  virtual void QueryFinalizerDependencies(plSet<const plRTTI*>& inout_finalizerDeps) const {}
};

class PL_PARTICLEPLUGIN_DLL plParticleBehavior : public plParticleModule
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleBehavior, plParticleModule);

  friend class plParticleSystemInstance;

protected:
  plParticleBehavior();
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const plTime& tDiff, plUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  plTime m_TimeDiff;
};
