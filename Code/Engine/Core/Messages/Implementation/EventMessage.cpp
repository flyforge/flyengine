#include <Core/CorePCH.h>

#include <Core/Messages/EventMessage.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

// clang-format off
PL_IMPLEMENT_MESSAGE_TYPE(plEventMessage);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEventMessage, 1, plRTTIDefaultAllocator<plEventMessage>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PL_CHECK_AT_COMPILETIME(sizeof(plEventMessageSender<plEventMessage>) == 16);

namespace plInternal
{
  template <typename World, typename GameObject>
  static void UpdateCachedReceivers(const plMessage& msg, World& ref_world, GameObject pSearchObject, plSmallArray<plComponentHandle, 1>& inout_cachedReceivers)
  {
    if (inout_cachedReceivers.GetUserData<plUInt32>() == 0)
    {
      using ComponentType = typename std::conditional<std::is_const<World>::value, const plComponent*, plComponent*>::type;

      plHybridArray<ComponentType, 4> eventMsgHandlers;
      ref_world.FindEventMsgHandlers(msg, pSearchObject, eventMsgHandlers);

      for (auto pEventMsgHandler : eventMsgHandlers)
      {
        inout_cachedReceivers.PushBack(pEventMsgHandler->GetHandle());
      }

      inout_cachedReceivers.GetUserData<plUInt32>() = 1;
    }
  }

  bool EventMessageSenderHelper::SendEventMessage(plMessage& ref_msg, plComponent* pSenderComponent, plGameObject* pSearchObject, plSmallArray<plComponentHandle, 1>& inout_cachedReceivers)
  {
    plWorld* pWorld = pSenderComponent->GetWorld();
    UpdateCachedReceivers(ref_msg, *pWorld, pSearchObject, inout_cachedReceivers);

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    bool bResult = false;
    for (auto hReceiver : inout_cachedReceivers)
    {
      plComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        bResult |= pReceiverComponent->SendMessage(ref_msg);
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && ref_msg.GetDebugMessageRouting())
    {
      plLog::Warning("plEventMessageSender::SendMessage: No event message handler found for message of type {0}.", ref_msg.GetId());
    }
#endif

    return bResult;
  }

  bool EventMessageSenderHelper::SendEventMessage(plMessage& ref_msg, const plComponent* pSenderComponent, const plGameObject* pSearchObject, plSmallArray<plComponentHandle, 1>& inout_cachedReceivers)
  {
    const plWorld* pWorld = pSenderComponent->GetWorld();
    UpdateCachedReceivers(ref_msg, *pWorld, pSearchObject, inout_cachedReceivers);

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    bool bResult = false;
    for (auto hReceiver : inout_cachedReceivers)
    {
      const plComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        bResult |= pReceiverComponent->SendMessage(ref_msg);
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && ref_msg.GetDebugMessageRouting())
    {
      plLog::Warning("plEventMessageSender::SendMessage: No event message handler found for message of type {0}.", ref_msg.GetId());
    }
#endif

    return bResult;
  }

  void EventMessageSenderHelper::PostEventMessage(const plMessage& msg, const plComponent* pSenderComponent, const plGameObject* pSearchObject, plSmallArray<plComponentHandle, 1>& inout_cachedReceivers, plTime delay, plObjectMsgQueueType::Enum queueType)
  {
    const plWorld* pWorld = pSenderComponent->GetWorld();
    UpdateCachedReceivers(msg, *pWorld, pSearchObject, inout_cachedReceivers);

    if (!inout_cachedReceivers.IsEmpty())
    {
      for (auto hReceiver : inout_cachedReceivers)
      {
        pWorld->PostMessage(hReceiver, msg, delay, queueType);
      }
    }
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
    else if (msg.GetDebugMessageRouting())
    {
      plLog::Warning("plEventMessageSender::PostMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }

} // namespace plInternal

PL_STATICLINK_FILE(Core, Core_Messages_Implementation_EventMessage);
