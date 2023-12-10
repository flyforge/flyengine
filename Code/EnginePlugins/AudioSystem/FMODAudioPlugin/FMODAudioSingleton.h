#pragma once

#include <FMODAudioPlugin/FMODAudioPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioMiddleware.h>

#include <Core/ResourceManager/ResourceHandle.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

using plFmodSoundBankResourceHandle = plTypedResourceHandle<class plFmodSoundBankResource>;

/// \brief Abstraction of FMOD_SPEAKERMODE
enum class plFmodSpeakerMode : plUInt8
{
  ModeStereo,
  Mode5Point1,
  Mode7Point1,
};

/// \brief The fmod configuration to be used on a specific platform
struct PLASMA_FMODAUDIOPLUGIN_DLL plFmodConfiguration
{
  plString m_sMasterSoundBank;
  plFmodSpeakerMode m_SpeakerMode = plFmodSpeakerMode::Mode5Point1; ///< This must be set to what is configured in Fmod Studio for the
                                                                    ///< target platform. Using anything else is incorrect.
  plUInt16 m_uiVirtualChannels = 32;                                ///< See FMOD::Studio::System::initialize
  plUInt32 m_uiSamplerRate = 0;                                     ///< See FMOD::System::setSoftwareFormat

  void Save(plOpenDdlWriter& ddl) const;
  void Load(const plOpenDdlReaderElement& ddl);

  bool operator==(const plFmodConfiguration& rhs) const;
  bool operator!=(const plFmodConfiguration& rhs) const { return !operator==(rhs); }
};

/// \brief All available fmod platform configurations
struct PLASMA_FMODAUDIOPLUGIN_DLL plFmodAssetProfiles
{
  static constexpr const plStringView s_sConfigFile = ":project/RuntimeConfigs/FmodConfig.ddl"_plsv;

  plResult Save(plStringView sFile = s_sConfigFile) const;
  plResult Load(plStringView sFile = s_sConfigFile);

  plMap<plString, plFmodConfiguration> m_AssetProfiles;
};

class PLASMA_FMODAUDIOPLUGIN_DLL plFMOD final : public plAudioMiddleware
{
private:
  PLASMA_DECLARE_SINGLETON_OF_INTERFACE(plFMOD, plAudioMiddleware);

public:

  plFMOD();
  ~plFMOD() override;  

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
  PLASMA_NODISCARD const char* GetMiddlewareName() const override;
  PLASMA_NODISCARD float GetMasterGain() const override;
  PLASMA_NODISCARD bool GetMute() const override;
  void OnMasterGainChange(float fGain) override;
  void OnMuteChange(bool bMute) override;
  void OnLoseFocus() override;
  void OnGainFocus() override;

  FMOD::Studio::System* GetStudioSystem() const { return m_pStudioSystem; }
  FMOD::System* GetLowLevelSystem() const { return m_pLowLevelSystem; }

  void DetectPlatform();

private:

  bool m_bInitialized = false;

  FMOD::Studio::System* m_pStudioSystem;
  FMOD::System* m_pLowLevelSystem;

  struct Data
  {
    plMap<plString, float> m_VcaVolumes;
    plFmodAssetProfiles m_Configs;
    plString m_sPlatform;
    plFmodSoundBankResourceHandle m_hMasterBank;
    plFmodSoundBankResourceHandle m_hMasterBankStrings;
    plHybridArray<plDataBuffer*, 4> m_SbDeletionQueue;
  };

  plUniquePtr<Data> m_pData;
};