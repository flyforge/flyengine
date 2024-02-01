
template <typename MetaDataType>
plMessageQueueBase<MetaDataType>::plMessageQueueBase(plAllocator* pAllocator)
  : m_Queue(pAllocator)
{
}

template <typename MetaDataType>
plMessageQueueBase<MetaDataType>::plMessageQueueBase(const plMessageQueueBase& rhs, plAllocator* pAllocator)
  : m_Queue(pAllocator)
{
  m_Queue = rhs.m_Queue;
}

template <typename MetaDataType>
plMessageQueueBase<MetaDataType>::~plMessageQueueBase()
{
  Clear();
}

template <typename MetaDataType>
void plMessageQueueBase<MetaDataType>::operator=(const plMessageQueueBase& rhs)
{
  m_Queue = rhs.m_Queue;
}

template <typename MetaDataType>
PL_ALWAYS_INLINE typename plMessageQueueBase<MetaDataType>::Entry& plMessageQueueBase<MetaDataType>::operator[](plUInt32 uiIndex)
{
  return m_Queue[uiIndex];
}

template <typename MetaDataType>
PL_ALWAYS_INLINE const typename plMessageQueueBase<MetaDataType>::Entry& plMessageQueueBase<MetaDataType>::operator[](plUInt32 uiIndex) const
{
  return m_Queue[uiIndex];
}

template <typename MetaDataType>
PL_ALWAYS_INLINE plUInt32 plMessageQueueBase<MetaDataType>::GetCount() const
{
  return m_Queue.GetCount();
}

template <typename MetaDataType>
PL_ALWAYS_INLINE bool plMessageQueueBase<MetaDataType>::IsEmpty() const
{
  return m_Queue.IsEmpty();
}

template <typename MetaDataType>
void plMessageQueueBase<MetaDataType>::Clear()
{
  m_Queue.Clear();
}

template <typename MetaDataType>
PL_ALWAYS_INLINE void plMessageQueueBase<MetaDataType>::Reserve(plUInt32 uiCount)
{
  m_Queue.Reserve(uiCount);
}

template <typename MetaDataType>
PL_ALWAYS_INLINE void plMessageQueueBase<MetaDataType>::Compact()
{
  m_Queue.Compact();
}

template <typename MetaDataType>
void plMessageQueueBase<MetaDataType>::Enqueue(plMessage* pMessage, const MetaDataType& metaData)
{
  Entry entry;
  entry.m_pMessage = pMessage;
  entry.m_MetaData = metaData;

  {
    PL_LOCK(m_Mutex);

    m_Queue.PushBack(entry);
  }
}

template <typename MetaDataType>
bool plMessageQueueBase<MetaDataType>::TryDequeue(plMessage*& out_pMessage, MetaDataType& out_metaData)
{
  PL_LOCK(m_Mutex);

  if (!m_Queue.IsEmpty())
  {
    Entry& entry = m_Queue.PeekFront();
    out_pMessage = entry.m_pMessage;
    out_metaData = entry.m_MetaData;

    m_Queue.PopFront();
    return true;
  }

  return false;
}

template <typename MetaDataType>
bool plMessageQueueBase<MetaDataType>::TryPeek(plMessage*& out_pMessage, MetaDataType& out_metaData)
{
  PL_LOCK(m_Mutex);

  if (!m_Queue.IsEmpty())
  {
    Entry& entry = m_Queue.PeekFront();
    out_pMessage = entry.m_pMessage;
    out_metaData = entry.m_MetaData;

    return true;
  }

  return false;
}

template <typename MetaDataType>
PL_ALWAYS_INLINE typename plMessageQueueBase<MetaDataType>::Entry& plMessageQueueBase<MetaDataType>::Peek()
{
  return m_Queue.PeekFront();
}

template <typename MetaDataType>
PL_ALWAYS_INLINE void plMessageQueueBase<MetaDataType>::Dequeue()
{
  m_Queue.PopFront();
}

template <typename MetaDataType>
template <typename Comparer>
PL_ALWAYS_INLINE void plMessageQueueBase<MetaDataType>::Sort(const Comparer& comparer)
{
  m_Queue.Sort(comparer);
}

template <typename MetaDataType>
void plMessageQueueBase<MetaDataType>::Lock()
{
  m_Mutex.Lock();
}

template <typename MetaDataType>
void plMessageQueueBase<MetaDataType>::Unlock()
{
  m_Mutex.Unlock();
}


template <typename MD, typename A>
plMessageQueue<MD, A>::plMessageQueue()
  : plMessageQueueBase<MD>(A::GetAllocator())
{
}

template <typename MD, typename A>
plMessageQueue<MD, A>::plMessageQueue(plAllocator* pQueueAllocator)
  : plMessageQueueBase<MD>(pQueueAllocator)
{
}

template <typename MD, typename A>
plMessageQueue<MD, A>::plMessageQueue(const plMessageQueue<MD, A>& rhs)
  : plMessageQueueBase<MD>(rhs, A::GetAllocator())
{
}

template <typename MD, typename A>
plMessageQueue<MD, A>::plMessageQueue(const plMessageQueueBase<MD>& rhs)
  : plMessageQueueBase<MD>(rhs, A::GetAllocator())
{
}

template <typename MD, typename A>
void plMessageQueue<MD, A>::operator=(const plMessageQueue<MD, A>& rhs)
{
  plMessageQueueBase<MD>::operator=(rhs);
}

template <typename MD, typename A>
void plMessageQueue<MD, A>::operator=(const plMessageQueueBase<MD>& rhs)
{
  plMessageQueueBase<MD>::operator=(rhs);
}
