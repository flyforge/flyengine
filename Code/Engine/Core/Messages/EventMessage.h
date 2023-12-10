#pragma once

#include <Core/World/World.h>
#include <Foundation/Communication/Message.h>

/// \brief Base class for all messages that are sent as 'events'
struct PLASMA_CORE_DLL plEventMessage : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plEventMessage, plMessage);

  plGameObjectHandle m_hSenderObject;
  plComponentHandle m_hSenderComponent;

  PLASMA_ALWAYS_INLINE void FillFromSenderComponent(const plComponent* pSenderComponent)
  {
    if (pSenderComponent != nullptr)
    {
      m_hSenderComponent = pSenderComponent->GetHandle();
      m_hSenderObject = pSenderComponent->GetOwner()->GetHandle();
    }
  }
};

namespace plInternal
{
  struct PLASMA_CORE_DLL EventMessageSenderHelper
  {
    static bool SendEventMessage(plMessage& msg, plComponent* pSenderComponent, plGameObject* pSearchObject, plSmallArray<plComponentHandle, 1>& inout_CachedReceivers);
    static bool SendEventMessage(plMessage& msg, const plComponent* pSenderComponent, const plGameObject* pSearchObject, plSmallArray<plComponentHandle, 1>& inout_CachedReceivers);
    static void PostEventMessage(const plMessage& msg, const plComponent* pSenderComponent, const plGameObject* pSearchObject, plSmallArray<plComponentHandle, 1>& inout_CachedReceivers, plTime delay, plObjectMsgQueueType::Enum queueType);
  };
} // namespace plInternal

/// \brief A message sender that sends all messages to the next component derived from plEventMessageHandlerComponent
///   up in the hierarchy starting with the given search object. If none is found the message is sent to
///   all components registered as global event message handler. The receiver is cached after the first send/post call.
template <typename EventMessageType>
class plEventMessageSender : public plMessageSenderBase<EventMessageType>
{
public:
  PLASMA_ALWAYS_INLINE bool SendEventMessage(EventMessageType& msg, plComponent* pSenderComponent, plGameObject* pSearchObject)
  {
    if constexpr (PLASMA_IS_DERIVED_FROM_STATIC(plEventMessage, EventMessageType))
    {
      msg.FillFromSenderComponent(pSenderComponent);
    }
    return plInternal::EventMessageSenderHelper::SendEventMessage(msg, pSenderComponent, pSearchObject, m_CachedReceivers);
  }

  PLASMA_ALWAYS_INLINE bool SendEventMessage(EventMessageType& msg, const plComponent* pSenderComponent, const plGameObject* pSearchObject) const
  {
    if constexpr (PLASMA_IS_DERIVED_FROM_STATIC(plEventMessage, EventMessageType))
    {
      msg.FillFromSenderComponent(pSenderComponent);
    }
    return plInternal::EventMessageSenderHelper::SendEventMessage(msg, pSenderComponent, pSearchObject, m_CachedReceivers);
  }

  PLASMA_ALWAYS_INLINE void PostEventMessage(EventMessageType& msg, plComponent* pSenderComponent, plGameObject* pSearchObject,
    plTime delay, plObjectMsgQueueType::Enum queueType = plObjectMsgQueueType::NextFrame)
  {
    if constexpr (PLASMA_IS_DERIVED_FROM_STATIC(plEventMessage, EventMessageType))
    {
      msg.FillFromSenderComponent(pSenderComponent);
    }
    plInternal::EventMessageSenderHelper::PostEventMessage(msg, pSenderComponent, pSearchObject, m_CachedReceivers, delay, queueType);
  }

  PLASMA_ALWAYS_INLINE void PostEventMessage(EventMessageType& msg, const plComponent* pSenderComponent, const plGameObject* pSearchObject,
    plTime delay, plObjectMsgQueueType::Enum queueType = plObjectMsgQueueType::NextFrame) const
  {
    if constexpr (PLASMA_IS_DERIVED_FROM_STATIC(plEventMessage, EventMessageType))
    {
      msg.FillFromSenderComponent(pSenderComponent);
    }
    plInternal::EventMessageSenderHelper::PostEventMessage(msg, pSenderComponent, pSearchObject, m_CachedReceivers, delay, queueType);
  }

  PLASMA_ALWAYS_INLINE void Invalidate()
  {
    m_CachedReceivers.Clear();
    m_CachedReceivers.GetUserData<plUInt32>() = 0;
  }

private:
  mutable plSmallArray<plComponentHandle, 1> m_CachedReceivers;
};
