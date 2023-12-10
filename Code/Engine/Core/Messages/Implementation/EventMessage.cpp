#include <Core/CorePCH.h>

#include <Core/Messages/EventMessage.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

// clang-format off
PLASMA_IMPLEMENT_MESSAGE_TYPE(plEventMessage);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEventMessage, 1, plRTTIDefaultAllocator<plEventMessage>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PLASMA_CHECK_AT_COMPILETIME(sizeof(plEventMessageSender<plEventMessage>) == 16);

namespace plInternal
{
  template <typename World, typename GameObject>
  static void UpdateCachedReceivers(const plMessage& msg, World& world, GameObject pSearchObject, plSmallArray<plComponentHandle, 1>& inout_CachedReceivers)
  {
    if (inout_CachedReceivers.GetUserData<plUInt32>() == 0)
    {
      using ComponentType = typename std::conditional<std::is_const<World>::value, const plComponent*, plComponent*>::type;

      plHybridArray<ComponentType, 4> eventMsgHandlers;
      world.FindEventMsgHandlers(msg, pSearchObject, eventMsgHandlers);

      for (auto pEventMsgHandler : eventMsgHandlers)
      {
        inout_CachedReceivers.PushBack(pEventMsgHandler->GetHandle());
      }

      inout_CachedReceivers.GetUserData<plUInt32>() = 1;
    }
  }

  bool EventMessageSenderHelper::SendEventMessage(plMessage& msg, plComponent* pSenderComponent, plGameObject* pSearchObject, plSmallArray<plComponentHandle, 1>& inout_CachedReceivers)
  {
    plWorld* pWorld = pSenderComponent->GetWorld();
    UpdateCachedReceivers(msg, *pWorld, pSearchObject, inout_CachedReceivers);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    bool bResult = false;

    for (auto hReceiver : inout_CachedReceivers)
    {
      plComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        bResult |= pReceiverComponent->SendMessage(msg);
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && msg.GetDebugMessageRouting())
    {
      plLog::Warning("plEventMessageSender::SendMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
    return bResult;
  }

  bool EventMessageSenderHelper::SendEventMessage(plMessage& msg, const plComponent* pSenderComponent, const plGameObject* pSearchObject, plSmallArray<plComponentHandle, 1>& inout_CachedReceivers)
  {
    const plWorld* pWorld = pSenderComponent->GetWorld();
    UpdateCachedReceivers(msg, *pWorld, pSearchObject, inout_CachedReceivers);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    bool bResult = false;

    for (auto hReceiver : inout_CachedReceivers)
    {
      const plComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        bResult |= pReceiverComponent->SendMessage(msg);
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && msg.GetDebugMessageRouting())
    {
      plLog::Warning("plEventMessageSender::SendMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif

    return bResult;
  }

  void EventMessageSenderHelper::PostEventMessage(const plMessage& msg, const plComponent* pSenderComponent, const plGameObject* pSearchObject, plSmallArray<plComponentHandle, 1>& inout_CachedReceivers, plTime delay, plObjectMsgQueueType::Enum queueType)
  {
    const plWorld* pWorld = pSenderComponent->GetWorld();
    UpdateCachedReceivers(msg, *pWorld, pSearchObject, inout_CachedReceivers);

    if (!inout_CachedReceivers.IsEmpty())
    {
      for (auto hReceiver : inout_CachedReceivers)
      {
        pWorld->PostMessage(hReceiver, msg, delay, queueType);
      }
    }
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    else if (msg.GetDebugMessageRouting())
    {
      plLog::Warning("plEventMessageSender::PostMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }

} // namespace plInternal

PLASMA_STATICLINK_FILE(Core, Core_Messages_Implementation_EventMessage);
