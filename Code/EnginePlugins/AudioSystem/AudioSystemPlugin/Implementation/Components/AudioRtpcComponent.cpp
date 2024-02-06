#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioRtpcComponent.h>
#include <AudioSystemPlugin/Core/AudioSystem.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

constexpr plTypeVersion kVersion_AudioRtpcComponent = 1;

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plAudioRtpcComponent, kVersion_AudioRtpcComponent, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Rtpc", m_sRtpcName),
    PL_MEMBER_PROPERTY("InitialValue", m_fInitialValue),

    PL_ACCESSOR_PROPERTY_READ_ONLY("Value", GetValue)->AddAttributes(new plHiddenAttribute()),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(SetValue, In, "Value", In, "Sync"),
    PL_SCRIPT_FUNCTION_PROPERTY(GetValue),
  }
  PL_END_FUNCTIONS;

  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgAudioSystemSetRtpcValue, OnSetValue),
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

void plAudioRtpcComponent::Initialize()
{
  SUPER::Initialize();

  SetValue(m_fInitialValue, false);
}

void plAudioRtpcComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s.WriteVersion(kVersion_AudioRtpcComponent);

  s << m_sRtpcName;
  s << m_fInitialValue;
}

void plAudioRtpcComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  s.ReadVersion(kVersion_AudioRtpcComponent);

  s >> m_sRtpcName;
  s >> m_fInitialValue;
}

plAudioRtpcComponent::plAudioRtpcComponent()
  : plAudioSystemProxyDependentComponent()
  , m_fInitialValue(0.0f)
  , m_fValue(0.0f)
{
}

plAudioRtpcComponent::~plAudioRtpcComponent() = default;

void plAudioRtpcComponent::SetValue(float fValue, bool bSync)
{
  if (m_sRtpcName.IsEmpty())
    return;

  if (fValue == m_fValue)
    return; // No need to update...

  plAudioSystemRequestSetRtpcValue request;

  request.m_uiEntityId = GetEntityId();
  request.m_uiObjectId = plAudioSystem::GetSingleton()->GetRtpcId(m_sRtpcName);
  request.m_fValue = fValue;

  request.m_Callback = [this](const plAudioSystemRequestSetRtpcValue& e)
  {
    if (e.m_eStatus.Failed())
      return;

    // Save the value in the component
    m_fValue = e.m_fValue;

    // Notify for the change
    plMsgAudioSystemRtpcValueChanged msg;
    msg.m_fValue = e.m_fValue;

    // We are not in the writing thread, so posting the message for the next frame instead of sending...
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

float plAudioRtpcComponent::GetValue() const
{
  return m_fValue;
}

void plAudioRtpcComponent::OnSetValue(plMsgAudioSystemSetRtpcValue& msg)
{
  SetValue(msg.m_fValue, msg.m_bSync);
}

PL_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioRtpcComponent);
