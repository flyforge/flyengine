#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Components/RenderComponent.h>

class plParticleRenderData;
struct plMsgUpdateLocalBounds;
struct plMsgExtractRenderData;
class plParticleSystemInstance;
class plParticleComponent;
struct plMsgSetPlaying;

using plParticleEffectResourceHandle = plTypedResourceHandle<class plParticleEffectResource>;

class PL_PARTICLEPLUGIN_DLL plParticleComponentManager final : public plComponentManager<class plParticleComponent, plBlockStorageType::Compact>
{
  using SUPER = plComponentManager<class plParticleComponent, plBlockStorageType::Compact>;

public:
  plParticleComponentManager(plWorld* pWorld);

  virtual void Initialize() override;

  void Update(const plWorldModule::UpdateContext& context);

  void UpdatePfxTransformsAndBounds();
};

/// \brief Plays a particle effect at the location of the game object.
class PL_PARTICLEPLUGIN_DLL plParticleComponent final : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plParticleComponent, plRenderComponent, plParticleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // plParticleComponent

public:
  plParticleComponent();
  ~plParticleComponent();

  /// \brief Starts a new particle effect. If one is already running, it will be stopped (but not interrupted) and a new one is started as
  /// well.
  ///
  /// Returns false, if no valid particle resource is specified.
  bool StartEffect(); // [ scriptable ]

  /// \brief Stops emitting further particles, making any existing particle system stop in a finite amount of time.
  void StopEffect(); // [ scriptable ]

  /// \brief Cancels the entire effect immediately, it will pop out of existence.
  void InterruptEffect(); // [ scriptable ]

  /// \brief Returns true, if an effect is currently in a state where it might emit new particles
  bool IsEffectActive() const; // [ scriptable ]

  /// \brief Forwards to StartEffect() or StopEffect().
  void OnMsgSetPlaying(plMsgSetPlaying& ref_msg); // [ msg handler ]

  /// \brief Replaces the effect to be played.
  void SetParticleEffect(const plParticleEffectResourceHandle& hEffect);
  PL_ALWAYS_INLINE const plParticleEffectResourceHandle& GetParticleEffect() const { return m_hEffectResource; }

  void SetParticleEffectFile(const char* szFile); // [ property ]
  const char* GetParticleEffectFile() const;      // [ property ]

  // Exposed Parameters
  const plRangeView<const char*, plUInt32> GetParameters() const;   // [ property ]
  void SetParameter(const char* szKey, const plVariant& value);     // [ property ]
  void RemoveParameter(const char* szKey);                          // [ property ]
  bool GetParameter(const char* szKey, plVariant& out_value) const; // [ property ]

  /// \brief If zero, the played effect is randomized each time. Use a fixed seed when the result should be deterministic.
  plUInt64 m_uiRandomSeed = 0; // [ property ]

  /// \brief If set, the component reuses the simulation state of another particle component with the same name.
  ///
  /// This can be used to reuse similar effects, for example smoke on chimneys doesn't need to be unique.
  /// Each instance renders the effect from its own perspective, but the simulation is only done once.
  /// This only makes sense for infinite, ambient effects.
  plString m_sSharedInstanceName; // [ property ]

  /// \brief If false, the effect starts in a paused state.
  bool m_bSpawnAtStart = true; // [ property ]

  /// \brief If true, the owner rotation is assumed to be identity. Useful for effects that need to always point in one direction (e.g. up).
  bool m_bIgnoreOwnerRotation = false; // [ property ]

  /// \brief What to do when the effect is finished playing.
  plEnum<plOnComponentFinishedAction2> m_OnFinishedAction; // [ property ]

  /// \brief Minimum delay between finishing and restarting.
  plTime m_MinRestartDelay; // [ property ]

  /// \brief Random additional delay between finishing and restarting.
  plTime m_RestartDelayRange; // [ property ]

  /// \brief The local direction into which to spawn the effect.
  plEnum<plBasisAxis> m_SpawnDirection = plBasisAxis::PositiveZ; // [ property ]

  /// \brief Allows more fine grain control over the effect execution.
  plParticleEffectController m_EffectController;

protected:
  void Update();
  plTransform GetPfxTransform() const;
  void UpdatePfxTransform();

  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  void OnMsgDeleteGameObject(plMsgDeleteGameObject& msg);

  plParticleEffectResourceHandle m_hEffectResource;

  plTime m_RestartTime;

  // Exposed Parameters
  friend class plParticleEventReaction_Effect;
  bool m_bIfContinuousStopRightAway = false;
  bool m_bFloatParamsChanged = false;
  bool m_bColorParamsChanged = false;
  plHybridArray<plParticleEffectFloatParam, 2> m_FloatParams;
  plHybridArray<plParticleEffectColorParam, 2> m_ColorParams;
};
