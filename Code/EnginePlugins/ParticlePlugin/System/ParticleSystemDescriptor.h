#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class plParticleEmitterFactory;
class plParticleBehaviorFactory;
class plParticleInitializerFactory;
class plParticleTypeFactory;

class PLASMA_PARTICLEPLUGIN_DLL plParticleSystemDescriptor final : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleSystemDescriptor, plReflectedClass);

public:
  plParticleSystemDescriptor();
  ~plParticleSystemDescriptor();

  //////////////////////////////////////////////////////////////////////////
  /// Properties

  const plHybridArray<plParticleEmitterFactory*, 1>& GetEmitterFactories() const { return m_EmitterFactories; }

  void AddInitializerFactory(plParticleInitializerFactory* pFactory) { m_InitializerFactories.PushBack(pFactory); }
  void RemoveInitializerFactory(plParticleInitializerFactory* pFactory) { m_InitializerFactories.RemoveAndCopy(pFactory); }
  const plHybridArray<plParticleInitializerFactory*, 4>& GetInitializerFactories() const { return m_InitializerFactories; }

  void AddBehaviorFactory(plParticleBehaviorFactory* pFactory) { m_BehaviorFactories.PushBack(pFactory); }
  void RemoveBehaviorFactory(plParticleBehaviorFactory* pFactory) { m_BehaviorFactories.RemoveAndCopy(pFactory); }
  const plHybridArray<plParticleBehaviorFactory*, 4>& GetBehaviorFactories() const { return m_BehaviorFactories; }

  void AddTypeFactory(plParticleTypeFactory* pFactory) { m_TypeFactories.PushBack(pFactory); }
  void RemoveTypeFactory(plParticleTypeFactory* pFactory) { m_TypeFactories.RemoveAndCopy(pFactory); }
  const plHybridArray<plParticleTypeFactory*, 2>& GetTypeFactories() const { return m_TypeFactories; }

  const plHybridArray<plParticleFinalizerFactory*, 2>& GetFinalizerFactories() const { return m_FinalizerFactories; }

  plTime GetAvgLifetime() const;

  bool m_bVisible;

  plVarianceTypeTime m_LifeTime;
  plString m_sOnDeathEvent;
  plString m_sLifeScaleParameter;

  //////////////////////////////////////////////////////////////////////////

  void Save(plStreamWriter& stream) const;
  void Load(plStreamReader& stream);

private:
  void ClearEmitters();
  void ClearInitializers();
  void ClearBehaviors();
  void ClearTypes();
  void ClearFinalizers();
  void SetupDefaultProcessors();

  plString m_sName;
  plHybridArray<plParticleEmitterFactory*, 1> m_EmitterFactories;
  plHybridArray<plParticleInitializerFactory*, 4> m_InitializerFactories;
  plHybridArray<plParticleBehaviorFactory*, 4> m_BehaviorFactories;
  plHybridArray<plParticleFinalizerFactory*, 2> m_FinalizerFactories;
  plHybridArray<plParticleTypeFactory*, 2> m_TypeFactories;
};
