#pragma once

#include <AmplitudeAudioPlugin/AmplitudeAudioPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioMiddleware.h>

#include <Core/ResourceManager/ResourceHandle.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

#include <SparkyStudios/Audio/Amplitude/Amplitude.h>

struct plGameApplicationExecutionEvent;
class plOpenDdlWriter;
class plOpenDdlReaderElement;
typedef plDynamicArray<plUInt8> plDataBuffer;

/// \brief The Amplitude configuration to be used on a specific platform.
struct PL_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeConfiguration
{
  plString m_sInitSoundBank;
  plString m_sEngineConfigFileName;

  void Save(plOpenDdlWriter& ddl) const;
  void Load(const plOpenDdlReaderElement& ddl);

  bool operator==(const plAmplitudeConfiguration& rhs) const;
  bool operator!=(const plAmplitudeConfiguration& rhs) const { return !operator==(rhs); }
};

/// \brief Loads and stores all available platform-specific configurations
/// for the Amplitude Audio middleware.
struct PL_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeAssetProfiles
{
  plResult Save(plOpenDdlWriter& writer) const;
  plResult Load(const plOpenDdlReaderElement& reader);

  plMap<plString, plAmplitudeConfiguration> m_AssetProfiles;
};

/// \brief The Amplitude Audio Middleware.
///
/// This class implements the plAudioMiddleware interface of the Audio System
/// and allows the audio system to execute audio requests through Amplitude Audio.
class PL_AMPLITUDEAUDIOPLUGIN_DLL plAmplitude final : public plAudioMiddleware
{
private:
  PL_DECLARE_SINGLETON_OF_INTERFACE(plAmplitude, plAudioMiddleware);

  // plAudioMiddleware

public:
  plResult LoadConfiguration(const plOpenDdlReaderElement& reader) override;
  plResult Startup() override;
  void Update(plTime delta) override;
  plResult Shutdown() override;
  plResult Release() override;
  plResult StopAllSounds() override;
  plResult AddEntity(plAudioSystemEntityData* pEntityData, const char* szEntityName) override;
  plResult ResetEntity(plAudioSystemEntityData* pEntityData) override;
  plResult UpdateEntity(plAudioSystemEntityData* pEntityData) override;
  plResult RemoveEntity(plAudioSystemEntityData* pEntityData) override;
  plResult SetEntityTransform(plAudioSystemEntityData* pEntityData, const plAudioSystemTransform& Transform) override;
  plResult LoadTrigger(plAudioSystemEntityData* pEntityData, const plAudioSystemTriggerData* pTriggerData, plAudioSystemEventData* pEventData) override;
  plResult ActivateTrigger(plAudioSystemEntityData* pEntityData, const plAudioSystemTriggerData* pTriggerData, plAudioSystemEventData* pEventData) override;
  plResult UnloadTrigger(plAudioSystemEntityData* pEntityData, const plAudioSystemTriggerData* pTriggerData) override;
  plResult StopEvent(plAudioSystemEntityData* pEntityData, const plAudioSystemEventData* pEventData) override;
  plResult StopAllEvents(plAudioSystemEntityData* pEntityData) override;
  plResult SetRtpc(plAudioSystemEntityData* pEntityData, const plAudioSystemRtpcData* pRtpcData, float fValue) override;
  plResult ResetRtpc(plAudioSystemEntityData* pEntityData, const plAudioSystemRtpcData* pRtpcData) override;
  plResult SetSwitchState(plAudioSystemEntityData* pEntityData, const plAudioSystemSwitchStateData* pSwitchStateData) override;
  plResult SetObstructionAndOcclusion(plAudioSystemEntityData* pEntityData, float fObstruction, float fOcclusion) override;
  plResult SetEnvironmentAmount(plAudioSystemEntityData* pEntityData, const plAudioSystemEnvironmentData* pEnvironmentData, float fAmount) override;
  plResult AddListener(plAudioSystemListenerData* pListenerData, const char* szListenerName) override;
  plResult ResetListener(plAudioSystemListenerData* pListenerData) override;
  plResult RemoveListener(plAudioSystemListenerData* pListenerData) override;
  plResult SetListenerTransform(plAudioSystemListenerData* pListenerData, const plAudioSystemTransform& Transform) override;
  plResult LoadBank(plAudioSystemBankData* pBankData) override;
  plResult UnloadBank(plAudioSystemBankData* pBankData) override;
  plAudioSystemEntityData* CreateWorldEntity(plAudioSystemDataID uiEntityId) override;
  plAudioSystemEntityData* CreateEntityData(plAudioSystemDataID uiEntityId) override;
  plResult DestroyEntityData(plAudioSystemEntityData* pEntityData) override;
  plAudioSystemListenerData* CreateListenerData(plAudioSystemDataID uiListenerId) override;
  plResult DestroyListenerData(plAudioSystemListenerData* pListenerData) override;
  plAudioSystemEventData* CreateEventData(plAudioSystemDataID uiEventId) override;
  plResult ResetEventData(plAudioSystemEventData* pEventData) override;
  plResult DestroyEventData(plAudioSystemEventData* pEventData) override;
  plResult DestroyBank(plAudioSystemBankData* pBankData) override;
  plResult DestroyTriggerData(plAudioSystemTriggerData* pTriggerData) override;
  plResult DestroyRtpcData(plAudioSystemRtpcData* pRtpcData) override;
  plResult DestroySwitchStateData(plAudioSystemSwitchStateData* pSwitchStateData) override;
  plResult DestroyEnvironmentData(plAudioSystemEnvironmentData* pEnvironmentData) override;
  plResult SetLanguage(const char* szLanguage) override;
  PL_NODISCARD const char* GetMiddlewareName() const override;
  PL_NODISCARD float GetMasterGain() const override;
  PL_NODISCARD bool GetMute() const override;
  void OnMasterGainChange(float fGain) override;
  void OnMuteChange(bool bMute) override;
  void OnLoseFocus() override;
  void OnGainFocus() override;

  // plAmplitude

public:
  static void GameApplicationEventHandler(const plGameApplicationExecutionEvent& e);

  plAmplitude();
  ~plAmplitude() override;

  PL_NODISCARD SparkyStudios::Audio::Amplitude::Engine* GetEngine() const { return m_pEngine; }

  /// \brief Parses the entry in the controls collection that represent a bank.
  /// \param pBankEntry The stream storing the bank entry.
  /// \return The created bank data, or nullptr if no bank was created.
  plAudioSystemBankData* DeserializeBankEntry(plStreamReader* pBankEntry);

  /// \brief Parses the entry in the controls collection that represent a trigger.
  /// \param pTriggerEntry The stream storing the event entry.
  /// \return The created trigger data, or nullptr if no trigger was created.
  plAudioSystemTriggerData* DeserializeTriggerEntry(plStreamReader* pTriggerEntry) const;

  /// \brief Parses the entry in the controls collection that represent a rtpc.
  /// \param pRtpcEntry The stream storing the rtpc entry.
  /// \return The created rtpc data, or nullptr if no rtpc was created.
  plAudioSystemRtpcData* DeserializeRtpcEntry(plStreamReader* pRtpcEntry) const;

  /// \brief Parses the entry in the controls collection that represent a switch state.
  /// \param pSwitchStateEntry The stream storing the switch state entry.
  /// \return The created switch state data, or nullptr if no switch state was created.
  plAudioSystemSwitchStateData* DeserializeSwitchStateEntry(plStreamReader* pSwitchStateEntry) const;

  /// \brief Parses the entry in the controls collection that represent a environment effect.
  /// \param pEnvironmentEntry The stream storing the environment effect entry.
  /// \return The created environment effect data, or nullptr if no environment effect was created.
  plAudioSystemEnvironmentData* DeserializeEnvironmentEntry(plStreamReader* pEnvironmentEntry) const;

private:
  void DetectPlatform() const;

  SparkyStudios::Audio::Amplitude::Engine* m_pEngine;

  SparkyStudios::Audio::Amplitude::AmTime m_dCurrentTime;
  SparkyStudios::Audio::Amplitude::FileLoader m_Loader;

  bool m_bInitialized;

  struct Data
  {
    plAmplitudeAssetProfiles m_Configs;
    plString m_sPlatform;
    SparkyStudios::Audio::Amplitude::AmObjectID m_uiInitSoundBank{SparkyStudios::Audio::Amplitude::kAmInvalidObjectId};
  };

  plUniquePtr<Data> m_pData;
};
