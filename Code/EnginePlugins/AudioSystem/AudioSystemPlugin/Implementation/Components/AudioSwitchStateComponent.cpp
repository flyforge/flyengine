#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioSwitchStateComponent.h>
#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemRequests.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

constexpr plTypeVersion kVersion_AudioSwitchStateComponent = 1;

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plAudioSwitchStateComponent, kVersion_AudioSwitchStateComponent, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("InitialValue", m_sInitialSwitchState),

    PL_ACCESSOR_PROPERTY_READ_ONLY("State", GetState)->AddAttributes(new plHiddenAttribute()),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(SetState, In, "State", In, "Sync"),
    PL_SCRIPT_FUNCTION_PROPERTY(GetState),
  }
  PL_END_FUNCTIONS;

  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgAudioSystemSetSwitchState, OnSetState),
  }
  PL_END_MESSAGEHANDLERS;

  PL_BEGIN_MESSAGESENDERS
  {
    PL_MESSAGE_SENDER(m_ValueChangedEventSender),
  }
  PL_END_MESSAGESENDERS;
}
PL_END_COMPONENT_TYPE;
// clang-format on

void plAudioSwitchStateComponent::Initialize()
{
  SUPER::Initialize();

  SetState(m_sInitialSwitchState, false);
}

void plAudioSwitchStateComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s.WriteVersion(kVersion_AudioSwitchStateComponent);

  s << m_sInitialSwitchState;
}

void plAudioSwitchStateComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  s.ReadVersion(kVersion_AudioSwitchStateComponent);

  s >> m_sInitialSwitchState;
}

plAudioSwitchStateComponent::plAudioSwitchStateComponent()
  : plAudioSystemProxyDependentComponent()
{
}

plAudioSwitchStateComponent::~plAudioSwitchStateComponent() = default;

void plAudioSwitchStateComponent::SetState(const plString& sSwitchStateName, bool bSync)
{
  if (m_sCurrentSwitchState == sSwitchStateName)
    return; // No need to update...

  plAudioSystemRequestSetSwitchState request;

  request.m_uiEntityId = GetEntityId();
  request.m_uiObjectId = plAudioSystem::GetSingleton()->GetSwitchStateId(sSwitchStateName);

  request.m_Callback = [this, sSwitchStateName](const plAudioSystemRequestSetSwitchState& e)
  {
    if (e.m_eStatus.Failed())
      return;

    // Save the value in the component
    m_sCurrentSwitchState = sSwitchStateName;

    // Notify for the change
    plMsgAudioSystemSwitchStateChanged msg;
    msg.m_sSwitchState = sSwitchStateName;

    // We are not in the writing thread, so posting the message for the next frame instead of sending it now...
    m_ValueChangedEventSender.PostEventMessage(msg, this, GetOwner(), plTime::MakeZero(), plObjectMsgQueueType::NextFrame);
  };

  if (bSync)
  {
    plAudioSystem::GetSingleton()->SendRequestSync(request);
  }
  else
  {
    plAudioSystem::GetSingleton()->SendRequest(request);
  }
}

const plString& plAudioSwitchStateComponent::GetState() const
{
  return m_sCurrentSwitchState;
}

void plAudioSwitchStateComponent::OnSetState(plMsgAudioSystemSetSwitchState& msg)
{
  SetState(msg.m_sSwitchState, msg.m_bSync);
}

PL_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioSwitchStateComponent);
