#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct PL_CORE_DLL plTriggerState
{
  using StorageType = plUInt8;

  enum Enum
  {
    Activated,   ///< The trigger was just activated (area entered, key pressed, etc.)
    Continuing,  ///< The trigger is active for more than one frame now.
    Deactivated, ///< The trigger was just deactivated (left area, key released, etc.)

    Default = Activated
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plTriggerState);

/// \brief For internal use by components to trigger some known behavior. Usually components will post this message to themselves with a
/// delay, e.g. to trigger self destruction.
struct PL_CORE_DLL plMsgComponentInternalTrigger : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgComponentInternalTrigger, plMessage);

  /// Identifies what the message should trigger.
  plHashedString m_sMessage;

  plInt32 m_iPayload = 0;
};

/// \brief Sent when something enters or leaves a trigger
struct PL_CORE_DLL plMsgTriggerTriggered : public plEventMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgTriggerTriggered, plEventMessage);

  /// Identifies what the message should trigger.
  plHashedString m_sMessage;

  /// Messages are only sent for 'entered' ('Activated') and 'left' ('Deactivated')
  plEnum<plTriggerState> m_TriggerState;

  /// The object that entered the trigger volume.
  plGameObjectHandle m_hTriggeringObject;
};
