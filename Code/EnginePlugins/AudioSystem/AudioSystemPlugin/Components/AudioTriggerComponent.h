#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>
#include <AudioSystemPlugin/Core/AudioSystemData.h>

class plPhysicsWorldModuleInterface;

constexpr plUInt32 k_MaxOcclusionRaysCount = 32;

class PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioTriggerComponentManager : public plComponentManager<class plAudioTriggerComponent, plBlockStorageType::FreeList>
{
public:
  explicit plAudioTriggerComponentManager(plWorld* pWorld);

  void Initialize() override;
  void Deinitialize() override;

private:
  friend class plAudioTriggerComponent;


  struct ObstructionOcclusionValue
  {
    /// \brief The new value target.
    float m_fTarget;

    /// \brief The current value.
    float m_fValue;

    /// \brief Gets the current value.
    PLASMA_NODISCARD float GetValue() const;

    /// \brief Sets the new value's target.
    /// \param fTarget The value's target.
    /// \param bReset Specifies if the value should be reset
    /// to the new target.
    void SetTarget(float fTarget, bool bReset = false);

    /// \brief Updates the current value by moving it to the target
    /// value with a fSmoothFactor speed.
    /// \param fSmoothFactor The smooth factor, if not defined, will
    /// use the default one from CVar.
    void Update(float fSmoothFactor = -1.0f);

    /// \brief Resets the value and target to the given one.
    /// \param fInitialValue The new initial value.
    void Reset(float fInitialValue = 0.0f);
  };

  struct ObstructionOcclusionState
  {
    /// \brief The audio trigger component owner of this state
    plAudioTriggerComponent* m_pComponent{nullptr};

    plUInt8 m_uiNextRayIndex{0};

    ObstructionOcclusionValue m_ObstructionValue;
    ObstructionOcclusionValue m_OcclusionValue;

    plStaticArray<float, k_MaxOcclusionRaysCount> m_ObstructionRaysValues;
  };

  plUInt32 AddObstructionOcclusionState(plAudioTriggerComponent* pComponent);
  void RemoveObstructionOcclusionState(plUInt32 uiIndex);
  PLASMA_NODISCARD const ObstructionOcclusionState& GetObstructionOcclusionState(plUInt32 uiIndex) const { return m_ObstructionOcclusionStates[uiIndex]; }

  void ShootOcclusionRays(ObstructionOcclusionState& state, plVec3 listenerPos, plUInt32 uiNumRays, const plPhysicsWorldModuleInterface* pPhysicsWorldModule);
  void CastRay(ObstructionOcclusionState& state, plVec3 sourcePos, plVec3 direction, plUInt8 collisionLayer, const plPhysicsWorldModuleInterface* pPhysicsWorldModule, plUInt32 rayIndex);
  void ProcessOcclusion(const plWorldModule::UpdateContext& context);
  void Update(const plWorldModule::UpdateContext& context);

  plDynamicArray<ObstructionOcclusionState> m_ObstructionOcclusionStates;
};

/// \brief Audio System Component that triggers an audio event.
///
/// This component takes as properties a mandatory play trigger an an optional stop trigger.
/// The user should specify the name of the triggers as defined by the available audio controls loaded in the audio system.
/// If the stop trigger is left empty, the component will send a StopEvent request to the audio system, that means
/// the event triggered by the play trigger should be stoppable that way.
///
/// The component also exposes a property that allows to access the internal state through scripting.
class PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioTriggerComponent : public plAudioSystemProxyDependentComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plAudioTriggerComponent, plAudioSystemProxyDependentComponent, plAudioTriggerComponentManager);

  // plComponent

public:
  void Initialize() override;
  void OnActivated() override;
  void OnSimulationStarted() override;
  void OnDeactivated() override;
  void Deinitialize() override;
  void SerializeComponent(plWorldWriter& stream) const override;
  void DeserializeComponent(plWorldReader& stream) override;

  // plAudioSystemComponent

private:
  void plAudioSystemComponentIsAbstract() override {}

  // plAudioTriggerComponent

public:
  plAudioTriggerComponent();
  ~plAudioTriggerComponent() override;

  /// \brief Sets the collision layer on which rays should hit when calculating
  /// obstruction and occlusion.
  void SetOcclusionCollisionLayer(plUInt8 uiCollisionLayer);

  /// \brief Gets the current occlusion collision layer.
  PLASMA_NODISCARD plUInt8 GetOcclusionCollisionLayer() const { return m_uiOcclusionCollisionLayer; }

  /// \brief Sets the name of the play trigger. If the provided name is the same than the
  /// current name, nothing will happen.
  ///
  /// When setting a new name, the current event will be stopped if playing, but the new event will
  /// not be triggered automatically.
  ///
  /// \param sName The name of the play trigger.
  void SetPlayTrigger(plString sName);

  /// \brief Gets the name of the current play trigger.
  PLASMA_NODISCARD const plString& GetPlayTrigger() const;

  /// \brief Sets the name of the stop trigger. If the provided name is the same than the
  /// current name, nothing will happen.
  ///
  /// \param sName The name of the stop trigger.
  void SetStopTrigger(plString sName);

  /// \brief Gets the name of the current stop trigger.
  PLASMA_NODISCARD const plString& GetStopTrigger() const;

  /// \brief Gets the internal state of the play trigger.
  /// \returns An plEnum with the value of the play trigger state.
  PLASMA_NODISCARD const plEnum<plAudioSystemTriggerState>& GetState() const;

  /// \brief Returns whether the play trigger is currently being loaded.
  /// \returns True if the play trigger is currently being loaded, false otherwise.
  PLASMA_NODISCARD bool IsLoading() const;

  /// \brief Returns whether the play trigger is ready to be activated.
  /// \returns True if the play trigger is ready to be activated, false otherwise.
  PLASMA_NODISCARD bool IsReady() const;

  /// \brief Returns whether the play trigger is being activated.
  /// \returns True if the play trigger is being activated, false otherwise.
  PLASMA_NODISCARD bool IsStarting() const;

  /// \brief Returns whether the play trigger has been activated and is currently playing.
  /// \returns True if the play trigger is currently playing, false otherwise.
  PLASMA_NODISCARD bool IsPlaying() const;

  /// \brief Returns whether the event is being stopped, either by activating the stop trigger
  /// if defined, or by stopping the event directly.
  /// \returns True if the event is being stopped, false otherwise.
  PLASMA_NODISCARD bool IsStopping() const;

  /// \brief Returns whether the play trigger has been activated and is currently stopped.
  /// \returns True if the play trigger is currently stopped, false otherwise.
  PLASMA_NODISCARD bool IsStopped() const;

  /// \brief Returns whether the play trigger is being unloaded.
  /// \returns True if the play trigger is currently being unloaded, false otherwise.
  PLASMA_NODISCARD bool IsUnloading() const;

  /// \brief Activates the play trigger. If the play trigger was not loaded on initialization, this will
  /// load the play trigger the first time it's called.
  ///
  /// \param bSync Whether the request should be executed synchronously or asynchronously.
  void Play(bool bSync = false);

  /// \brief If a stop trigger is defined, this will activate it. Otherwise, the triggered event will be stopped.
  ///
  /// \param bSync Whether the request should be executed synchronously or asynchronously.
  void Stop(bool bSync = false);

private:
  void LoadPlayTrigger(bool bSync);
  void LoadStopTrigger(bool bSync, bool bDeinit);
  void UnloadPlayTrigger(bool bSync, bool bDeinit = false);
  void UnloadStopTrigger(bool bSync, bool bDeinit = false);

  void UpdateOcclusion();
  void Update();

  void StopInternal(bool bSync = false, bool bDeinit = false);

  plEnum<plAudioSystemTriggerState> m_eState;

  plAudioSystemDataID m_uiPlayEventId = 0;
  plAudioSystemDataID m_uiStopEventId = 0;

  plString m_sPlayTrigger;
  plString m_sStopTrigger;

  plEnum<plAudioSystemSoundObstructionType> m_eObstructionType;
  plUInt8 m_uiOcclusionCollisionLayer;
  plUInt32 m_uiObstructionOcclusionStateIndex;

  bool m_bLoadOnInit;
  bool m_bPlayOnActivate;

  bool m_bPlayTriggerLoaded{false};
  bool m_bStopTriggerLoaded{false};

  bool m_bHasPlayedOnActivate{false};

  bool m_bCanPlay{false};
};
