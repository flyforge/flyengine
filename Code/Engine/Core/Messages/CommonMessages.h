#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

/// \brief Common message for components that can be toggled between playing and paused states
struct PL_CORE_DLL plMsgSetPlaying : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgSetPlaying, plMessage);

  bool m_bPlay = true;
};

/// \brief Basic message to set some generic parameter to a float value.
struct PL_CORE_DLL plMsgSetFloatParameter : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgSetFloatParameter, plMessage);

  plString m_sParameterName;
  float m_fValue = 0;
};

/// \brief For use in scripts to signal a custom event that some game event has occurred.
///
/// This is a simple message for simple use cases. Create custom messages for more elaborate cases where a string is not sufficient
/// information.
struct PL_CORE_DLL plMsgGenericEvent : public plEventMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgGenericEvent, plEventMessage);

  /// A custom string to identify the intent.
  plHashedString m_sMessage;
  plVariant m_Value;
};

/// \brief Sent when an animation reached its end (either forwards or backwards playing)
///
/// This is sent regardless of whether the animation is played once, looped or back and forth,
/// ie. it should be sent at each 'end' point, even when it then starts another cycle.
struct PL_CORE_DLL plMsgAnimationReachedEnd : public plEventMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgAnimationReachedEnd, plEventMessage);
};
