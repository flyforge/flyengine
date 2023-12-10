#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/ATL/AudioTranslationLayer.h>
#include <AudioSystemPlugin/Core/AudioSystemData.h>
#include <AudioSystemPlugin/Core/AudioThread.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Interfaces/SoundInterface.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Threading/Semaphore.h>

typedef plDeque<plVariant> plAudioSystemRequestsQueue;

/// \brief The AudioSystem.
///
/// This class is responsible to handle the communication between the
/// ATL and PLASMA. It implements the plSoundInterface class and provides
/// methods to push audio requests in the requests queue (synchronously or asynchronously).
class PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystem final : public plSoundInterface
{
  PLASMA_DECLARE_SINGLETON_OF_INTERFACE(plAudioSystem, plSoundInterface);

  // ----- plSoundInterface

public:
  /// \brief This should be called in the audio middleware implementation startup to
  /// load the AudioSystem with the correct configuration.
  void LoadConfiguration(plStringView sFile) override;

  /// \brief By default, the AudioSystem will auto-detect the platform (and thus the config) to use.
  /// Calling this before startup allows to override which configuration is used.
  void SetOverridePlatform(plStringView sPlatform) override;

  /// \brief Called once per frame to update all sounds.
  void UpdateSound() override;

  /// \brief Asks the audio middleware to adjust its master volume.
  void SetMasterChannelVolume(float fVolume) override;

  /// \brief Gets the master volume of the audio middleware.
  float GetMasterChannelVolume() const override;

  /// \brief Asks the audio middleware to mute its master channel.
  void SetMasterChannelMute(bool bMute) override;

  /// \brief Gets the muted state of the audio middleware master channel.
  bool GetMasterChannelMute() const override;

  /// \brief Asks the audio middleware to pause every playbacks.
  void SetMasterChannelPaused(bool bPaused) override;

  /// \brief Gets the paused state of the audio middleware.
  bool GetMasterChannelPaused() const override;

  /// \brief Asks the audio middleware to adjust the volume of a sound group.
  void SetSoundGroupVolume(plStringView sVcaGroupGuid, float fVolume) override;

  /// \brief Gets a sound group volume from the audio middleware.
  float GetSoundGroupVolume(plStringView sVcaGroupGuid) const override;

  /// \brief Asks the audio middleware to set the required number of listeners.
  void SetNumListeners(plUInt8 uiNumListeners) override {}

  /// \brief Gets the number of listeners from the audio middleware.
  plUInt8 GetNumListeners() override;

  /// \brief Overrides the active audio middleware listener by the editor camera. Transformation
  /// data will be provided by the editor camera.
  void SetListenerOverrideMode(bool bEnabled) override;

  /// \brief Sets the transformation of the listener with the given ID.
  /// ID -1 is used for the override mode listener (editor camera).
  void SetListener(plInt32 iIndex, const plVec3& vPosition, const plVec3& vForward, const plVec3& vUp, const plVec3& vVelocity) override;

//  /// \brief Plays a sound once. Called by plSoundInterface::PlaySound().
//  plResult OneShotSound(plStringView sResourceID, const plTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true) override;

  // ----- plAudioSystem

public:
  plAudioSystem();
  virtual ~plAudioSystem();

  bool Startup();
  void Shutdown();

  PLASMA_NODISCARD bool IsInitialized() const;

  void SendRequest(plVariant&& request);
  void SendRequests(plAudioSystemRequestsQueue& requests);

  void SendRequestSync(plVariant&& request);

  plAudioSystemDataID GetTriggerId(plStringView sTriggerName) const;

  plAudioSystemDataID GetRtpcId(plStringView sRtpcName) const;

  plAudioSystemDataID GetSwitchStateId(plStringView sSwitchStateName) const;

  plAudioSystemDataID GetEnvironmentId(plStringView sEnvironmentName) const;

  plAudioSystemDataID GetBankId(plStringView sBankName) const;

  void RegisterTrigger(plAudioSystemDataID uiId, plAudioSystemTriggerData* pTriggerData);
  void RegisterRtpc(plAudioSystemDataID uiId, plAudioSystemRtpcData* pRtpcData);
  void RegisterSwitchState(plAudioSystemDataID uiId, plAudioSystemSwitchStateData* pSwitchStateData);
  void RegisterEnvironment(plAudioSystemDataID uiId, plAudioSystemEnvironmentData* pEnvironmentData);
  void RegisterSoundBank(plAudioSystemDataID uiId, plAudioSystemBankData* pSoundBankData);

  void UnregisterEntity(plAudioSystemDataID uiId);
  void UnregisterListener(plAudioSystemDataID uiId);
  void UnregisterTrigger(plAudioSystemDataID uiId);
  void UnregisterRtpc(plAudioSystemDataID uiId);
  void UnregisterSwitchState(plAudioSystemDataID uiId);
  void UnregisterEnvironment(plAudioSystemDataID uiId);
  void UnregisterSoundBank(plAudioSystemDataID uiId);

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(AudioSystem, AudioSystemPlugin);

  friend class plAudioThread;
  friend class plAudioTranslationLayer;

  static void GameApplicationEventHandler(const plGameApplicationExecutionEvent& e);

  void UpdateInternal();

  void StartAudioThread();
  void StopAudioThread();

  void QueueRequestCallback(plVariant&& request, bool bSync);

  plAudioThread* m_pAudioThread = nullptr;
  plAudioTranslationLayer m_AudioTranslationLayer;

  plAudioSystemRequestsQueue m_RequestsQueue;
  plAudioSystemRequestsQueue m_PendingRequestsQueue;
  plAudioSystemRequestsQueue m_BlockingRequestsQueue;
  plAudioSystemRequestsQueue m_PendingRequestCallbacksQueue;
  plAudioSystemRequestsQueue m_BlockingRequestCallbacksQueue;

  mutable plMutex m_RequestsMutex;
  mutable plMutex m_PendingRequestsMutex;
  mutable plMutex m_BlockingRequestsMutex;
  mutable plMutex m_PendingRequestCallbacksMutex;
  mutable plMutex m_BlockingRequestCallbacksMutex;

  plSemaphore m_MainEvent;
  plSemaphore m_ProcessingEvent;

  bool m_bInitialized;

  bool m_bListenerOverrideMode;
};
