#pragma once

#include <EditorPluginAudioSystem/EditorPluginAudioSystemDLL.h>

#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/Preferences/Preferences.h>

class PL_EDITORPLUGINAUDIOSYSTEM_DLL plAudioSystemProjectPreferences : public plPreferences
{
  PL_ADD_DYNAMIC_REFLECTION(plAudioSystemProjectPreferences, plPreferences);

public:
  plAudioSystemProjectPreferences();
  ~plAudioSystemProjectPreferences() override;

  /// \brief Mutes or unmute the audio system.
  /// \param bMute The mute state to apply.
  void SetMute(bool bMute);

  /// \brief Gets the current mute state of the audio system.
  /// \returns The audio system's mute state.
  PL_NODISCARD bool GetMute() const { return m_bMute; }

  /// \brief Sets the audio system's gain.
  /// \param fGain The gain value.
  void SetGain(float fGain);

  /// \brief Gets the current gain value of the audio system.
  /// \returns The audio system's gain.
  PL_NODISCARD float GetGain() const { return m_fGain; }

  /// \brief Synchronise the preferences with the current CVars
  /// values.
  void SyncCVars();

private:
  void ProcessEventHandler(const plEditorEngineProcessConnection::Event& e);

  bool m_bMute{false};
  float m_fGain{1.0f};
};
