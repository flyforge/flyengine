#pragma once

#include <Foundation/Types/ScopeExit.h>

template <typename EventData, typename MutexType, plEventType EventType>
plEventBase<EventData, MutexType, EventType>::plEventBase(plAllocatorBase* pAllocator)
  : m_EventHandlers(pAllocator)
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  m_pSelf = this;
#endif
}

template <typename EventData, typename MutexType, plEventType EventType>
plEventBase<EventData, MutexType, EventType>::~plEventBase()
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  PLASMA_ASSERT_ALWAYS(m_pSelf == this, "The plEvent was relocated in memory. This is not allowed, as it breaks the Unsubscribers.");
#endif
}

/// A callback can be registered multiple times with different pass-through data (or even with the same,
/// though that is less useful).
template <typename EventData, typename MutexType, plEventType EventType>
plEventSubscriptionID plEventBase<EventData, MutexType, EventType>::AddEventHandler(Handler handler) const
{
  PLASMA_LOCK(m_Mutex);

  if constexpr (std::is_same_v<MutexType, plNoMutex>)
  {
    if constexpr (EventType == plEventType::Default)
    {
      PLASMA_ASSERT_DEV(m_uiRecursionDepth == 0, "Can't add or remove event handlers while broadcasting (without a mutex). Either enable the use of a mutex on this event, or switch to plCopyOnBroadcastEvent if this should be allowed. Since this event does not have a mutex, this error can also happen due to multi-threaded access.");
    }
  }

  PLASMA_ASSERT_DEV(!handler.IsComparable() || !HasEventHandler(handler), "The same event handler cannot be added twice");

  auto& item = m_EventHandlers.ExpandAndGetRef();
  item.m_Handler = std::move(handler);
  item.m_SubscriptionID = ++m_NextSubscriptionID;

  return item.m_SubscriptionID;
}

template <typename EventData, typename MutexType, plEventType EventType>
void plEventBase<EventData, MutexType, EventType>::AddEventHandler(Handler handler, Unsubscriber& inout_unsubscriber) const
{
  PLASMA_LOCK(m_Mutex);

  if constexpr (std::is_same_v<MutexType, plNoMutex>)
  {
    if constexpr (EventType == plEventType::Default)
    {
      PLASMA_ASSERT_DEV(m_uiRecursionDepth == 0, "Can't add or remove event handlers while broadcasting (without a mutex). Either enable the use of a mutex on this event, or switch to plCopyOnBroadcastEvent if this should be allowed. Since this event does not have a mutex, this error can also happen due to multi-threaded access.");
    }
  }

  inout_unsubscriber.Unsubscribe();
  inout_unsubscriber.m_pEvent = this;
  inout_unsubscriber.m_SubscriptionID = AddEventHandler(std::move(handler));
}


/// Use exactly the same combination of callback/pass-through-data to unregister an event handlers.
/// Otherwise an error occurs.
template <typename EventData, typename MutexType, plEventType EventType>
void plEventBase<EventData, MutexType, EventType>::RemoveEventHandler(const Handler& handler) const
{
  PLASMA_ASSERT_DEV(handler.IsComparable(), "Lambdas that capture data cannot be removed via function pointer. Use an plEventSubscriptionID instead.");

  PLASMA_LOCK(m_Mutex);

  if constexpr (EventType == plEventType::Default)
  {
    if constexpr (std::is_same_v<MutexType, plNoMutex>)
    {
      PLASMA_ASSERT_DEV(m_uiRecursionDepth == 0, "Can't add or remove event handlers while broadcasting (without a mutex). Either enable the use of a mutex on this event, or switch to plCopyOnBroadcastEvent if this should be allowed. Since this event does not have a mutex, this error can also happen due to multi-threaded access.");
    }
  }

  for (plUInt32 idx = 0; idx < m_EventHandlers.GetCount(); ++idx)
  {
    if (m_EventHandlers[idx].m_Handler.IsEqualIfComparable(handler))
    {
      if constexpr (EventType == plEventType::Default)
      {
        // if this event does not copy the handlers, and we are currently broadcasting
        // we can't shrink the size of the array, however, we can replace elements (the check above says that we have a mutex, so this is fine)

        if (m_uiRecursionDepth > 0)
        {
          // we just write an invalid handler here, and let the broadcast function clean it up for us
          m_EventHandlers[idx].m_Handler = {};
          m_EventHandlers[idx].m_SubscriptionID = {};
          return;
        }
      }

      // if we are not broadcasting, or the broadcast uses a copy anyway, we can just modify the handler array directly
      m_EventHandlers.RemoveAtAndCopy(idx);
      return;
    }
  }

  PLASMA_ASSERT_DEV(false, "plEvent::RemoveEventHandler: Handler has not been registered or already been unregistered.");
}

template <typename EventData, typename MutexType, plEventType EventType>
void plEventBase<EventData, MutexType, EventType>::RemoveEventHandler(plEventSubscriptionID& inout_id) const
{
  if (inout_id == 0)
    return;

  const plEventSubscriptionID subId = inout_id;
  inout_id = 0;

  PLASMA_LOCK(m_Mutex);

  if constexpr (EventType == plEventType::Default)
  {
    if constexpr (std::is_same_v<MutexType, plNoMutex>)
    {
      PLASMA_ASSERT_DEV(m_uiRecursionDepth == 0, "Can't add or remove event handlers while broadcasting (without a mutex). Either enable the use of a mutex on this event, or switch to plCopyOnBroadcastEvent if this should be allowed. Since this event does not have a mutex, this error can also happen due to multi-threaded access.");
    }
  }

  for (plUInt32 idx = 0; idx < m_EventHandlers.GetCount(); ++idx)
  {
    if (m_EventHandlers[idx].m_SubscriptionID == subId)
    {
      if constexpr (EventType == plEventType::Default)
      {
        // if this event does not copy the handlers, and we are currently broadcasting
        // we can't shrink the size of the array, however, we can replace elements (the check above says that we have a mutex, so this is fine)

        if (m_uiRecursionDepth > 0)
        {
          // we just write an invalid handler here, and let the broadcast function clean it up for us
          m_EventHandlers[idx].m_Handler = {};
          m_EventHandlers[idx].m_SubscriptionID = {};
          return;
        }
      }

      // if we are not broadcasting, or the broadcast uses a copy anyway, we can just modify the handler array directly
      m_EventHandlers.RemoveAtAndCopy(idx);
      return;
    }
  }

  PLASMA_ASSERT_DEV(false, "plEvent::RemoveEventHandler: Invalid subscription ID '{0}'.", (plInt32)subId);
}

template <typename EventData, typename MutexType, plEventType EventType>
bool plEventBase<EventData, MutexType, EventType>::HasEventHandler(const Handler& handler) const
{
  PLASMA_ASSERT_DEV(handler.IsComparable(), "Lambdas that capture data cannot be checked via function pointer. Use an plEventSubscriptionID instead.");

  PLASMA_LOCK(m_Mutex);

  for (plUInt32 i = 0; i < m_EventHandlers.GetCount(); ++i)
  {
    if (m_EventHandlers[i].m_Handler.IsEqualIfComparable(handler))
      return true;
  }

  return false;
}

template <typename EventData, typename MutexType, plEventType EventType>
void plEventBase<EventData, MutexType, EventType>::Clear()
{
  PLASMA_LOCK(m_Mutex);
  m_EventHandlers.Clear();
}

template <typename EventData, typename MutexType, plEventType EventType>
bool plEventBase<EventData, MutexType, EventType>::IsEmpty() const
{
  PLASMA_LOCK(m_Mutex);
  return m_EventHandlers.IsEmpty();
}

/// The notification is sent to all event handlers in the order that they were registered.
template <typename EventData, typename MutexType, plEventType EventType>
void plEventBase<EventData, MutexType, EventType>::Broadcast(EventData eventData, plUInt8 uiMaxRecursionDepth)
{
  if constexpr (EventType == plEventType::Default)
  {
    PLASMA_LOCK(m_Mutex);

    PLASMA_ASSERT_DEV(m_uiRecursionDepth <= uiMaxRecursionDepth, "The event has been triggered recursively or from several threads simultaneously.");

    if (m_uiRecursionDepth > uiMaxRecursionDepth)
      return;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
    PLASMA_ASSERT_ALWAYS(m_pSelf == this, "The plEvent was relocated in memory. This is not allowed, as it breaks the Unsubscribers.");
#endif

    m_uiRecursionDepth++;

    // RAII to ensure correctness in case exceptions are used
    auto scopeExit = plMakeScopeExit([&]() {
      m_uiRecursionDepth--;
    });

    // don't execute handlers that are added while we are broadcasting
    plUInt32 uiMaxHandlers = m_EventHandlers.GetCount();

    for (plUInt32 ui = 0; ui < uiMaxHandlers;)
    {
      if (m_EventHandlers[ui].m_Handler.IsValid())
      {
        m_EventHandlers[ui].m_Handler(eventData);
        ++ui;
      }
      else
      {
        m_EventHandlers.RemoveAtAndCopy(ui);
        --uiMaxHandlers;
      }
    }
  }
  else
  {
    plHybridArray<HandlerData, 16> eventHandlers;
    {
      PLASMA_LOCK(m_Mutex);

      if constexpr (RecursionDepthSupported)
      {
        PLASMA_ASSERT_DEV(m_uiRecursionDepth <= uiMaxRecursionDepth, "The event has been triggered recursively or from several threads simultaneously.");

        if (m_uiRecursionDepth > uiMaxRecursionDepth)
          return;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
        PLASMA_ASSERT_ALWAYS(m_pSelf == this, "The plEvent was relocated in memory. This is not allowed, as it breaks the Unsubscribers.");
#endif

        m_uiRecursionDepth++;
      }
      else
      {
        PLASMA_ASSERT_DEV(uiMaxRecursionDepth == 255, "uiMaxRecursionDepth is not supported if plEventType::CopyOnBroadcast is used and the event needs to be threadsafe.");
      }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
      PLASMA_ASSERT_ALWAYS(m_pSelf == this, "The plEvent was relocated in memory. This is not allowed, as it breaks the Unsubscribers.");
#endif

      eventHandlers = m_EventHandlers;
    }

    // RAII to ensure correctness in case exceptions are used
    auto scopeExit = plMakeScopeExit([&]() {
    // Bug in MSVC 2017. Can't use if constexpr.
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC) && _MSC_VER < 1920
      if (RecursionDepthSupported)
      {
        m_uiRecursionDepth--;
      }
#else
      if constexpr (RecursionDepthSupported)
      {
        m_uiRecursionDepth--;
      }
#endif
    });

    const plUInt32 uiHandlerCount = eventHandlers.GetCount();
    for (plUInt32 ui = 0; ui < uiHandlerCount; ++ui)
    {
      eventHandlers[ui].m_Handler(eventData);
    }
  }
}


template <typename EventData, typename MutexType, typename AllocatorWrapper, plEventType EventType>
plEvent<EventData, MutexType, AllocatorWrapper, EventType>::plEvent()
  : plEventBase<EventData, MutexType, EventType>(AllocatorWrapper::GetAllocator())
{
}

template <typename EventData, typename MutexType, typename AllocatorWrapper, plEventType EventType>
plEvent<EventData, MutexType, AllocatorWrapper, EventType>::plEvent(plAllocatorBase* pAllocator)
  : plEventBase<EventData, MutexType, EventType>(pAllocator)
{
}
