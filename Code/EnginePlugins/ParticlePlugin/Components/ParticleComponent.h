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

class PLASMA_PARTICLEPLUGIN_DLL plParticleComponentManager final : public plComponentManager<class plParticleComponent, plBlockStorageType::Compact>
{
  using SUPER = plComponentManager<class plParticleComponent, plBlockStorageType::Compact>;

public:
  plParticleComponentManager(plWorld* pWorld);

  virtual void Initialize() override;

  void Update(const plWorldModule::UpdateContext& context);

  void UpdatePfxTransformsAndBounds();
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleComponent final : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plParticleComponent, plRenderComponent, plParticleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg) override;


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

  void OnMsgSetPlaying(plMsgSetPlaying& msg); // [ msg handler ]

  void SetParticleEffect(const plParticleEffectResourceHandle& hEffect);
  PLASMA_ALWAYS_INLINE const plParticleEffectResourceHandle& GetParticleEffect() const { return m_hEffectResource; }

  void SetParticleEffectFile(const char* szFile); // [ property ]
  const char* GetParticleEffectFile() const;      // [ property ]

  // Exposed Parameters
  const plRangeView<const char*, plUInt32> GetParameters() const;   // [ property ]
  void SetParameter(const char* szKey, const plVariant& value);     // [ property ]
  void RemoveParameter(const char* szKey);                          // [ property ]
  bool GetParameter(const char* szKey, plVariant& out_value) const; // [ property ]

  plUInt64 m_uiRandomSeed = 0;    // [ property ]
  plString m_sSharedInstanceName; // [ property ]

  bool m_bSpawnAtStart = true;                                   // [ property ]
  bool m_bIfContinuousStopRightAway = false;                     // [ property ]
  bool m_bIgnoreOwnerRotation = false;                           // [ property ]
  plEnum<plOnComponentFinishedAction2> m_OnFinishedAction;       // [ property ]
  plTime m_MinRestartDelay;                                      // [ property ]
  plTime m_RestartDelayRange;                                    // [ property ]
  plEnum<plBasisAxis> m_SpawnDirection = plBasisAxis::PositiveZ; // [ property ]

  plParticleEffectController m_EffectController;

protected:
  void Update();
  plTransform GetPfxTransform() const;
  void UpdatePfxTransform();

  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  void OnMsgDeleteGameObject(plMsgDeleteGameObject& msg);

  virtual void OnDeactivated() override;

  plParticleEffectResourceHandle m_hEffectResource;
  plTime m_RestartTime;

  // Exposed Parameters
  friend class plParticleEventReaction_Effect;
  bool m_bFloatParamsChanged = false;
  bool m_bColorParamsChanged = false;
  plHybridArray<plParticleEffectFloatParam, 2> m_FloatParams;
  plHybridArray<plParticleEffectColorParam, 2> m_ColorParams;
};
