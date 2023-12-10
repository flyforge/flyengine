#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/ATL/AudioTranslationLayerData.h>
#include <AudioSystemPlugin/Core/AudioMiddleware.h>
#include <AudioSystemPlugin/Core/AudioThread.h>

struct plCVarEvent;

/// \brief The Audio Translation Layer
///
/// This class is the bridge between the audio system and the audio middleware
/// and it is responsible of the execution of audio requests. It also stores the
/// registered audio controls so they can be retrieved during runtime by their names.
class PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioTranslationLayer
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plAudioTranslationLayer);

public:
  plAudioTranslationLayer();
  ~plAudioTranslationLayer();

  /// \brief Initializes the audio translation layer.
  PLASMA_NODISCARD plResult Startup();

  /// \brief Shuts down the audio translation layer.
  void Shutdown();

  /// \brief Updates the audio translation layer.
  /// This will also trigger an update of the audio middleware.
  void Update();

  /// \brief Returns the ID of a trigger control identified by its name.
  /// \param sTriggerName The name of the trigger control.
  /// \returns The ID of the trigger, or 0 if a trigger control with the given name
  /// is not registered.
  PLASMA_NODISCARD plAudioSystemDataID GetTriggerId(plStringView sTriggerName) const;

  /// \brief Returns the ID of a real-time parameter control identified by its name.
  /// \param sRtpcName The name of the RTPC.
  /// \returns The ID of the RTPC, or 0 if a RTPC with the given name is not registered.
  PLASMA_NODISCARD plAudioSystemDataID GetRtpcId(plStringView sRtpcName) const;

  /// \brief Returns the ID of a switch state control identified by its name.
  /// \param sSwitchStateName The name of the switch state control.
  /// \returns The ID of the switch state, or 0 if a switch state control with the given name
  /// is not registered.
  PLASMA_NODISCARD plAudioSystemDataID GetSwitchStateId(plStringView sSwitchStateName) const;

  /// \brief Returns the ID of an environment control identified by its name.
  /// \param sEnvironmentName The name of the environment control.
  /// \returns The ID of the environment, or 0 if an environment control with the given name
  /// is not registered.
  PLASMA_NODISCARD plAudioSystemDataID GetEnvironmentId(plStringView sEnvironmentName) const;

private:
  friend class plAudioSystem;

  bool ProcessRequest(plVariant&& request, bool bSync);
  void OnMasterGainChange(const plCVarEvent& e) const;
  void OnMuteChange(const plCVarEvent& e) const;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  void DebugRender() const;
#endif

  // ATLObject containers
  plATLEntityLookup m_mEntities;
  plATLListenerLookup m_mListeners;
  plATLTriggerLookup m_mTriggers;
  plATLRtpcLookup m_mRtpcs;
  plATLSwitchStateLookup m_mSwitchStates;
  plATLEnvironmentLookup m_mEnvironments;
  plATLSoundBankLookup m_mSoundBanks;

  plTime m_LastUpdateTime;
  plTime m_LastFrameTime;

  plAudioMiddleware* m_pAudioMiddleware{nullptr};
};
