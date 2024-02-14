#include <EditorPluginAmplitudeAudio/EditorPluginAmplitudeAudioPCH.h>
#include <EditorPluginAmplitudeAudio/Assets/AmplitudeAudioControlCollectionAsset.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/OSFile.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioControlCollectionAssetEntry, 1, plRTTIDefaultAllocator<plAmplitudeAudioControlCollectionAssetEntry>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sName),
    PL_ENUM_MEMBER_PROPERTY("Type", plAmplitudeAudioControlType, m_Type)->AddAttributes(new plDefaultValueAttribute(plAmplitudeAudioControlType::Invalid), new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("Control", m_sControlFile)->AddAttributes(new plFileBrowserAttribute("Select Audio System Control", "*.plAudioSystemControl")),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioControlCollectionAssetTriggerEntry, 1, plRTTIDefaultAllocator<plAmplitudeAudioControlCollectionAssetTriggerEntry>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioControlCollectionAssetRtpcEntry, 1, plRTTIDefaultAllocator<plAmplitudeAudioControlCollectionAssetRtpcEntry>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioControlCollectionAssetSwitchEntry, 1, plRTTIDefaultAllocator<plAmplitudeAudioControlCollectionAssetSwitchEntry>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioControlCollectionAssetEnvironmentEntry, 1, plRTTIDefaultAllocator<plAmplitudeAudioControlCollectionAssetEnvironmentEntry>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioControlCollectionAssetSoundBankEntry, 1, plRTTIDefaultAllocator<plAmplitudeAudioControlCollectionAssetSoundBankEntry>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioControlCollectionAsset, 2, plRTTIDefaultAllocator<plAmplitudeAudioControlCollectionAsset>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("Triggers", m_TriggerEntries),
    PL_ARRAY_MEMBER_PROPERTY("RTPCs", m_RtpcEntries),
    PL_ARRAY_MEMBER_PROPERTY("SwitchStates", m_SwitchEntries),
    PL_ARRAY_MEMBER_PROPERTY("Environments", m_EnvironmentEntries),
    PL_ARRAY_MEMBER_PROPERTY("SoundBanks", m_SoundBankEntries),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioControlCollectionAssetDocument, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAmplitudeAudioControlCollectionAssetTriggerEntry::plAmplitudeAudioControlCollectionAssetTriggerEntry()
  : plAmplitudeAudioControlCollectionAssetEntry()
{
  m_Type = plAmplitudeAudioControlType::Trigger;
}

plAmplitudeAudioControlCollectionAssetRtpcEntry::plAmplitudeAudioControlCollectionAssetRtpcEntry()
  : plAmplitudeAudioControlCollectionAssetEntry()
{
  m_Type = plAmplitudeAudioControlType::Rtpc;
}

plAmplitudeAudioControlCollectionAssetSwitchEntry::plAmplitudeAudioControlCollectionAssetSwitchEntry()
{
  m_Type = plAmplitudeAudioControlType::SwitchState;
}

plAmplitudeAudioControlCollectionAssetEnvironmentEntry::plAmplitudeAudioControlCollectionAssetEnvironmentEntry()
{
  m_Type = plAmplitudeAudioControlType::Environment;
}

plAmplitudeAudioControlCollectionAssetSoundBankEntry::plAmplitudeAudioControlCollectionAssetSoundBankEntry()
{
  m_Type = plAmplitudeAudioControlType::SoundBank;
}

plAmplitudeAudioControlCollectionAssetDocument::plAmplitudeAudioControlCollectionAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plAmplitudeAudioControlCollectionAsset>(sDocumentPath, plAssetDocEngineConnection::None)
{
}

void plAmplitudeAudioControlCollectionAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const plAmplitudeAudioControlCollectionAsset* pProp = GetProperties();

  for (const auto& e : pProp->m_TriggerEntries)
  {
    if (!e.m_sControlFile.IsEmpty())
      pInfo->m_TransformDependencies.Insert(e.m_sControlFile);
  }

  for (const auto& e : pProp->m_RtpcEntries)
  {
    if (!e.m_sControlFile.IsEmpty())
      pInfo->m_TransformDependencies.Insert(e.m_sControlFile);
  }

  for (const auto& e : pProp->m_SwitchEntries)
  {
    if (!e.m_sControlFile.IsEmpty())
      pInfo->m_TransformDependencies.Insert(e.m_sControlFile);
  }

  for (const auto& e : pProp->m_EnvironmentEntries)
  {
    if (!e.m_sControlFile.IsEmpty())
      pInfo->m_TransformDependencies.Insert(e.m_sControlFile);
  }

  for (const auto& e : pProp->m_SoundBankEntries)
  {
    if (!e.m_sControlFile.IsEmpty())
      pInfo->m_TransformDependencies.Insert(e.m_sControlFile);
  }
}

plTransformStatus plAmplitudeAudioControlCollectionAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plAmplitudeAudioControlCollectionAsset* pProp = GetProperties();

  plAmplitudeAudioControlCollectionResourceDescriptor descriptor;

  for (auto& e : pProp->m_TriggerEntries)
  {
    e.m_Type = plAmplitudeAudioControlType::Trigger;
    TransformAssetEntry(e, descriptor).IgnoreResult();
  }

  for (auto& e : pProp->m_RtpcEntries)
  {
    e.m_Type = plAmplitudeAudioControlType::Rtpc;
    TransformAssetEntry(e, descriptor).IgnoreResult();
  }

  for (auto& e : pProp->m_SwitchEntries)
  {
    e.m_Type = plAmplitudeAudioControlType::SwitchState;
    TransformAssetEntry(e, descriptor).IgnoreResult();
  }

  for (auto& e : pProp->m_EnvironmentEntries)
  {
    e.m_Type = plAmplitudeAudioControlType::Environment;
    TransformAssetEntry(e, descriptor).IgnoreResult();
  }

  for (auto& e : pProp->m_SoundBankEntries)
  {
    e.m_Type = plAmplitudeAudioControlType::SoundBank;
    TransformAssetEntry(e, descriptor).IgnoreResult();
  }

  descriptor.Save(stream);
  return {PL_SUCCESS};
}

plResult plAmplitudeAudioControlCollectionAssetDocument::TransformAssetEntry(const plAmplitudeAudioControlCollectionAssetEntry& entry, plAmplitudeAudioControlCollectionResourceDescriptor& descriptor) const
{
  if (entry.m_sControlFile.IsEmpty() || entry.m_Type == plAmplitudeAudioControlType::Invalid)
    return PL_FAILURE;

  {
    plStringBuilder sAssetFile = entry.m_sControlFile;
    if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAssetFile))
    {
      plLog::Warning("Failed to make audio control path absolute: '{0}'", entry.m_sControlFile);
      return PL_FAILURE;
    }
  }

  plAmplitudeAudioControlCollectionEntry e;
  e.m_sName = entry.m_sName;
  e.m_Type = entry.m_Type;
  e.m_sControlFile = entry.m_sControlFile;

  descriptor.m_Entries.PushBack(e);
  return PL_SUCCESS;
}

#if defined(AM_WINDOWS_VERSION)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

PL_STATICLINK_FILE(EditorPluginAmplitudeAudio, EditorPluginAmplitudeAudio_Assets_AmplitudeAudioControlCollectionAsset);
