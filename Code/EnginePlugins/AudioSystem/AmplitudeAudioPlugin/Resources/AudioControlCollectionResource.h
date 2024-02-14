#pragma once

#include <AmplitudeAudioPlugin/AmplitudeAudioPluginDLL.h>

#include <AmplitudeAudioPlugin/Core/AmplitudeAudioData.h>

#include <Core/ResourceManager/Resource.h>

using plAudioControlCollectionResourceHandle = plTypedResourceHandle<class plAmplitudeAudioControlCollectionResource>;

/// \brief Represents one audio control, used by a single audio middleware.
struct PL_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeAudioControlCollectionEntry
{
  plString m_sName;                                               ///< Optional, can be used to lookup the resource at runtime with a nice name. E.g. "SkyTexture" instead of some GUID.
  plString m_sControlFile;                                        ///< The path to the audio system control.
  plDefaultMemoryStreamStorage* m_pControlBufferStorage{nullptr}; ///< Buffer storage that contains the control data. Only have a value for loaded resources.
  plEnum<plAmplitudeAudioControlType> m_Type;                     ///< The type of the control.
};

/// \brief Describes a full plAmplitudeAudioControlCollectionResource, ie. lists all the controls that the collection contains.
struct PL_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeAudioControlCollectionResourceDescriptor
{
  plDynamicArray<plAmplitudeAudioControlCollectionEntry> m_Entries;

  void Save(plStreamWriter& stream) const;
  void Load(plStreamReader& stream);
};

/// \brief An plAmplitudeAudioControlCollectionResource is used by the audio system to map a collection of audio controls to a single audio middleware.
///
/// For each audio control should specify the control name, the control type, and the path to the generated middleware control file.
/// Those controls will now be able to be used by the audio system through components.
class PL_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeAudioControlCollectionResource : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioControlCollectionResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plAmplitudeAudioControlCollectionResource);
  PL_RESOURCE_DECLARE_CREATEABLE(plAudioControlCollectionResource, plAmplitudeAudioControlCollectionResourceDescriptor);

public:
  plAmplitudeAudioControlCollectionResource();

  /// \brief Registers this collection to the audio system.
  ///
  /// \note This is called automatically at initialization by the audio system on the control collection
  /// asset having the same name than the current audio middleware.
  ///
  /// Calling this twice has no effect.
  void Register();

  /// \brief Removes the registered controls from the audio system.
  ///
  /// Calling this twice has no effect.
  void Unregister();

  /// \brief Returns the resource descriptor for this resource.
  PL_NODISCARD const plAmplitudeAudioControlCollectionResourceDescriptor& GetDescriptor() const;

private:
  void RegisterTrigger(const char* szTriggerName, const char* szControlFile);
  void RegisterTrigger(const char* szTriggerName, plStreamReader* pStreamReader);
  void UnregisterTrigger(const char* szTriggerName);

  void RegisterRtpc(const char* szRtpcName, const char* szControlFile);
  void RegisterRtpc(const char* szRtpcName, plStreamReader* pStreamReader);
  void UnregisterRtpc(const char* szRtpcName);

  void RegisterSwitchState(const char* szSwitchStateName, const char* szControlFile);
  void RegisterSwitchState(const char* szSwitchStateName, plStreamReader* pStreamReader);
  void UnregisterSwitchState(const char* szSwitchStateName);

  void RegisterEnvironment(const char* szEnvironmentName, const char* szControlFile);
  void RegisterEnvironment(const char* szEnvironmentName, plStreamReader* pStreamReader);
  void UnregisterEnvironment(const char* szEnvironmentName);

  void RegisterSoundBank(const char* szBankName, const char* szControlFile);
  void RegisterSoundBank(const char* szBankName, plStreamReader* pStreamReader);
  void UnregisterSoundBank(const char* szBankName);

  plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  plResourceLoadDesc UpdateContent(plStreamReader* pStream) override;
  void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  bool m_bRegistered = false;
  plAmplitudeAudioControlCollectionResourceDescriptor m_Collection;
};
