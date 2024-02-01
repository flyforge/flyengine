#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class plProcessingStream;
class plParticleSystemInstance;
class plParticleFinalizer;

/// \brief Base class for all particle Finalizers
class PL_PARTICLEPLUGIN_DLL plParticleFinalizerFactory : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleFinalizerFactory, plReflectedClass);

public:
  virtual const plRTTI* GetFinalizerType() const = 0;
  virtual void CopyFinalizerProperties(plParticleFinalizer* pObject, bool bFirstTime) const = 0;

  plParticleFinalizer* CreateFinalizer(plParticleSystemInstance* pOwner) const;
};

class PL_PARTICLEPLUGIN_DLL plParticleFinalizer : public plParticleModule
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleFinalizer, plParticleModule);

  friend class plParticleSystemInstance;

protected:
  plParticleFinalizer();
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const plTime& tDiff, plUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  plTime m_TimeDiff;
};
