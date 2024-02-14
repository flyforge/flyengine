#include "AudioSystemPlugin/Core/AudioSystemData.h"
#include "Foundation/Types/Types.h"
#include <EditorPluginAmplitudeAudio/EditorPluginAmplitudeAudioPCH.h>

#include <EditorPluginAmplitudeAudio/Core/AmplitudeAudioControlsManager.h>

#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>

#include <AmplitudeAudioPlugin/Core/AmplitudeAudioData.h>
#include <AmplitudeAudioPlugin/Core/Common.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/IO/OSFile.h>

PL_IMPLEMENT_SINGLETON(plAmplitudeAudioControlsManager);

plAmplitudeAudioControlsManager::plAmplitudeAudioControlsManager()
  : m_SingletonRegistrar(this)
{
}

plResult plAmplitudeAudioControlsManager::ReloadControls()
{
  plStringBuilder assetsPath;
  if (plFileSystem::ResolvePath(":project/Sounds/Amplitude/amplitude_assets", &assetsPath, nullptr).Failed())
  {
    plLog::Error("No Amplitude assets directory available. Amplitude will be deactivated.");
    return PL_FAILURE;
  }

  plStringBuilder projectPath;
  if (plFileSystem::ResolvePath(":project/Sounds/Amplitude/amplitude_project", &projectPath, nullptr).Failed())
  {
    plLog::Error("No Amplitude project directory available. Amplitude will be deactivated.");
    return PL_FAILURE;
  }

  if (plFileSystem::FindDataDirectoryWithRoot("atl") == nullptr)
    plFileSystem::AddDataDirectory(">project/Sounds/ATL/Amplitude", "ATL", "atl", plFileSystem::AllowWrites).IgnoreResult();

  {
    plStringBuilder basePath(projectPath);
    basePath.AppendPath(kEventsFolder);

    if (LoadControlsInFolder(basePath, plAmplitudeAudioControlType::Trigger).Failed())
      return PL_FAILURE;
  }

  {
    plStringBuilder basePath(projectPath);
    basePath.AppendPath(kRtpcFolder);

    if (LoadControlsInFolder(basePath, plAmplitudeAudioControlType::Rtpc).Failed())
      return PL_FAILURE;
  }

  {
    plStringBuilder basePath(projectPath);
    basePath.AppendPath(kSwitchesFolder);

    if (LoadControlsInFolder(basePath, plAmplitudeAudioControlType::Switch).Failed())
      return PL_FAILURE;
  }

  {
    plStringBuilder basePath(projectPath);
    basePath.AppendPath(kEnvironmentsFolder);

    if (LoadControlsInFolder(basePath, plAmplitudeAudioControlType::Environment).Failed())
      return PL_FAILURE;
  }

  return LoadSoundBanks(projectPath, kSoundBanksFolder);
}

plResult plAmplitudeAudioControlsManager::SerializeTriggerControl(plStreamWriter* pStream, const plAudioSystemTriggerData* pControlData)
{
  if (pStream == nullptr || pControlData == nullptr)
    return PL_FAILURE;

  if (const auto* const pAmplitudeAudioTriggerData = plDynamicCast<const plAmplitudeAudioTriggerData*>(pControlData); pAmplitudeAudioTriggerData != nullptr)
  {
    *pStream << pAmplitudeAudioTriggerData->m_uiAmId;
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

plResult plAmplitudeAudioControlsManager::SerializeRtpcControl(plStreamWriter* pStream, const plAudioSystemRtpcData* pControlData)
{
  if (pStream == nullptr || pControlData == nullptr)
    return PL_FAILURE;

  if (const auto* const pAmplitudeAudioRtpcData = plDynamicCast<const plAmplitudeAudioRtpcData*>(pControlData); pAmplitudeAudioRtpcData != nullptr)
  {
    *pStream << pAmplitudeAudioRtpcData->m_uiAmId;
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

plResult plAmplitudeAudioControlsManager::SerializeSwitchStateControl(plStreamWriter* pStream, const plAudioSystemSwitchStateData* pControlData)
{
  if (pStream == nullptr || pControlData == nullptr)
    return PL_FAILURE;

  if (const auto* const pAmplitudeAudioSwitchStateData = plDynamicCast<const plAmplitudeAudioSwitchStateData*>(pControlData); pAmplitudeAudioSwitchStateData != nullptr)
  {
    *pStream << pAmplitudeAudioSwitchStateData->m_uiSwitchId;
    *pStream << pAmplitudeAudioSwitchStateData->m_uiSwitchStateId;
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

plResult plAmplitudeAudioControlsManager::SerializeEnvironmentControl(plStreamWriter* pStream, const plAudioSystemEnvironmentData* pControlData)
{
  if (pStream == nullptr || pControlData == nullptr)
    return PL_FAILURE;

  if (const auto* const pAmplitudeAudioEnvironmentData = plDynamicCast<const plAmplitudeAudioEnvironmentData*>(pControlData); pAmplitudeAudioEnvironmentData != nullptr)
  {
    *pStream << pAmplitudeAudioEnvironmentData->m_uiAmId;
    *pStream << pAmplitudeAudioEnvironmentData->m_uiEffectId;
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

plResult plAmplitudeAudioControlsManager::SerializeSoundBankControl(plStreamWriter* pStream, const plAudioSystemBankData* pControlData)
{
  if (pStream == nullptr || pControlData == nullptr)
    return PL_FAILURE;

  if (const auto* const pAmplitudeAudioSoundBankData = plDynamicCast<const plAmplitudeAudioSoundBankData*>(pControlData); pAmplitudeAudioSoundBankData != nullptr)
  {
    *pStream << pAmplitudeAudioSoundBankData->m_uiAmId;
    *pStream << pAmplitudeAudioSoundBankData->m_sFileName;
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

plResult plAmplitudeAudioControlsManager::CreateTriggerControl(const char* szControlName, const plAudioSystemTriggerData* pControlData)
{
  plStringBuilder sbOutputFile;
  sbOutputFile.SetFormat(":atl/Triggers/{0}.plAudioSystemControl", szControlName);

  plStringBuilder sbAssetPath;
  if (plFileSystem::ResolvePath(sbOutputFile, &sbAssetPath, nullptr).Failed())
    return PL_FAILURE;

  plFileWriter file;
  if (file.Open(sbAssetPath, 256).Failed())
    return PL_FAILURE;

  // Set the control type
  file << plAmplitudeAudioControlType::Trigger;

  // Serialize the control data
  if (SerializeTriggerControl(&file, pControlData).Succeeded())
    return PL_SUCCESS;

  return PL_FAILURE;
}

plResult plAmplitudeAudioControlsManager::CreateRtpcControl(const char* szControlName, const plAudioSystemRtpcData* pControlData)
{
  plStringBuilder sbOutputFile;
  sbOutputFile.SetFormat(":atl/Rtpcs/{0}.plAudioSystemControl", szControlName);

  plStringBuilder sbAssetPath;
  if (plFileSystem::ResolvePath(sbOutputFile, &sbAssetPath, nullptr).Failed())
    return PL_FAILURE;

  plFileWriter file;
  if (file.Open(sbAssetPath, 256).Failed())
    return PL_FAILURE;

  // Set the control type
  file << plAmplitudeAudioControlType::Rtpc;

  // Serialize the control data
  if (SerializeRtpcControl(&file, pControlData).Succeeded())
    return PL_SUCCESS;

  return PL_FAILURE;
}

plResult plAmplitudeAudioControlsManager::CreateSwitchStateControl(const char* szControlName, const plAudioSystemSwitchStateData* pControlData)
{
  plStringBuilder sbOutputFile;
  sbOutputFile.SetFormat(":atl/SwitchStates/{0}.plAudioSystemControl", szControlName);

  plStringBuilder sbAssetPath;
  if (plFileSystem::ResolvePath(sbOutputFile, &sbAssetPath, nullptr).Failed())
    return PL_FAILURE;

  plFileWriter file;
  if (file.Open(sbAssetPath, 256).Failed())
    return PL_FAILURE;

  // Set the control type
  file << plAmplitudeAudioControlType::SwitchState;

  // Serialize the control data
  if (SerializeSwitchStateControl(&file, pControlData).Succeeded())
    return PL_SUCCESS;

  return PL_FAILURE;
}

plResult plAmplitudeAudioControlsManager::CreateEnvironmentControl(const char* szControlName, const plAudioSystemEnvironmentData* pControlData)
{
  plStringBuilder sbOutputFile;
  sbOutputFile.SetFormat(":atl/Environments/{0}.plAudioSystemControl", szControlName);

  plStringBuilder sbAssetPath;
  if (plFileSystem::ResolvePath(sbOutputFile, &sbAssetPath, nullptr).Failed())
    return PL_FAILURE;

  plFileWriter file;
  if (file.Open(sbAssetPath, 256).Failed())
    return PL_FAILURE;

  // Set the control type
  file << plAmplitudeAudioControlType::Environment;

  // Serialize the control data
  if (SerializeEnvironmentControl(&file, pControlData).Succeeded())
    return PL_SUCCESS;

  return PL_FAILURE;
}

plResult plAmplitudeAudioControlsManager::CreateSoundBankControl(const char* szControlName, const plAudioSystemBankData* pControlData)
{
  plStringBuilder sbOutputFile;
  sbOutputFile.SetFormat(":atl/SoundBanks/{0}.plAudioSystemControl", szControlName);

  plStringBuilder sbAssetPath;
  if (plFileSystem::ResolvePath(sbOutputFile, &sbAssetPath, nullptr).Failed())
    return PL_FAILURE;

  plFileWriter file;
  if (file.Open(sbAssetPath, 256).Failed())
    return PL_FAILURE;

  // Set the control type
  file << plAmplitudeAudioControlType::SoundBank;

  // Serialize the control data
  if (SerializeSoundBankControl(&file, pControlData).Succeeded())
    return PL_SUCCESS;

  return PL_FAILURE;
}

plResult plAmplitudeAudioControlsManager::LoadSoundBanks(const char* sRootFolder, const char* sSubPath)
{
  plStringBuilder searchPath(sRootFolder);
  searchPath.AppendPath(sSubPath);

  plFileSystemIterator fsIt;
  for (fsIt.StartSearch(searchPath, plFileSystemIteratorFlags::ReportFilesRecursive); fsIt.IsValid(); fsIt.Next())
  {
    plStringBuilder filePath = fsIt.GetCurrentPath();
    filePath.AppendPath(fsIt.GetStats().m_sName);

    // Read the asset into a memory buffer
    plFileReader reader;
    if (!reader.Open(filePath).Succeeded())
    {
      plLog::Error("Could not open sound bank file '{0}'.", filePath);
      continue;
    }

    plJSONReader json;
    json.SetLogInterface(plLog::GetThreadLocalLogSystem());

    if (json.Parse(reader).Succeeded())
    {
      const auto& bank = json.GetTopLevelObject();
      const plVariant* name = bank.GetValue("name");
      const plVariant* id = bank.GetValue("id");

      if (!name->CanConvertTo<plString>() || !id->CanConvertTo<plUInt64>())
        return PL_FAILURE;

      const plString controlName(name->Get<plString>());

      plAmplitudeAudioSoundBankData* control = PL_AUDIOSYSTEM_NEW(plAmplitudeAudioSoundBankData, id->ConvertTo<plUInt64>(), fsIt.GetStats().m_sName);
      const plResult result = CreateSoundBankControl(controlName, control);
      PL_AUDIOSYSTEM_DELETE(control);
      if (result.Failed())
        return PL_FAILURE;
    }
    else
    {
      plLog::Error("Could not parse sound bank file '{0}'.", filePath);
      return PL_FAILURE;
    }
  }

  return PL_SUCCESS;
}

plResult plAmplitudeAudioControlsManager::LoadControlsInFolder(const char* sFolderPath, const plEnum<plAmplitudeAudioControlType>& eType)
{
  plStringBuilder const searchPath(sFolderPath);

  plFileSystemIterator fsIt;
  for (fsIt.StartSearch(searchPath, plFileSystemIteratorFlags::ReportFilesRecursive); fsIt.IsValid(); fsIt.Next())
  {
    plStringBuilder filePath = fsIt.GetCurrentPath();
    filePath.AppendPath(fsIt.GetStats().m_sName);

    // Read the asset into a memory buffer
    plFileReader reader;
    if (!reader.Open(filePath).Succeeded())
    {
      plLog::Error("Could not open control file '{0}'.", filePath);
      continue;
    }

    plJSONReader json;
    json.SetLogInterface(plLog::GetThreadLocalLogSystem());

    if (json.Parse(reader).Succeeded())
    {
      if (LoadControl(json.GetTopLevelObject(), eType).Failed())
        return PL_FAILURE;
    }
    else
    {
      plLog::Error("Could not parse control file '{0}'.", filePath);
      return PL_FAILURE;
    }
  }

  return PL_SUCCESS;
}

plResult plAmplitudeAudioControlsManager::LoadControl(const plVariantDictionary& json, const plEnum<plAmplitudeAudioControlType>& eType)
{
  if (json.Contains("name"))
  {
    const plVariant* name = json.GetValue("name");
    const plVariant* id = json.GetValue("id");

    if (!name->CanConvertTo<plString>() || !id->CanConvertTo<plUInt64>())
      return PL_FAILURE;

    const plString controlName(name->Get<plString>());

    switch (eType)
    {
      case plAmplitudeAudioControlType::Invalid:
      case plAmplitudeAudioControlType::SoundBank:
        break;

      case plAmplitudeAudioControlType::Trigger:
      {
        plAmplitudeAudioTriggerData* control = PL_AUDIOSYSTEM_NEW(plAmplitudeAudioTriggerData, id->ConvertTo<plUInt64>());
        const plResult result = CreateTriggerControl(controlName, control);
        PL_AUDIOSYSTEM_DELETE(control);
        return result;
      }

      case plAmplitudeAudioControlType::Rtpc:
      {
        plAmplitudeAudioRtpcData* control = PL_AUDIOSYSTEM_NEW(plAmplitudeAudioRtpcData, id->ConvertTo<plUInt64>());
        const plResult result = CreateRtpcControl(controlName, control);
        PL_AUDIOSYSTEM_DELETE(control);
        return result;
      }

      case plAmplitudeAudioControlType::Switch:
      {
        if (json.Contains("states"))
        {
          const plVariant* states = json.GetValue("states");

          if (!states->CanConvertTo<plVariantArray>())
            return PL_FAILURE;

          for (const auto& state : states->Get<plVariantArray>())
          {
            if (!state.CanConvertTo<plVariantDictionary>())
              continue;

            const auto& value = state.Get<plVariantDictionary>();

            plStringBuilder stateName(controlName);
            stateName.AppendFormat("_{0}", value.GetValue("name")->Get<plString>());

            plAmplitudeAudioSwitchStateData* control = PL_AUDIOSYSTEM_NEW(plAmplitudeAudioSwitchStateData, id->ConvertTo<plUInt64>(), value.GetValue("id")->ConvertTo<plUInt64>());
            const plResult result = CreateSwitchStateControl(stateName, control);
            PL_AUDIOSYSTEM_DELETE(control);

            if (result.Failed())
              return result;
          }

          return PL_SUCCESS;
        }

        return PL_FAILURE;
      }

      case plAmplitudeAudioControlType::Environment:
      {
        plAmplitudeAudioEnvironmentData* control = PL_AUDIOSYSTEM_NEW(plAmplitudeAudioEnvironmentData, id->ConvertTo<plUInt64>(), json.GetValue("effect")->ConvertTo<plUInt64>());
        const plResult result = CreateEnvironmentControl(controlName, control);
        PL_AUDIOSYSTEM_DELETE(control);
        return result;
      }

      case plAmplitudeAudioControlType::SwitchState:
        break;
    }
  }

  return PL_FAILURE;
}

#if defined(AM_WINDOWS_VERSION)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

PL_STATICLINK_FILE(EditorPluginAmplitudeAudio, EditorPluginAmplitudeAudio_AmplitudeAudioControlsManager);
