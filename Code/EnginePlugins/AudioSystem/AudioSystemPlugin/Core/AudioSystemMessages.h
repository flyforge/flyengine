#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>

#include <Core/Messages/EventMessage.h>
#include <Foundation/Communication/Message.h>

struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plMsgAudioSystemSetRtpcValue : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgAudioSystemSetRtpcValue, plMessage);

  float m_fValue{0.0f};
  bool m_bSync{false};
};

struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plMsgAudioSystemRtpcValueChanged : public plEventMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgAudioSystemRtpcValueChanged, plEventMessage);

  float m_fValue{0.0f};
};

struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plMsgAudioSystemSetSwitchState : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgAudioSystemSetSwitchState, plMessage);

  plString m_sSwitchState{};
  bool m_bSync{false};
};

struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plMsgAudioSystemSwitchStateChanged : public plEventMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgAudioSystemSwitchStateChanged, plEventMessage);

  plString m_sSwitchState{};
};

struct PLASMA_AUDIOSYSTEMPLUGIN_DLL plMsgAudioSystemSetEnvironmentAmount : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgAudioSystemSetEnvironmentAmount, plMessage);

  float m_fAmount{0.0f};
  bool m_bSync{false};
};
