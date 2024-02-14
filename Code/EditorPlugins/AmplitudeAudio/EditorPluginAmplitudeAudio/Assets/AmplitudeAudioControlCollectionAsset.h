#pragma once

#include <AmplitudeAudioPlugin/Resources/AudioControlCollectionResource.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class plAmplitudeAudioControlCollectionAssetEntry : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioControlCollectionAssetEntry, plReflectedClass);

public:
  plString m_sName;
  plEnum<plAmplitudeAudioControlType> m_Type;
  plString m_sControlFile;
};

class plAmplitudeAudioControlCollectionAssetTriggerEntry : public plAmplitudeAudioControlCollectionAssetEntry
{
  PL_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioControlCollectionAssetTriggerEntry, plAmplitudeAudioControlCollectionAssetEntry);

public:
  plAmplitudeAudioControlCollectionAssetTriggerEntry();
};

class plAmplitudeAudioControlCollectionAssetRtpcEntry : public plAmplitudeAudioControlCollectionAssetEntry
{
  PL_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioControlCollectionAssetRtpcEntry, plAmplitudeAudioControlCollectionAssetEntry);

public:
  plAmplitudeAudioControlCollectionAssetRtpcEntry();
};

class plAmplitudeAudioControlCollectionAssetSwitchEntry : public plAmplitudeAudioControlCollectionAssetEntry
{
  PL_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioControlCollectionAssetSwitchEntry, plAmplitudeAudioControlCollectionAssetEntry);

public:
  plAmplitudeAudioControlCollectionAssetSwitchEntry();
};

class plAmplitudeAudioControlCollectionAssetEnvironmentEntry : public plAmplitudeAudioControlCollectionAssetEntry
{
  PL_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioControlCollectionAssetEnvironmentEntry, plAmplitudeAudioControlCollectionAssetEntry);

public:
  plAmplitudeAudioControlCollectionAssetEnvironmentEntry();
};

class plAmplitudeAudioControlCollectionAssetSoundBankEntry : public plAmplitudeAudioControlCollectionAssetEntry
{
  PL_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioControlCollectionAssetSoundBankEntry, plAmplitudeAudioControlCollectionAssetEntry);

public:
  plAmplitudeAudioControlCollectionAssetSoundBankEntry();
};

class plAmplitudeAudioControlCollectionAsset : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioControlCollectionAsset, plReflectedClass);

public:
  plDynamicArray<plAmplitudeAudioControlCollectionAssetTriggerEntry> m_TriggerEntries;
  plDynamicArray<plAmplitudeAudioControlCollectionAssetRtpcEntry> m_RtpcEntries;
  plDynamicArray<plAmplitudeAudioControlCollectionAssetSwitchEntry> m_SwitchEntries;
  plDynamicArray<plAmplitudeAudioControlCollectionAssetEnvironmentEntry> m_EnvironmentEntries;
  plDynamicArray<plAmplitudeAudioControlCollectionAssetSoundBankEntry> m_SoundBankEntries;
};

class plAmplitudeAudioControlCollectionAssetDocument : public plSimpleAssetDocument<plAmplitudeAudioControlCollectionAsset>
{
  PL_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioControlCollectionAssetDocument, plSimpleAssetDocument<plAmplitudeAudioControlCollectionAsset>);

public:
  plAmplitudeAudioControlCollectionAssetDocument(plStringView sDocumentPath);

protected:
  void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
  plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

private:
  plResult TransformAssetEntry(const plAmplitudeAudioControlCollectionAssetEntry& entry, plAmplitudeAudioControlCollectionResourceDescriptor& descriptor) const;
};
