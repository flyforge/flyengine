#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

struct plMsgExtractRenderData;

/// \brief A particle system stores all data for one 'layer' of a running particle effect
class PLASMA_PARTICLEPLUGIN_DLL plParticleSystemInstance
{
public:
  plParticleSystemInstance();

  void Construct(plUInt32 uiMaxParticles, plWorld* pWorld, plParticleEffectInstance* pOwnerEffect, float fSpawnCountMultiplier);
  void Destruct();

  bool IsVisible() const { return m_bVisible; }

  void SetEmitterEnabled(bool enable) { m_bEmitterEnabled = enable; }
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  void ConfigureFromTemplate(const plParticleSystemDescriptor* pTemplate);
  void Finalize();

  void ReinitializeStreamProcessors(const plParticleSystemDescriptor* pTemplate);

  void CreateStreamProcessors(const plParticleSystemDescriptor* pTemplate);

  void SetupOptionalStreams();

  void SetTransform(const plTransform& transform, const plVec3& vParticleStartVelocity);
  const plTransform& GetTransform() const { return m_Transform; }
  const plVec3& GetParticleStartVelocity() const { return m_vParticleStartVelocity; }

  plParticleSystemState::Enum Update(const plTime& tDiff);

  plWorld* GetWorld() const { return m_pWorld; }

  plUInt64 GetMaxParticles() const { return m_StreamGroup.GetNumElements(); }
  plUInt64 GetNumActiveParticles() const { return m_StreamGroup.GetNumActiveElements(); }



  /// \brief Returns the desired stream, if it already exists, nullptr otherwise.
  plProcessingStream* QueryStream(const char* szName, plProcessingStream::DataType Type) const;

  /// \brief Returns the desired stream, if it already exists, creates it otherwise.
  void CreateStream(const char* szName, plProcessingStream::DataType Type, plProcessingStream** ppStream, plParticleStreamBinding& binding, bool bExpectInitializedValue);

  void ProcessEventQueue(plParticleEventQueue queue);

  plParticleEffectInstance* GetOwnerEffect() const { return m_pOwnerEffect; }
  plParticleWorldModule* GetOwnerWorldModule() const;

  void ExtractSystemRenderData(plMsgExtractRenderData& msg, const plTransform& instanceTransform) const;

  typedef plEvent<const plStreamGroupElementRemovedEvent&>::Handler ParticleDeathHandler;

  void AddParticleDeathEventHandler(ParticleDeathHandler handler);
  void RemoveParticleDeathEventHandler(ParticleDeathHandler handler);

  void SetBoundingVolume(const plBoundingBoxSphere& volume, float fMaxParticleSize);
  const plBoundingBoxSphere& GetBoundingVolume() const { return m_BoundingVolume; }

  bool IsContinuous() const;

  float GetSpawnCountMultiplier() const { return m_fSpawnCountMultiplier; }

private:
  bool IsEmitterConfigEqual(const plParticleSystemDescriptor* pTemplate) const;
  bool IsInitializerConfigEqual(const plParticleSystemDescriptor* pTemplate) const;
  bool IsBehaviorConfigEqual(const plParticleSystemDescriptor* pTemplate) const;
  bool IsTypeConfigEqual(const plParticleSystemDescriptor* pTemplate) const;
  bool IsFinalizerConfigEqual(const plParticleSystemDescriptor* pTemplate) const;

  void CreateStreamZeroInitializers();

  plHybridArray<plParticleEmitter*, 2> m_Emitters;
  plHybridArray<plParticleInitializer*, 6> m_Initializers;
  plHybridArray<plParticleBehavior*, 6> m_Behaviors;
  plHybridArray<plParticleFinalizer*, 2> m_Finalizers;
  plHybridArray<plParticleType*, 2> m_Types;

  bool m_bVisible; // typically used in editor to hide a system
  bool m_bEmitterEnabled;
  plParticleEffectInstance* m_pOwnerEffect;
  plWorld* m_pWorld;
  plTransform m_Transform;
  plVec3 m_vParticleStartVelocity;
  float m_fSpawnCountMultiplier = 1.0f;

  plProcessingStreamGroup m_StreamGroup;

  struct StreamInfo
  {
    plString m_sName;
    bool m_bGetsInitialized = false;
    bool m_bInUse = false;
    plProcessingStreamProcessor* m_pDefaultInitializer = nullptr;
  };

  plHybridArray<StreamInfo, 16> m_StreamInfo;

  // culling data
  plBoundingBoxSphere m_BoundingVolume;
};
