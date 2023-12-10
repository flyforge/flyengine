#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Containers/IdTable.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

using plParticleEffectResourceHandle = plTypedResourceHandle<class plParticleEffectResource>;
class plParticleEffectInstance;
struct plResourceEvent;
class plTaskGroupID;
class plParticleStream;
class plParticleStreamFactory;

/// \brief This world module stores all particle effect data that is active in a given plWorld instance
///
/// It is used to update all effects in one world and also to render them.
/// When an effect is stopped, it only stops emitting new particles, but it lives on until all particles are dead.
/// Therefore particle effects need to be managed outside of components. When a component dies, it only tells the
/// world module to 'destroy' it's effect, the rest is handled behind the scenes.
class PLASMA_PARTICLEPLUGIN_DLL plParticleWorldModule final : public plWorldModule
{
  PLASMA_DECLARE_WORLD_MODULE();
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleWorldModule, plWorldModule);

public:
  plParticleWorldModule(plWorld* pWorld);
  ~plParticleWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  plParticleEffectHandle CreateEffectInstance(const plParticleEffectResourceHandle& hResource, plUInt64 uiRandomSeed, const char* szSharedName /*= nullptr*/, const void*& inout_pSharedInstanceOwner, plArrayPtr<plParticleEffectFloatParam> floatParams, plArrayPtr<plParticleEffectColorParam> colorParams);

  /// \brief This does not actually the effect, it first stops it from emitting and destroys it once all particles have actually died of old age.
  void DestroyEffectInstance(const plParticleEffectHandle& hEffect, bool bInterruptImmediately, const void* pSharedInstanceOwner);

  bool TryGetEffectInstance(const plParticleEffectHandle& hEffect, plParticleEffectInstance*& out_pEffect);
  bool TryGetEffectInstance(const plParticleEffectHandle& hEffect, const plParticleEffectInstance*& out_pEffect) const;

  /// \brief Extracts render data for the given effect.
  void ExtractEffectRenderData(const plParticleEffectInstance* pEffect, plMsgExtractRenderData& msg, const plTransform& systemTransform) const;

  plParticleSystemInstance* CreateSystemInstance(plUInt32 uiMaxParticles, plWorld* pWorld, plParticleEffectInstance* pOwnerEffect, float fSpawnMultiplier);
  void DestroySystemInstance(plParticleSystemInstance* pInstance);

  plParticleStream* CreateStreamDefaultInitializer(plParticleSystemInstance* pOwner, const char* szFullStreamName) const;

  /// \brief Can be called at any time (e.g. during plParticleBehaviorFactory::CopyBehaviorProperties()) to query a previously cached world module,
  /// even if that happens on a thread which would not be allowed to query this from the plWorld at that time.
  plWorldModule* GetCachedWorldModule(const plRTTI* pRtti) const;

  /// \brief Should be called by plParticleModule::RequestRequiredWorldModulesForCache() to cache a pointer to a world module that is needed later.
  template <class T>
  void CacheWorldModule()
  {
    CacheWorldModule(plGetStaticRTTI<T>());
  }

  /// \brief Should be called by plParticleModule::RequestRequiredWorldModulesForCache() to cache a pointer to a world module that is needed later.
  void CacheWorldModule(const plRTTI* pRtti);

private:
  virtual void WorldClear() override;

  void UpdateEffects(const plWorldModule::UpdateContext& context);
  void EnsureUpdatesFinished(const plWorldModule::UpdateContext& context);

  void DestroyFinishedEffects();
  void CreateFinisherComponent(plParticleEffectInstance* pEffect);
  void ResourceEventHandler(const plResourceEvent& e);
  void ReconfigureEffects();
  plParticleEffectHandle InternalCreateSharedEffectInstance(const char* szSharedName, const plParticleEffectResourceHandle& hResource, plUInt64 uiRandomSeed, const void* pSharedInstanceOwner);
  plParticleEffectHandle InternalCreateEffectInstance(const plParticleEffectResourceHandle& hResource, plUInt64 uiRandomSeed, bool bIsShared, plArrayPtr<plParticleEffectFloatParam> floatParams, plArrayPtr<plParticleEffectColorParam> colorParams);

  void ConfigureParticleStreamFactories();
  void ClearParticleStreamFactories();

  mutable plMutex m_Mutex;
  plDeque<plParticleEffectInstance> m_ParticleEffects;
  plDynamicArray<plParticleEffectInstance*> m_FinishingEffects;
  plDynamicArray<plParticleEffectInstance*> m_NeedFinisherComponent;
  plDynamicArray<plParticleEffectInstance*> m_EffectsToReconfigure;
  plDynamicArray<plParticleEffectInstance*> m_ParticleEffectsFreeList;
  plMap<plString, plParticleEffectHandle> m_SharedEffects;
  plIdTable<plParticleEffectId, plParticleEffectInstance*> m_ActiveEffects;
  plDeque<plParticleSystemInstance> m_ParticleSystems;
  plDynamicArray<plParticleSystemInstance*> m_ParticleSystemFreeList;
  plTaskGroupID m_EffectUpdateTaskGroup;
  plMap<plString, plParticleStreamFactory*> m_StreamFactories;
  plHashTable<const plRTTI*, plWorldModule*> m_WorldModuleCache;
};
