#pragma once

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/SharedPtr.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

class plParticleEffectInstance;

class plParticleEffectUpdateTask final : public plTask
{
public:
  plParticleEffectUpdateTask(plParticleEffectInstance* pEffect);

  plTime m_UpdateDiff;

private:
  virtual void Execute() override;

  plParticleEffectInstance* m_pEffect;
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleEffectInstance
{
  friend class plParticleWorldModule;
  friend class plParticleEffectUpdateTask;

public:
  plParticleEffectInstance();
  ~plParticleEffectInstance();

  void Construct(plParticleEffectHandle hEffectHandle, const plParticleEffectResourceHandle& hResource, plWorld* pWorld, plParticleWorldModule* pOwnerModule, plUInt64 uiRandomSeed, bool bIsShared, plArrayPtr<plParticleEffectFloatParam> floatParams, plArrayPtr<plParticleEffectColorParam> colorParams);
  void Destruct();

  void Interrupt();

  const plParticleEffectHandle& GetHandle() const { return m_hEffectHandle; }

  void SetEmitterEnabled(bool enable);
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  void ClearParticleSystems();
  void ClearEventReactions();

  bool IsContinuous() const;

  plWorld* GetWorld() const { return m_pWorld; }
  plParticleWorldModule* GetOwnerWorldModule() const { return m_pOwnerModule; }

  const plParticleEffectResourceHandle& GetResource() const { return m_hResource; }

  const plHybridArray<plParticleSystemInstance*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

  void AddParticleEvent(const plParticleEvent& pe);

  plRandom& GetRNG() { return m_Random; }

  plUInt64 GetRandomSeed() const { return m_uiRandomSeed; }

  void UpdateWindSamples();

  /// \brief Returns the number of currently active particles across all systems.
  plUInt64 GetNumActiveParticles() const;

  /// @name Transform Related
  /// @{
public:
  /// \brief Whether the effect is simulated around the origin and thus not affected by instance position and rotation
  bool IsSimulatedInLocalSpace() const { return m_bSimulateInLocalSpace; }

  /// \brief Sets the transformation of this instance
  void SetTransform(const plTransform& transform, const plVec3& vParticleStartVelocity);

  /// \brief Sets the transformation of this instance that should be used next frame.
  /// This function is typically used to set the transformation while the particle simulation is running to prevent race conditions.
  void SetTransformForNextFrame(const plTransform& transform, const plVec3& vParticleStartVelocity);

  /// \brief Returns the transform of the main or shared instance.
  const plTransform& GetTransform() const { return m_Transform; }

  /// \brief For the renderer to know whether the instance transform has to be applied to each particle position.
  bool NeedsToApplyTransform() const { return m_bSimulateInLocalSpace || m_bIsSharedEffect; }

  /// \brief Adds a location where the wind system should be sampled.
  ///
  /// Particle behaviors can't sample the wind system directly, because they are updated in parallel and this can
  /// cause crashes. Therefore the desired sample locations have to be cached.
  /// In the next frame, the result can be retrieved via GetWindSampleResult() with the returned index.
  ///
  /// Only a very limited amount of locations can be sampled (4) across all behaviors.
  plInt32 AddWindSampleLocation(const plVec3& pos);

  /// \brief Returns the wind result sampled at the previously specified location (see AddWindSampleLocation()).
  ///
  /// Returns a zero vector, if no wind value is available (invalid index).
  plVec3 GetWindSampleResult(plInt32 idx) const;

private:
  void PassTransformToSystems();

  plTransform m_Transform;
  plTransform m_TransformForNextFrame;

  plVec3 m_vVelocity;
  plVec3 m_vVelocityForNextFrame;

  plStaticArray<plVec3, 4> m_vSampleWindLocations[2];
  plStaticArray<plVec3, 4> m_vSampleWindResults[2];

  /// @}
  /// @name Updates
  /// @{

public:
  /// \brief Returns false when the effect is finished.
  bool Update(const plTime& tDiff);

  /// \brief Returns the total (game) time that the effect is alive and has been updated.
  ///
  /// Use this time, instead of a world clock, for time-dependent calculations. It is mostly tied to the world clock (game update),
  /// but additionally includes pre-simulation timings, which would otherwise be left out which can break some calculations.
  plTime GetTotalEffectLifeTime() const { return m_TotalEffectLifeTime; }

private: // friend plParticleWorldModule
  /// \brief Whether this instance is in a state where its update task should be run
  bool ShouldBeUpdated() const;

  /// \brief Returns the task that is used to update the effect
  const plSharedPtr<plTask>& GetUpdateTask() { return m_pTask; }

private: // friend plParticleEffectUpdateTask
  friend class plParticleEffectController;
  /// \brief If the effect wants to skip all the initial behavior, this simulates it multiple times before it is shown the first time.
  void PreSimulate();

  /// \brief Applies a given time step, without any restrictions.
  bool StepSimulation(const plTime& tDiff);

private:
  plTime m_TotalEffectLifeTime = plTime::MakeZero();
  plTime m_ElapsedTimeSinceUpdate = plTime::MakeZero();


  /// @}
  /// @name Shared Instances
  /// @{
public:
  /// \brief Returns true, if this effect is configured to be simulated once per frame, but rendered by multiple instances.
  bool IsSharedEffect() const { return m_bIsSharedEffect; }

private: // friend plParticleWorldModule
  void AddSharedInstance(const void* pSharedInstanceOwner);
  void RemoveSharedInstance(const void* pSharedInstanceOwner);

private:
  bool m_bIsSharedEffect = false;

  /// @}
  /// \name Visibility and Culling
  /// @{
public:
  /// \brief Marks this effect as visible from at least one view.
  /// This affects simulation update rates.
  void SetIsVisible() const;

  void SetVisibleIf(plParticleEffectInstance* pOtherVisible);

  /// \brief Whether the effect has been marked as visible recently.
  bool IsVisible() const;

  /// \brief Returns the bounding volume of the effect.
  /// The volume is in the local space of the effect.
  void GetBoundingVolume(plBoundingBoxSphere& volume) const;

private:
  void CombineSystemBoundingVolumes();

  plBoundingBoxSphere m_BoundingVolume;
  mutable plTime m_EffectIsVisible;
  plParticleEffectInstance* m_pVisibleIf = nullptr;
  plEnum<plEffectInvisibleUpdateRate> m_InvisibleUpdateRate;
  plUInt64 m_uiRandomSeed = 0;

  /// @}
  /// \name Effect Parameters
  /// @{
public:
  void SetParameter(const plTempHashedString& name, float value);
  void SetParameter(const plTempHashedString& name, const plColor& value);

  plInt32 FindFloatParameter(const plTempHashedString& name) const;
  float GetFloatParameter(const plTempHashedString& name, float defaultValue) const;
  float GetFloatParameter(plUInt32 idx) const { return m_FloatParameters[idx].m_fValue; }

  plInt32 FindColorParameter(const plTempHashedString& name) const;
  const plColor& GetColorParameter(const plTempHashedString& name, const plColor& defaultValue) const;
  const plColor& GetColorParameter(plUInt32 idx) const { return m_ColorParameters[idx].m_Value; }


private:
  struct FloatParameter
  {
    PLASMA_DECLARE_POD_TYPE();
    plUInt64 m_uiNameHash;
    float m_fValue;
  };

  struct ColorParameter
  {
    PLASMA_DECLARE_POD_TYPE();
    plUInt64 m_uiNameHash;
    plColor m_Value;
  };

  plHybridArray<FloatParameter, 2> m_FloatParameters;
  plHybridArray<ColorParameter, 2> m_ColorParameters;

  /// @}


private:
  void Reconfigure(bool bFirstTime, plArrayPtr<plParticleEffectFloatParam> floatParams, plArrayPtr<plParticleEffectColorParam> colorParams);
  void ClearParticleSystem(plUInt32 index);
  void ProcessEventQueues();

  // for deterministic randomness
  plRandom m_Random;

  plHashSet<const void*> m_SharedInstances;
  plParticleEffectHandle m_hEffectHandle;
  bool m_bEmitterEnabled = true;
  bool m_bSimulateInLocalSpace = false;
  bool m_bIsFinishing = false;
  plUInt8 m_uiReviveTimeout = 3;
  plInt8 m_iMinSimStepsToDo = 0;
  float m_fApplyInstanceVelocity = 0;
  plTime m_PreSimulateDuration;
  plParticleEffectResourceHandle m_hResource;

  plParticleWorldModule* m_pOwnerModule = nullptr;
  plWorld* m_pWorld = nullptr;
  plHybridArray<plParticleSystemInstance*, 4> m_ParticleSystems;
  plHybridArray<plParticleEventReaction*, 4> m_EventReactions;

  plSharedPtr<plTask> m_pTask;

  plStaticArray<plParticleEvent, 16> m_EventQueue;
};
