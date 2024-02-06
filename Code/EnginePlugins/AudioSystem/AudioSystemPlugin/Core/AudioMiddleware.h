#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>

#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/Time/Time.h>

/// \brief The Audio Middleware Interface.
/// This interface should be implemented by the ATL middleware to communicate with the audio system.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioMiddleware
{
public:
  // Audio Middleware Interface

  virtual ~plAudioMiddleware() = default;

  /// \brief Loads the middleware-specif configuration from the ATL config file.
  /// \param reader The DDL file reader.
  virtual plResult LoadConfiguration(const plOpenDdlReaderElement& reader) = 0;

  /// \brief Initializes the audio middleware.
  /// \return PL_SUCCESS when the audio middleware was successfully initialized,
  /// PL_FAILURE otherwise.
  virtual plResult Startup() = 0;

  /// \brief Updates the audio middleware.
  /// \param delta The elapsed time since the last update.
  virtual void Update(plTime delta) = 0;

  /// \brief Deinitializes and stop the audio middleware.
  /// \return PL_SUCCESS when the audio middleware was successfully stopped,
  /// PL_FAILURE otherwise.
  virtual plResult Shutdown() = 0;

  /// \brief Destroys all the resources allocated by the audio middleware.
  /// This is usually called after Shutdown.
  /// \return PL_SUCCESS when audio middleware resources was successfully released,
  /// PL_FAILURE otherwise.
  virtual plResult Release() = 0;

  /// \brief Stops all the sounds actually played by the audio middleware.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult StopAllSounds() = 0;

  /// \brief Add/register an entity in the audio middleware.
  /// This is needed in order to provide transformation values (position and orientation), execute
  /// triggers (play sounds), and set real-time parameters or switches.
  /// \param pEntityData The entity that should be added in the audio middleware.
  /// \param szEntityName The name of the game object representing that entity. (Can be used for debug purposes)
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult AddEntity(plAudioSystemEntityData* pEntityData, const char* szEntityName) = 0;

  /// \brief Resets an entity state.
  /// \param pEntityData The entity that should be reset.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult ResetEntity(plAudioSystemEntityData* pEntityData) = 0;

  /// \brief Updates an entity state.
  /// \param pEntityData The entity that should be updated.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult UpdateEntity(plAudioSystemEntityData* pEntityData) = 0;

  /// \brief Remove/unregister an entity from the audio middleware.
  /// This action disable the possibility to execute triggers, set real-time parameters or switches, and
  /// update the position of the entity.
  /// \param pEntityData The entity that should be removed from the audio middleware.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult RemoveEntity(plAudioSystemEntityData* pEntityData) = 0;

  /// \brief Sets the global position (world position) of an entity.
  /// \param pEntityData The entity on which set the global position.
  /// \param Transform The global transform of the entity.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult SetEntityTransform(plAudioSystemEntityData* pEntityData, const plAudioSystemTransform& Transform) = 0;

  /// \brief Loads a trigger for a further activation.
  /// All the data and media needed by the trigger will be loaded. Once done, the trigger status will
  /// change to Ready.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult LoadTrigger(plAudioSystemEntityData* pEntityData, const plAudioSystemTriggerData* pTriggerData, plAudioSystemEventData* pEventData) = 0;

  /// \brief Triggers an event on an entity. A trigger is everything which can affect the state of an event.
  /// \param pEntityData The entity on which trigger the event.
  /// \param pTriggerData The event trigger. Can't be modified here.
  /// \param pEventData The triggered event.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult ActivateTrigger(plAudioSystemEntityData* pEntityData, const plAudioSystemTriggerData* pTriggerData, plAudioSystemEventData* pEventData) = 0;

  /// \brief Unload the trigger. This is called when the trigger and all data loaded during LoadTrigger
  /// need to be disposed.
  /// \param pEntityData The entity on which trigger the event.
  /// \param pTriggerData The event trigger. Can't be modified here.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult UnloadTrigger(plAudioSystemEntityData* pEntityData, const plAudioSystemTriggerData* pTriggerData) = 0;

  /// \brief Stops an event on the given entity.
  /// \param pEntityData The entity on which stop the event.
  /// \param pEventData The event to stop.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult StopEvent(plAudioSystemEntityData* pEntityData, const plAudioSystemEventData* pEventData) = 0;

  /// \brief Stops all events active on the given entity.
  /// \param pEntityData The entity on which stop all events.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult StopAllEvents(plAudioSystemEntityData* pEntityData) = 0;

  /// \brief Sets an audio RTPC to the specified value on an entity.
  /// \param pEntityData The entity on which set the rtpc value.
  /// \param pRtpcData The rtpc data.
  /// \param fValue The rtpc value.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult SetRtpc(plAudioSystemEntityData* pEntityData, const plAudioSystemRtpcData* pRtpcData, float fValue) = 0;

  /// \brief Resets an audio RTPC to the default value on an entity.
  /// \param pEntityData The entity on which reset the rtpc value.
  /// \param pRtpcData The rtpc data.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult ResetRtpc(plAudioSystemEntityData* pEntityData, const plAudioSystemRtpcData* pRtpcData) = 0;

  /// \brief Sets an audio switch to the specified state on an entity.
  /// \param pEntityData The entity on which set the switch value.
  /// \param pSwitchStateData The switch state data.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult SetSwitchState(plAudioSystemEntityData* pEntityData, const plAudioSystemSwitchStateData* pSwitchStateData) = 0;

  /// \brief Sets the obstruction and occlusion values on an entity.
  /// \param pEntityData The entity on which set the obstruction and occlusion values.
  /// \param fObstruction The obstruction value.
  /// \param fOcclusion The occlusion value.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult SetObstructionAndOcclusion(plAudioSystemEntityData* pEntityData, float fObstruction, float fOcclusion) = 0;

  /// \brief Sets the amount of an audio environment effect associated on an entity.
  /// \param pEntityData The entity on which set the environment effect value.
  /// \param pEnvironmentData The environment effect data.
  /// \param fAmount The environment effect value.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult SetEnvironmentAmount(plAudioSystemEntityData* pEntityData, const plAudioSystemEnvironmentData* pEnvironmentData, float fAmount) = 0;

  /// \brief Add/register a listener in the audio middleware.
  /// This is needed to let the middleware know where to render audio, and to provide
  /// transformation values (position and orientation).
  /// \param pListenerData The listener that should be added in the audio middleware.
  /// \param szListenerName The name of the game object representing that listener. (Can be used for debug purposes)
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult AddListener(plAudioSystemListenerData* pListenerData, const char* szListenerName) = 0;

  /// \brief Resets a listener state.
  /// \param pListenerData The listener that should be reset.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult ResetListener(plAudioSystemListenerData* pListenerData) = 0;

  /// \brief Remove/unregister a listener from the audio middleware.
  /// This action disable the possibility to update the position of the listener.
  /// \param pListenerData The listener that should be removed from the audio middleware.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult RemoveListener(plAudioSystemListenerData* pListenerData) = 0;

  /// \brief Sets the global transform (wold position and orientation) of a listener.
  /// \param pListenerData The listener data.
  /// \param Transform The global transformation of the listener.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult SetListenerTransform(plAudioSystemListenerData* pListenerData, const plAudioSystemTransform& Transform) = 0;

  /// \brief Loads a bank file.
  /// \param pBankData The bank data used for loading.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult LoadBank(plAudioSystemBankData* pBankData) = 0;

  /// \brief Unloads a bank file.
  /// \param pBankData The bank data used for unloading.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult UnloadBank(plAudioSystemBankData* pBankData) = 0;

  /// \brief Creates a world entity. This is a special entity used by non-spatial sounds.
  /// The engine will take care to create a game object at position (0,0,0) and orientation (0,0,0,1),
  /// and pass the game object ID as parameter to this function. The game object is ensured to never
  /// move nor rotate.
  /// \param uiEntityId The game object ID referencing the new audio entity.
  /// \return The created entity data, or nullptr if it was not created.
  virtual plAudioSystemEntityData* CreateWorldEntity(plAudioSystemDataID uiEntityId) = 0;

  /// \brief Creates an audio entity that is attached to a game object.
  /// \param uiEntityId The game object ID referencing the new audio entity.
  /// \return The created entity data, or nullptr if it was not created.
  virtual plAudioSystemEntityData* CreateEntityData(plAudioSystemDataID uiEntityId) = 0;

  /// \brief Destroys an audio entity and release memory.
  /// \param pEntityData The entity data to destroy.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult DestroyEntityData(plAudioSystemEntityData* pEntityData) = 0;

  /// \brief Creates an audio listener that is attached to a game object.
  /// \param uiListenerId The game object ID referencing the new audio listener.
  /// \return The created listener data, or nullptr if it was not created.
  virtual plAudioSystemListenerData* CreateListenerData(plAudioSystemDataID uiListenerId) = 0;

  /// \brief Destroys an audio listener and release memory.
  /// \param pListenerData The listener data to destroy.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult DestroyListenerData(plAudioSystemListenerData* pListenerData) = 0;

  /// \brief Creates an audio event that is attached to a game object.
  /// \param uiEventId The game object ID referencing the new audio event.
  /// \return The created event data, or nullptr if it was not created.
  virtual plAudioSystemEventData* CreateEventData(plAudioSystemDataID uiEventId) = 0;

  /// \brief Resets the audio event state, so it can safely recycled in the pool.
  /// \param pEventData The event data to reset.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult ResetEventData(plAudioSystemEventData* pEventData) = 0;

  /// \brief Destroys an audio event and release memory.
  /// \param pEventData The event data to destroy.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult DestroyEventData(plAudioSystemEventData* pEventData) = 0;

  /// \brief Destroys an audio bank and release memory.
  /// \param pBankData The bank data to destroy.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult DestroyBank(plAudioSystemBankData* pBankData) = 0;

  /// \brief Destroys an audio trigger and release memory.
  /// \param pTriggerData The trigger data to destroy.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult DestroyTriggerData(plAudioSystemTriggerData* pTriggerData) = 0;

  /// \brief Destroys an audio rtpc and release memory.
  /// \param pRtpcData The rtpc data to destroy.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult DestroyRtpcData(plAudioSystemRtpcData* pRtpcData) = 0;

  /// \brief Destroys an audio switch state and release memory.
  /// \param pSwitchStateData The switch state data to destroy.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult DestroySwitchStateData(plAudioSystemSwitchStateData* pSwitchStateData) = 0;

  /// \brief Destroys an audio environment effect and release memory.
  /// \param pEnvironmentData The environment effect data to destroy.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult DestroyEnvironmentData(plAudioSystemEnvironmentData* pEnvironmentData) = 0;

  /// \brief Sets the language used by the audio middleware.
  /// \param szLanguage The language to use.
  /// \return PL_SUCCESS when the operation is successful, PL_FAILURE otherwise.
  virtual plResult SetLanguage(const char* szLanguage) = 0;

  /// \brief Gets the audio middleware implementation name.
  /// e.g. "FMOD", "Wwise", "Amplitude", etc.
  /// \return The name of the audio middleware.
  PL_NODISCARD virtual const char* GetMiddlewareName() const = 0;

  /// \brief Gets the audio middleware's master gain.
  /// \return The master gain.
  PL_NODISCARD virtual float GetMasterGain() const = 0;

  /// \brief Gets the audio middleware's muted state.
  /// \return The muted state.
  PL_NODISCARD virtual bool GetMute() const = 0;

  /// \brief Called each time the master gain value change.
  /// \param fGain The master gain value.
  virtual void OnMasterGainChange(float fGain) = 0;

  /// \brief Called when the audio middleware should toggle the muted state.
  /// \paarm bMute The muted state.
  virtual void OnMuteChange(bool bMute) = 0;

  /// \brief Called each time the game application window loses focus.
  virtual void OnLoseFocus() = 0;

  /// \brief Called each time the game application window gains focus.
  virtual void OnGainFocus() = 0;
};
