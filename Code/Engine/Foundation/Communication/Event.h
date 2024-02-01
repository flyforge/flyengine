#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/Delegate.h>

/// \brief Identifies an event subscription. Zero is always an invalid subscription ID.
using plEventSubscriptionID = plUInt32;

/// \brief Specifies the type of plEvent implementation to use
enum class plEventType
{
  Default,        /// Default implementation. Does not support modifying the event while broadcasting.
  CopyOnBroadcast /// CopyOnBroadcast implementation. Supports modifying the event while broadcasting.
};

/// \brief This class propagates event information to registered event handlers.
///
/// An event can be anything that "happens" that might be of interest to other code, such
/// that it can react on it in some way.
/// Just create an instance of plEvent and call Broadcast() on it. Other interested code needs const access to
/// the event variable to be able to call AddEventHandler() and RemoveEventHandler().
/// To pass information to the handlers, create a custom struct with event information
/// and then pass a (const) reference to that data through Broadcast().
///
/// If you need to modify the event while broadcasting, for example inside one of the registered event handlers,
/// set EventType = plEventType::CopyOnBroadcast. Each broadcast will then copy the event handler array before signaling them, allowing
/// modifications during broadcasting.
///
/// \note A class holding an plEvent member needs to provide public access to the member for external code to
/// be able to register as an event handler. To make it possible to prevent external code from also raising events,
/// all functions that are needed for listening are const, and all others are non-const.
/// Therefore, simply make event members private and provide const reference access through a public getter.
template <typename EventData, typename MutexType, plEventType EventType>
class plEventBase
{
protected:
  /// \brief Constructor.
  plEventBase(plAllocator* pAllocator);
  ~plEventBase();

public:
  /// \brief Notification callback type for events.
  using Handler = plDelegate<void(EventData)>;

  /// \brief An object that can be passed to plEvent::AddEventHandler to store the subscription information
  /// and automatically remove the event handler upon destruction.
  class Unsubscriber
  {
    PL_DISALLOW_COPY_AND_ASSIGN(Unsubscriber);

  public:
    Unsubscriber() = default;
    Unsubscriber(Unsubscriber&& other)
    {
      m_pEvent = other.m_pEvent;
      m_SubscriptionID = other.m_SubscriptionID;
      other.Clear();
    }
    ~Unsubscriber() { Unsubscribe(); }

    void operator=(Unsubscriber&& other)
    {
      Unsubscribe();

      m_pEvent = other.m_pEvent;
      m_SubscriptionID = other.m_SubscriptionID;
      other.Clear();
    }

    /// \brief If the unsubscriber holds a valid subscription, it will be removed from the target plEvent.
    void Unsubscribe()
    {
      if (m_SubscriptionID == 0)
        return;

      m_pEvent->RemoveEventHandler(m_SubscriptionID);
      Clear();
    }

    /// \brief Checks whether this unsubscriber has a valid subscription.
    bool IsSubscribed() const { return m_SubscriptionID != 0; }

    /// \brief Resets the unsubscriber. Use when the target plEvent may have been destroyed and automatic unsubscription cannot be executed
    /// anymore.
    void Clear()
    {
      m_pEvent = nullptr;
      m_SubscriptionID = 0;
    }

  private:
    friend class plEventBase<EventData, MutexType, EventType>;

    const plEventBase<EventData, MutexType, EventType>* m_pEvent = nullptr;
    plEventSubscriptionID m_SubscriptionID = 0;
  };

  /// \brief Implementation specific constants.
  enum
  {
    /// Whether the uiMaxRecursionDepth parameter to Broadcast() is supported in this implementation or not.
    RecursionDepthSupported = (EventType == plEventType::Default || plConversionTest<MutexType, plNoMutex>::sameType == 1) ? 1 : 0,

    /// Default value for the maximum recursion depth of Broadcast.
    /// As limiting the recursion depth is not supported when EventType == plEventType::CopyAndBroadcast and MutexType != plNoMutex
    /// the default value for that case is the maximum.
    MaxRecursionDepthDefault = RecursionDepthSupported ? 0 : 255
  };

  /// \brief This function will broadcast to all registered users, that this event has just happened.
  ///  Setting uiMaxRecursionDepth will allow you to permit recursions. When broadcasting consider up to what depth
  ///  you want recursions to be permitted. By default no recursion is allowed.
  void Broadcast(EventData pEventData, plUInt8 uiMaxRecursionDepth = MaxRecursionDepthDefault); // [tested]

  /// \brief Adds a function as an event handler. All handlers will be notified in the order that they were registered.
  ///
  /// The return value can be stored and used to remove the event handler later again.
  plEventSubscriptionID AddEventHandler(Handler handler) const; // [tested]

  /// \brief An overload that adds an event handler and initializes the given \a Unsubscriber object.
  ///
  /// When the Unsubscriber is destroyed, it will automatically remove the event handler.
  void AddEventHandler(Handler handler, Unsubscriber& inout_unsubscriber) const; // [tested]

  /// \brief Removes a previously registered handler. It is an error to remove a handler that was not registered.
  void RemoveEventHandler(const Handler& handler) const; // [tested]

  /// \brief Removes a previously registered handler via the returned subscription ID.
  ///
  /// The ID will be reset to zero.
  /// If this is called with a zero ID, nothing happens.
  void RemoveEventHandler(plEventSubscriptionID& inout_id) const;

  /// \brief Checks whether an event handler has already been registered.
  bool HasEventHandler(const Handler& handler) const;

  /// \brief Removes all registered event handlers.
  void Clear();

  /// \brief Returns true, if no event handlers are registered.
  bool IsEmpty() const;

  // it would be a problem if the plEvent moves in memory, for instance the Unsubscriber's would point to invalid memory
  PL_DISALLOW_COPY_AND_ASSIGN(plEventBase);

private:
  // Used to detect recursive broadcasts and then throw asserts at you.
  plUInt8 m_uiRecursionDepth = 0;
  mutable plEventSubscriptionID m_NextSubscriptionID = 0;

  mutable MutexType m_Mutex;

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  const void* m_pSelf = nullptr;
#endif

  struct HandlerData
  {
    Handler m_Handler;
    plEventSubscriptionID m_SubscriptionID;
  };

  /// \brief A dynamic array allows to have zero overhead as long as no event handlers are registered.
  mutable plDynamicArray<HandlerData> m_EventHandlers;
};

/// \brief Can be used when plEvent is used without any additional data
struct plNoEventData
{
};

/// \brief \see plEventBase
template <typename EventData, typename MutexType = plNoMutex, typename AllocatorWrapper = plDefaultAllocatorWrapper, plEventType EventType = plEventType::Default>
class plEvent : public plEventBase<EventData, MutexType, EventType>
{
public:
  plEvent();
  plEvent(plAllocator* pAllocator);
};

template <typename EventData, typename MutexType = plNoMutex, typename AllocatorWrapper = plDefaultAllocatorWrapper>
using plCopyOnBroadcastEvent = plEvent<EventData, MutexType, AllocatorWrapper, plEventType::CopyOnBroadcast>;

#include <Foundation/Communication/Implementation/Event_inl.h>
