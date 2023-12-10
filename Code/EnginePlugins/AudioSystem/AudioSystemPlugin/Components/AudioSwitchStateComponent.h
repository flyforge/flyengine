#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>
#include <AudioSystemPlugin/Core/AudioSystemData.h>
#include <AudioSystemPlugin/Core/AudioSystemMessages.h>

typedef plComponentManager<class plAudioSwitchStateComponent, plBlockStorageType::FreeList> plAudioSwitchStateComponentManager;

/// \brief Component used to set the current state of a switch in the audio middleware.
class PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSwitchStateComponent : public plAudioSystemProxyDependentComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plAudioSwitchStateComponent, plAudioSystemProxyDependentComponent, plAudioSwitchStateComponentManager);

  // plComponent

public:
  void Initialize() override;
  void SerializeComponent(plWorldWriter& stream) const override;
  void DeserializeComponent(plWorldReader& stream) override;

  // plAudioSystemComponent

private:
  void plAudioSystemComponentIsAbstract() override {}

  // plAudioSwitchStateComponent

public:
  plAudioSwitchStateComponent();
  ~plAudioSwitchStateComponent() override;

  /// \brief Sets the active state of the switch. This will send a request to the Audio System.
  ///
  /// \param sSwitchStateName The name of the new state the switch should have.
  /// \param bSync Whether the request should be sent synchronously or asynchronously.
  void SetState(const plString& sSwitchStateName, bool bSync = false);

  /// \brief Gets the current state of the switch.
  /// \returns The current state of the switch.
  PLASMA_NODISCARD const plString& GetState() const;

  /// \brief Event that is triggered when the component receives a
  /// SetSwitchState message.
  void OnSetState(plMsgAudioSystemSetSwitchState& msg);

private:
  plString m_sCurrentSwitchState;
  plString m_sInitialSwitchState;

  plEventMessageSender<plMsgAudioSystemSwitchStateChanged> m_ValueChangedEventSender;
};
