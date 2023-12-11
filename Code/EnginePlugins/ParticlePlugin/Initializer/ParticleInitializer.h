#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class plParticleSystemInstance;
class plProcessingStream;
class plParticleInitializer;
class plParticleEffectInstance;

/// \brief Base class for all particle emitters
class PLASMA_PARTICLEPLUGIN_DLL plParticleInitializerFactory : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleInitializerFactory, plReflectedClass);

public:
  virtual const plRTTI* GetInitializerType() const = 0;
  virtual void CopyInitializerProperties(plParticleInitializer* pInitializer, bool bFirstTime) const = 0;
  virtual float GetSpawnCountMultiplier(const plParticleEffectInstance* pEffect) const;

  plParticleInitializer* CreateInitializer(plParticleSystemInstance* pOwner) const;

  virtual void Save(plStreamWriter& stream) const = 0;
  virtual void Load(plStreamReader& stream) = 0;

  virtual void QueryFinalizerDependencies(plSet<const plRTTI*>& inout_FinalizerDeps) const {}
};

/// \brief Base class for stream spawners that are used by plParticleEmitter's
class PLASMA_PARTICLEPLUGIN_DLL plParticleInitializer : public plParticleModule
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleInitializer, plParticleModule);

  friend class plParticleSystemInstance;
  friend class plParticleInitializerFactory;

protected:
  plParticleInitializer();

  virtual void Process(plUInt64 uiNumElements) final override {}
};