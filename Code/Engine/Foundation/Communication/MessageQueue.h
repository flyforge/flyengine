
#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

/// \brief Implementation of a message queue on top of a deque.
///
/// Enqueue and TryDequeue/TryPeek methods are thread safe all the others are not. To ensure
/// thread safety for all methods the queue can be locked using plLock like a mutex.
/// Every entry consists of a pointer to a message and some meta data.
/// Lifetime of the enqueued messages needs to be managed by the user.
/// \see plMessage
template <typename MetaDataType>
class plMessageQueueBase
{
public:
  struct Entry
  {
    PL_DECLARE_POD_TYPE();

    plMessage* m_pMessage;
    MetaDataType m_MetaData;
    mutable plUInt64 m_uiMessageHash = 0;
  };

protected:
  /// \brief No memory is allocated during construction.
  plMessageQueueBase(plAllocator* pAllocator); // [tested]

  /// \brief No memory is allocated during construction.
  plMessageQueueBase(const plMessageQueueBase& rhs, plAllocator* pAllocator);

  /// \brief Destructor.
  ~plMessageQueueBase(); // [tested]

  /// \brief Assignment operator.
  void operator=(const plMessageQueueBase& rhs);

public:
  /// \brief Returns the element at the given index. Not thread safe.
  Entry& operator[](plUInt32 uiIndex); // [tested]

  /// \brief Returns the element at the given index. Not thread safe.
  const Entry& operator[](plUInt32 uiIndex) const; // [tested]

  /// \brief Returns the number of active elements in the queue.
  plUInt32 GetCount() const;

  /// \brief Returns true, if the queue does not contain any elements.
  bool IsEmpty() const;

  /// \brief Destructs all elements and sets the count to zero. Does not deallocate any data.
  void Clear();

  /// \brief Expands the queue so it can at least store the given capacity.
  void Reserve(plUInt32 uiCount);

  /// \brief Tries to compact the array to avoid wasting memory.The resulting capacity is at least 'GetCount' (no elements get removed).
  void Compact();

  /// \brief Enqueues the given message and meta-data. This method is thread safe.
  void Enqueue(plMessage* pMessage, const MetaDataType& metaData); // [tested]

  /// \brief Dequeues the first element if the queue is not empty and returns true. Returns false if the queue is empty. This method is thread safe.
  bool TryDequeue(plMessage*& out_pMessage, MetaDataType& out_metaData); // [tested]

  /// \brief Gives the first element if the queue is not empty and returns true. Returns false if the queue is empty. This method is thread safe.
  bool TryPeek(plMessage*& out_pMessage, MetaDataType& out_metaData); // [tested]

  /// \brief Returns the first element in the queue. Not thread safe.
  Entry& Peek();

  /// \brief Removes the first element from the queue. Not thread safe.
  void Dequeue();

  /// \brief Sort with explicit comparer. Not thread safe.
  template <typename Comparer>
  void Sort(const Comparer& comparer); // [tested]

  /// \brief Acquires an exclusive lock on the queue. Do not use this method directly but use plLock instead.
  void Lock(); // [tested]

  /// \brief Releases a lock that has been previously acquired. Do not use this method directly but use plLock instead.
  void Unlock(); // [tested]

private:
  plDeque<Entry, plNullAllocatorWrapper> m_Queue;
  plMutex m_Mutex;
};

/// \brief \see plMessageQueueBase
template <typename MetaDataType, typename AllocatorWrapper = plDefaultAllocatorWrapper>
class plMessageQueue : public plMessageQueueBase<MetaDataType>
{
public:
  plMessageQueue();
  plMessageQueue(plAllocator* pAllocator);

  plMessageQueue(const plMessageQueue<MetaDataType, AllocatorWrapper>& rhs);
  plMessageQueue(const plMessageQueueBase<MetaDataType>& rhs);

  void operator=(const plMessageQueue<MetaDataType, AllocatorWrapper>& rhs);
  void operator=(const plMessageQueueBase<MetaDataType>& rhs);
};

#include <Foundation/Communication/Implementation/MessageQueue_inl.h>
