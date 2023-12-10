#pragma once

#include <EditorPluginAmplitudeAudio/EditorPluginAmplitudeAudioDLL.h>
#include <AmplitudeAudioPlugin/Core/AmplitudeAudioData.h>
#include <AudioSystemPlugin/Core/AudioSystemData.h>

#include <Foundation/Configuration/Singleton.h>

#include <SparkyStudios/Audio/Amplitude/Amplitude.h>

using namespace SparkyStudios::Audio;

class plJSONReader;

class PLASMA_EDITORPLUGINAMPLITUDEAUDIO_DLL plAmplitudeAudioControlsManager
{
  PLASMA_DECLARE_SINGLETON(plAmplitudeAudioControlsManager);

public:
  plAmplitudeAudioControlsManager();
  ~plAmplitudeAudioControlsManager() = default;

  /// \brief Create controls assets needed for Amplitude Audio.
  /// \return EZ_SUCCESS on success, otherwise EZ_FAILURE.
  plResult ReloadControls();

private:
  plResult SerializeTriggerControl(plStreamWriter* pStream, const plAudioSystemTriggerData* pControlData);
  plResult SerializeRtpcControl(plStreamWriter* pStream, const plAudioSystemRtpcData* pControlData);
  plResult SerializeSwitchStateControl(plStreamWriter* pStream, const plAudioSystemSwitchStateData* pControlData);
  plResult SerializeEnvironmentControl(plStreamWriter* pStream, const plAudioSystemEnvironmentData* pControlData);
  plResult SerializeSoundBankControl(plStreamWriter* pStream, const plAudioSystemBankData* pControlData);

  plResult CreateTriggerControl(const char* szControlName, const plAudioSystemTriggerData* pControlData);
  plResult CreateRtpcControl(const char* szControlName, const plAudioSystemRtpcData* pControlData);
  plResult CreateSwitchStateControl(const char* szControlName, const plAudioSystemSwitchStateData* pControlData);
  plResult CreateEnvironmentControl(const char* szControlName, const plAudioSystemEnvironmentData* pControlData);
  plResult CreateSoundBankControl(const char* szControlName, const plAudioSystemBankData* pControlData);

  plResult LoadSoundBanks(const char* szRootFolder, const char* szSubPath);
  //  void LoadBuses(const char* sRootFolder);
  plResult LoadControlsInFolder(const char* szFolderPath, const plEnum<plAmplitudeAudioControlType>& eType);
  plResult LoadControl(const plVariantDictionary& json, const plEnum<plAmplitudeAudioControlType>& eType);
};
