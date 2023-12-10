#pragma once

#include <Core/World/EventMessageHandlerComponent.h>
#include <GameEngine/GameEngineDLL.h>

struct plMsgTriggerTriggered;
struct plMsgComponentInternalTrigger;

using plTriggerDelayModifierComponentManager = plComponentManager<class plTriggerDelayModifierComponent, plBlockStorageType::Compact>;

/// \brief Handles plMsgTriggerTriggered events and sends new messages after a delay.
///
/// The 'enter' and 'leave' messages are sent only when an empty trigger is entered or when the last object leaves the trigger.
/// While any object is already inside the trigger, no change event is sent.
/// Therefore this component can't be used to keep track of all the objects inside the trigger.
///
/// The 'enter' and 'leave' events can be sent with a delay. The 'enter' event is only sent, if the trigger had at least one object
/// inside it for the full duration of the delay. Which exact object may change, but once the trigger contains no object, the timer is reset.
///
/// The sent plMsgTriggerTriggered does not contain a reference to the 'triggering' object, since there may be multiple and they may change randomly.
class PLASMA_GAMEENGINE_DLL plTriggerDelayModifierComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plTriggerDelayModifierComponent, plComponent, plTriggerDelayModifierComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plTriggerDelayModifierComponent

public:
  plTriggerDelayModifierComponent();
  ~plTriggerDelayModifierComponent();

protected:
  virtual void Initialize() override;

  void OnMsgTriggerTriggered(plMsgTriggerTriggered& msg);
  void OnMsgComponentInternalTrigger(plMsgComponentInternalTrigger& msg);

  bool m_bIsActivated = false;
  plInt32 m_iElementsInside = 0;
  plInt32 m_iValidActivationToken = 0;
  plInt32 m_iValidDeactivationToken = 0;

  plTime m_ActivationDelay;
  plTime m_DeactivationDelay;
  plHashedString m_sMessage;

  plEventMessageSender<plMsgTriggerTriggered> m_TriggerEventSender; // [ event ]
};
