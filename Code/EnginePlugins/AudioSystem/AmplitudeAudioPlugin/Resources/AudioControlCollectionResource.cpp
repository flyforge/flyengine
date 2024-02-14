#include <AmplitudeAudioPlugin/AmplitudeAudioPluginPCH.h>

#include <AmplitudeAudioPlugin/AmplitudeAudioSingleton.h>
#include <AmplitudeAudioPlugin/Resources/AudioControlCollectionResource.h>

#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioControlCollectionResource, 1, plRTTIDefaultAllocator<plAmplitudeAudioControlCollectionResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plAmplitudeAudioControlCollectionResource);

void plAmplitudeAudioControlCollectionResourceDescriptor::Save(plStreamWriter& stream) const
{
  constexpr plUInt8 uiVersion = 1;
  constexpr plUInt8 uiIdentifier = 0xAC;
  const plUInt32 uiNumResources = m_Entries.GetCount();

  stream << uiVersion;
  stream << uiIdentifier;
  stream << uiNumResources;

  for (plUInt32 i = 0; i < uiNumResources; ++i)
  {
    plDefaultMemoryStreamStorage storage(0, plAudioSystemAllocatorWrapper::GetAllocator());

    {
      plFileReader file;
      if (file.Open(m_Entries[i].m_sControlFile).Failed())
      {
        plLog::Error("Could not open audio control file '{0}'", m_Entries[i].m_sControlFile);
        continue;
      }

      plMemoryStreamWriter writer(&storage);
      plUInt8 Temp[4 * 1024];
      while (true)
      {
        const plUInt64 uiRead = file.ReadBytes(Temp, PL_ARRAY_SIZE(Temp));

        if (uiRead == 0)
          break;

        writer.WriteBytes(Temp, uiRead).IgnoreResult();
      }
    }

    stream << m_Entries[i].m_sName;
    stream << m_Entries[i].m_Type;
    stream << m_Entries[i].m_sControlFile;
    stream << storage.GetStorageSize32();
    storage.CopyToStream(stream).IgnoreResult();
  }
}

void plAmplitudeAudioControlCollectionResourceDescriptor::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  plUInt8 uiIdentifier = 0;
  plUInt32 uiNumResources = 0;

  stream >> uiVersion;
  stream >> uiIdentifier;
  stream >> uiNumResources;

  PL_ASSERT_DEV(uiIdentifier == 0xAC, "File does not contain a valid plAmplitudeAudioControlCollectionResourceDescriptor");
  PL_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  m_Entries.SetCount(uiNumResources);

  for (plUInt32 i = 0; i < uiNumResources; ++i)
  {
    plUInt32 uiSize = 0;

    stream >> m_Entries[i].m_sName;
    stream >> m_Entries[i].m_Type;
    stream >> m_Entries[i].m_sControlFile;
    stream >> uiSize;

    m_Entries[i].m_pControlBufferStorage = PL_AUDIOSYSTEM_NEW(plDefaultMemoryStreamStorage, uiSize, plAudioSystemAllocatorWrapper::GetAllocator());
    m_Entries[i].m_pControlBufferStorage->ReadAll(stream, uiSize);
  }
}


PL_RESOURCE_IMPLEMENT_CREATEABLE(plAmplitudeAudioControlCollectionResource, plAmplitudeAudioControlCollectionResourceDescriptor)
{
  m_Collection = descriptor;

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

plAmplitudeAudioControlCollectionResource::plAmplitudeAudioControlCollectionResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

void plAmplitudeAudioControlCollectionResource::Register()
{
  if (m_bRegistered)
    return;

  for (const auto& entry : m_Collection.m_Entries)
  {
    if (entry.m_sName.IsEmpty() || entry.m_pControlBufferStorage == nullptr)
      continue;

    plMemoryStreamReader reader(entry.m_pControlBufferStorage);

    switch (entry.m_Type)
    {
      case plAmplitudeAudioControlType::Switch:
      case plAmplitudeAudioControlType::Invalid:
        plLog::Error("Unable to register an audio control. Encountered an invalid control type");
        break;

      case plAmplitudeAudioControlType::Trigger:
        RegisterTrigger(entry.m_sName, &reader);
        break;

      case plAmplitudeAudioControlType::Rtpc:
        RegisterRtpc(entry.m_sName, &reader);
        break;

      case plAmplitudeAudioControlType::SwitchState:
        RegisterSwitchState(entry.m_sName, &reader);
        break;

      case plAmplitudeAudioControlType::Environment:
        RegisterEnvironment(entry.m_sName, &reader);
        break;

      case plAmplitudeAudioControlType::SoundBank:
        RegisterSoundBank(entry.m_sName, &reader);
        break;
    }
  }
}

void plAmplitudeAudioControlCollectionResource::Unregister()
{
  if (!m_bRegistered)
    return;

  for (const auto& entry : m_Collection.m_Entries)
  {
    if (entry.m_sName.IsEmpty() || entry.m_pControlBufferStorage == nullptr)
      continue;

    switch (entry.m_Type)
    {
      case plAmplitudeAudioControlType::Invalid:
      case plAmplitudeAudioControlType::Switch:
        plLog::Error("Unable to unregister an audio control. Encountered an invalid control type.");
        break;

      case plAmplitudeAudioControlType::Trigger:
        UnregisterTrigger(entry.m_sName);
        break;

      case plAmplitudeAudioControlType::Rtpc:
        UnregisterRtpc(entry.m_sName);
        break;

      case plAmplitudeAudioControlType::SwitchState:
        UnregisterSwitchState(entry.m_sName);
        break;

      case plAmplitudeAudioControlType::Environment:
        UnregisterEnvironment(entry.m_sName);
        break;

      case plAmplitudeAudioControlType::SoundBank:
        UnregisterSoundBank(entry.m_sName);
        break;
    }
  }
}

const plAmplitudeAudioControlCollectionResourceDescriptor& plAmplitudeAudioControlCollectionResource::GetDescriptor() const
{
  return m_Collection;
}

void plAmplitudeAudioControlCollectionResource::RegisterTrigger(const char* szTriggerName, const char* szControlFile)
{
  if (!plAudioSystem::GetSingleton()->IsInitialized())
    return;

  plFileReader file;
  if (!file.Open(szControlFile, 256).Succeeded())
  {
    plLog::Error("Unable to register a trigger in the audio system: Could not open trigger control file '{0}'", szControlFile);
    return;
  }

  RegisterTrigger(szTriggerName, &file);
}

void plAmplitudeAudioControlCollectionResource::RegisterTrigger(const char* szTriggerName, plStreamReader* pStreamReader)
{
  auto* pAudioSystem = plAudioSystem::GetSingleton();
  if (!pAudioSystem->IsInitialized())
    return;

  auto* pAudioMiddleware = plAmplitude::GetSingleton();
  if (pAudioMiddleware == nullptr)
  {
    plLog::Error("Unable to register a trigger in the audio system: No audio middleware currently running.");
    return;
  }

  plEnum<plAmplitudeAudioControlType> type;
  *pStreamReader >> type;

  if (type != plAmplitudeAudioControlType::Trigger)
  {
    plLog::Error("Unable to register a trigger in the audio system: The control have an invalid file.");
    return;
  }

  plAudioSystemTriggerData* pTriggerData = pAudioMiddleware->DeserializeTriggerEntry(pStreamReader);
  if (pTriggerData == nullptr)
  {
    plLog::Error("Unable to register a trigger in the audio system: Could not deserialize control.");
    return;
  }

  const plUInt32 uiTriggerId = plHashHelper<const char*>::Hash(szTriggerName);
  pAudioSystem->RegisterTrigger(uiTriggerId, pTriggerData);
}

void plAmplitudeAudioControlCollectionResource::UnregisterTrigger(const char* szTriggerName)
{
  auto* pAudioSystem = plAudioSystem::GetSingleton();
  if (!pAudioSystem->IsInitialized())
    return;

  const plUInt32 uiTriggerId = plHashHelper<const char*>::Hash(szTriggerName);
  pAudioSystem->UnregisterTrigger(uiTriggerId);
}

void plAmplitudeAudioControlCollectionResource::RegisterRtpc(const char* szRtpcName, const char* szControlFile)
{
  if (!plAudioSystem::GetSingleton()->IsInitialized())
    return;

  plFileReader file;
  if (!file.Open(szControlFile, 256).Succeeded())
  {
    plLog::Error("Unable to register a rtpc in the audio system: Could not open control file '{0}'", szControlFile);
    return;
  }

  RegisterRtpc(szRtpcName, &file);
}

void plAmplitudeAudioControlCollectionResource::RegisterRtpc(const char* szRtpcName, plStreamReader* pStreamReader)
{
  auto* pAudioSystem = plAudioSystem::GetSingleton();
  if (!pAudioSystem->IsInitialized())
    return;

  auto* pAudioMiddleware = plAmplitude::GetSingleton();
  if (pAudioMiddleware == nullptr)
  {
    plLog::Error("Unable to register a rtpc in the audio system: No audio middleware currently running.");
    return;
  }

  plEnum<plAmplitudeAudioControlType> type;
  *pStreamReader >> type;

  if (type != plAmplitudeAudioControlType::Rtpc)
  {
    plLog::Error("Unable to register a rtpc in the audio system: The control have an invalid file.");
    return;
  }

  plAudioSystemRtpcData* pSystemRtpcData = pAudioMiddleware->DeserializeRtpcEntry(pStreamReader);
  if (pSystemRtpcData == nullptr)
  {
    plLog::Error("Unable to register a rtpc in the audio system: Could not deserialize control.");
    return;
  }

  const plUInt32 uiRtpcId = plHashHelper<const char*>::Hash(szRtpcName);
  pAudioSystem->RegisterRtpc(uiRtpcId, pSystemRtpcData);
}

void plAmplitudeAudioControlCollectionResource::UnregisterRtpc(const char* szRtpcName)
{
  auto* pAudioSystem = plAudioSystem::GetSingleton();
  if (!pAudioSystem->IsInitialized())
    return;

  const plUInt32 uiRtpcId = plHashHelper<const char*>::Hash(szRtpcName);
  pAudioSystem->UnregisterRtpc(uiRtpcId);
}

void plAmplitudeAudioControlCollectionResource::RegisterSwitchState(const char* szSwitchStateName, const char* szControlFile)
{
  if (!plAudioSystem::GetSingleton()->IsInitialized())
    return;

  plFileReader file;
  if (!file.Open(szControlFile, 256).Succeeded())
  {
    plLog::Error("Unable to register a switch state in the audio system: Could not open control file '{0}'", szControlFile);
    return;
  }

  RegisterSwitchState(szSwitchStateName, &file);
}

void plAmplitudeAudioControlCollectionResource::RegisterSwitchState(const char* szSwitchStateName, plStreamReader* pStreamReader)
{
  auto* pAudioSystem = plAudioSystem::GetSingleton();
  if (!pAudioSystem->IsInitialized())
    return;

  auto* pAudioMiddleware = plAmplitude::GetSingleton();
  if (pAudioMiddleware == nullptr)
  {
    plLog::Error("Unable to register a switch state in the audio system: No audio middleware currently running.");
    return;
  }

  plEnum<plAmplitudeAudioControlType> type;
  *pStreamReader >> type;

  if (type != plAmplitudeAudioControlType::SwitchState)
  {
    plLog::Error("Unable to register a switch state in the audio system: The control have an invalid file.");
    return;
  }

  plAudioSystemSwitchStateData* pSwitchStateData = pAudioMiddleware->DeserializeSwitchStateEntry(pStreamReader);
  if (pSwitchStateData == nullptr)
  {
    plLog::Error("Unable to register a switch state in the audio system: Could not deserialize control.");
    return;
  }

  const plUInt32 uiSwitchStateId = plHashHelper<const char*>::Hash(szSwitchStateName);
  pAudioSystem->RegisterSwitchState(uiSwitchStateId, pSwitchStateData);
}

void plAmplitudeAudioControlCollectionResource::UnregisterSwitchState(const char* szSwitchStateName)
{
  auto* pAudioSystem = plAudioSystem::GetSingleton();
  if (!pAudioSystem->IsInitialized())
    return;

  const plUInt32 uiSwitchStateId = plHashHelper<const char*>::Hash(szSwitchStateName);
  pAudioSystem->UnregisterSwitchState(uiSwitchStateId);
}

void plAmplitudeAudioControlCollectionResource::RegisterEnvironment(const char* szEnvironmentName, const char* szControlFile)
{
  if (!plAudioSystem::GetSingleton()->IsInitialized())
    return;

  plFileReader file;
  if (!file.Open(szControlFile, 256).Succeeded())
  {
    plLog::Error("Unable to register an environment in the audio system: Could not open control file '{0}'", szControlFile);
    return;
  }

  RegisterSwitchState(szEnvironmentName, &file);
}

void plAmplitudeAudioControlCollectionResource::RegisterEnvironment(const char* szEnvironmentName, plStreamReader* pStreamReader)
{
  auto* pAudioSystem = plAudioSystem::GetSingleton();
  if (!pAudioSystem->IsInitialized())
    return;

  auto* pAudioMiddleware = plAmplitude::GetSingleton();
  if (pAudioMiddleware == nullptr)
  {
    plLog::Error("Unable to register an environment in the audio system: No audio middleware currently running.");
    return;
  }

  plEnum<plAmplitudeAudioControlType> type;
  *pStreamReader >> type;

  if (type != plAmplitudeAudioControlType::Environment)
  {
    plLog::Error("Unable to register an environment in the audio system: The control have an invalid file.");
    return;
  }

  plAudioSystemEnvironmentData* pEnvironmentData = pAudioMiddleware->DeserializeEnvironmentEntry(pStreamReader);
  if (pEnvironmentData == nullptr)
  {
    plLog::Error("Unable to register an environment in the audio system: Could not deserialize control.");
    return;
  }

  const plUInt32 uiEnvironmentId = plHashHelper<const char*>::Hash(szEnvironmentName);
  pAudioSystem->RegisterEnvironment(uiEnvironmentId, pEnvironmentData);
}

void plAmplitudeAudioControlCollectionResource::UnregisterEnvironment(const char* szEnvironmentName)
{
  auto* pAudioSystem = plAudioSystem::GetSingleton();
  if (!pAudioSystem->IsInitialized())
    return;

  const plUInt32 uiEnvironmentId = plHashHelper<const char*>::Hash(szEnvironmentName);
  pAudioSystem->UnregisterEnvironment(uiEnvironmentId);
}

void plAmplitudeAudioControlCollectionResource::RegisterSoundBank(const char* szBankName, const char* szControlFile)
{
  if (!plAudioSystem::GetSingleton()->IsInitialized())
    return;

  plFileReader file;
  if (!file.Open(szBankName, 256).Succeeded())
  {
    plLog::Error("Unable to register a sound bank in the audio system: Could not open control file '{0}'", szControlFile);
    return;
  }

  RegisterSoundBank(szBankName, &file);
}

void plAmplitudeAudioControlCollectionResource::RegisterSoundBank(const char* szBankName, plStreamReader* pStreamReader)
{
  auto* pAudioSystem = plAudioSystem::GetSingleton();
  if (!pAudioSystem->IsInitialized())
    return;

  auto* pAudioMiddleware = plAmplitude::GetSingleton();
  if (pAudioMiddleware == nullptr)
  {
    plLog::Error("Unable to register a sound bank in the audio system: No audio middleware currently running.");
    return;
  }

  plEnum<plAmplitudeAudioControlType> type;
  *pStreamReader >> type;

  if (type != plAmplitudeAudioControlType::SoundBank)
  {
    plLog::Error("Unable to register a sound bank in the audio system: The control have an invalid file.");
    return;
  }

  plAudioSystemBankData* pSoundBankData = pAudioMiddleware->DeserializeBankEntry(pStreamReader);
  if (pSoundBankData == nullptr)
  {
    plLog::Error("Unable to register a sound bank in the audio system: Could not deserialize control.");
    return;
  }

  const plUInt32 uiSoundBankId = plHashHelper<const char*>::Hash(szBankName);
  pAudioSystem->RegisterSoundBank(uiSoundBankId, pSoundBankData);
}

void plAmplitudeAudioControlCollectionResource::UnregisterSoundBank(const char* szBankName)
{
  auto* pAudioSystem = plAudioSystem::GetSingleton();
  if (!pAudioSystem->IsInitialized())
    return;

  const plUInt32 uiSoundBankId = plHashHelper<const char*>::Hash(szBankName);
  pAudioSystem->UnregisterSoundBank(uiSoundBankId);
}

plResourceLoadDesc plAmplitudeAudioControlCollectionResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  {
    Unregister();

    for (auto& entry : m_Collection.m_Entries)
    {
      if (entry.m_pControlBufferStorage != nullptr)
      {
        entry.m_pControlBufferStorage->Clear();
        PL_AUDIOSYSTEM_DELETE(entry.m_pControlBufferStorage);
      }
    }

    m_Collection.m_Entries.Clear();
    m_Collection.m_Entries.Compact();
  }

  return res;
}

plResourceLoadDesc plAmplitudeAudioControlCollectionResource::UpdateContent(plStreamReader* pStream)
{
  PL_LOG_BLOCK("plAmplitudeAudioControlCollectionResource::UpdateContent", GetResourceDescription().GetData());

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (pStream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  // Skip the absolute file path data that the standard file reader writes into the stream
  {
    plStringBuilder sAbsFilePath;
    *pStream >> sAbsFilePath;
  }

  // Skip the asset file header at the start of the file
  {
    plAssetFileHeader AssetHash;
    AssetHash.Read(*pStream).IgnoreResult();
  }

  // Load the asset file
  m_Collection.Load(*pStream);

  // Register asset controls in the audio system
  Register();

  res.m_State = plResourceState::Loaded;
  return res;
}

void plAmplitudeAudioControlCollectionResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = m_Collection.m_Entries.GetHeapMemoryUsage();
}

#if defined(AM_WINDOWS_VERSION)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

PL_STATICLINK_FILE(AmplitudeAudioPlugin, AmplitudeAudioPlugin_Resources_AudioControlCollectionResource);
