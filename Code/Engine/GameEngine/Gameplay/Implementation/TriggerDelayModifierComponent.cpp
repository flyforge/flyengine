#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/World/GameObject.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/TriggerDelayModifierComponent.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plTriggerDelayModifierComponent, 1 /* version */, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ActivationDelay", m_ActivationDelay),
    PL_MEMBER_PROPERTY("DeactivationDelay", m_DeactivationDelay),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgComponentInternalTrigger, OnMsgComponentInternalTrigger),
    PL_MESSAGE_HANDLER(plMsgTriggerTriggered, OnMsgTriggerTriggered),
  }
  PL_END_MESSAGEHANDLERS;

  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Logic"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plTriggerDelayModifierComponent::plTriggerDelayModifierComponent() = default;
plTriggerDelayModifierComponent::~plTriggerDelayModifierComponent() = default;

void plTriggerDelayModifierComponent::Initialize()
{
  SUPER::Initialize();
}

void plTriggerDelayModifierComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_ActivationDelay;
  s << m_DeactivationDelay;
}

void plTriggerDelayModifierComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_ActivationDelay;
  s >> m_DeactivationDelay;
}

void plTriggerDelayModifierComponent::OnMsgTriggerTriggered(plMsgTriggerTriggered& msg)
{
  if (msg.m_TriggerState == plTriggerState::Activated)
  {
    if (m_iElementsInside++ == 0) // was 0 before the increment
    {
      // the first object entered the trigger

      if (!m_bIsActivated)
      {
        // the trigger is not active yet -> send an activation message with a new activation token

        ++m_iValidActivationToken;

        // store the original trigger message for later
        m_sMessage = msg.m_sMessage;

        plMsgComponentInternalTrigger intMsg;
        intMsg.m_sMessage.Assign("Activate");
        intMsg.m_iPayload = m_iValidActivationToken;

        PostMessage(intMsg, m_ActivationDelay, plObjectMsgQueueType::PostTransform);
      }
      else
      {
        // the trigger is already active -> there are pending deactivations (otherwise we wouldn't have had an element count of zero)
        // -> invalidate those pending deactivations
        ++m_iValidDeactivationToken;

        // no need to send an activation message
      }
    }

    return;
  }

  if (msg.m_TriggerState == plTriggerState::Deactivated)
  {
    if (--m_iElementsInside == 0) // 0 after the decrement
    {
      // the last object left the trigger

      if (m_bIsActivated)
      {
        // if the trigger is active, we need to send a deactivation message and we give it a new token

        ++m_iValidDeactivationToken;

        // store the original trigger message for later
        m_sMessage = msg.m_sMessage;

        plMsgComponentInternalTrigger intMsg;
        intMsg.m_sMessage.Assign("Deactivate");
        intMsg.m_iPayload = m_iValidDeactivationToken;

        PostMessage(intMsg, m_DeactivationDelay, plObjectMsgQueueType::PostTransform);
      }
      else
      {
        // when we are already inactive, all that's needed is to invalidate any pending activations
        ++m_iValidActivationToken;
      }
    }

    return;
  }
}

void plTriggerDelayModifierComponent::OnMsgComponentInternalTrigger(plMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage == plTempHashedString("Activate"))
  {
    if (msg.m_iPayload == m_iValidActivationToken && !m_bIsActivated)
    {
      m_bIsActivated = true;

      plMsgTriggerTriggered newMsg;
      newMsg.m_sMessage = m_sMessage;
      newMsg.m_TriggerState = plTriggerState::Activated;

      m_TriggerEventSender.PostEventMessage(newMsg, this, GetOwner()->GetParent(), plTime::MakeZero(), plObjectMsgQueueType::PostTransform);
    }
  }
  else if (msg.m_sMessage == plTempHashedString("Deactivate"))
  {
    if (msg.m_iPayload == m_iValidDeactivationToken && m_bIsActivated)
    {
      m_bIsActivated = false;

      plMsgTriggerTriggered newMsg;
      newMsg.m_sMessage = m_sMessage;
      newMsg.m_TriggerState = plTriggerState::Deactivated;

      m_TriggerEventSender.PostEventMessage(newMsg, this, GetOwner()->GetParent(), plTime::MakeZero(), plObjectMsgQueueType::PostTransform);
    }
  }
}


PL_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_TriggerDelayModifierComponent);
