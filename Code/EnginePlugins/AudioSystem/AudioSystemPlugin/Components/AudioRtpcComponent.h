#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>
#include <AudioSystemPlugin/Core/AudioSystemData.h>
#include <AudioSystemPlugin/Core/AudioSystemMessages.h>

typedef plComponentManager<class plAudioRtpcComponent, plBlockStorageType::FreeList> plAudioRtpcComponentManager;

/// \brief Component used to set the value of a real-time parameter in the audio middleware.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioRtpcComponent : public plAudioSystemProxyDependentComponent
{
  PL_DECLARE_COMPONENT_TYPE(plAudioRtpcComponent, plAudioSystemProxyDependentComponent, plAudioRtpcComponentManager);

  // plComponent

public:
  void Initialize() override;
  void SerializeComponent(plWorldWriter& stream) const override;
  void DeserializeComponent(plWorldReader& stream) override;

  // plAudioSystemComponent

private:
  void plAudioSystemComponentIsAbstract() override {}

  // plAudioRtpcComponent

public:
  plAudioRtpcComponent();
  ~plAudioRtpcComponent() override;

  /// \brief Sets the value of the parameter. This will send a request to the Audio System.
  ///
  /// \param fValue The new value the parameter should have.
  /// \param bSync Whether the request should be sent synchronously or asynchronously.
  void SetValue(float fValue, bool bSync = false);

  /// \brief Gets the current value of the parameter.
  /// \returns The current value of the parameter.
  PL_NODISCARD float GetValue() const;

  /// \brief Event that is triggered when the component receives a
  /// SetRtpcValue message.
  void OnSetValue(plMsgAudioSystemSetRtpcValue& msg);

private:
  plString m_sRtpcName;
  float m_fInitialValue;
  float m_fValue;

  plEventMessageSender<plMsgAudioSystemRtpcValueChanged> m_ValueChangedEventSender;
};
