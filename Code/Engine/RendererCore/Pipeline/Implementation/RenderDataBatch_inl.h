
template <typename T>
PLASMA_ALWAYS_INLINE const T& plRenderDataBatch::Iterator<T>::operator*() const
{
  return *plStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
PLASMA_ALWAYS_INLINE const T* plRenderDataBatch::Iterator<T>::operator->() const
{
  return plStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
PLASMA_ALWAYS_INLINE plRenderDataBatch::Iterator<T>::operator const T*() const
{
  return plStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
PLASMA_FORCE_INLINE void plRenderDataBatch::Iterator<T>::Next()
{
  ++m_pCurrent;

  if (m_Filter.IsValid())
  {
    while (m_pCurrent < m_pEnd && m_Filter(m_pCurrent->m_pRenderData))
    {
      ++m_pCurrent;
    }
  }
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plRenderDataBatch::Iterator<T>::IsValid() const
{
  return m_pCurrent < m_pEnd;
}

template <typename T>
PLASMA_ALWAYS_INLINE void plRenderDataBatch::Iterator<T>::operator++()
{
  Next();
}

template <typename T>
PLASMA_FORCE_INLINE plRenderDataBatch::Iterator<T>::Iterator(const SortableRenderData* pStart, const SortableRenderData* pEnd, Filter filter)
  : m_Filter(filter)
{
  const SortableRenderData* pCurrent = pStart;
  if (m_Filter.IsValid())
  {
    while (pCurrent < pEnd && m_Filter(pCurrent->m_pRenderData))
    {
      ++pCurrent;
    }
  }

  m_pCurrent = pCurrent;
  m_pEnd = pEnd;
}


PLASMA_ALWAYS_INLINE plUInt32 plRenderDataBatch::GetCount() const
{
  return m_Data.GetCount();
}

template <typename T>
PLASMA_FORCE_INLINE const T* plRenderDataBatch::GetFirstData() const
{
  auto it = Iterator<T>(m_Data.GetPtr(), m_Data.GetPtr() + m_Data.GetCount(), m_Filter);
  return it.IsValid() ? (const T*)it : nullptr;
}

template <typename T>
PLASMA_FORCE_INLINE plRenderDataBatch::Iterator<T> plRenderDataBatch::GetIterator(plUInt32 uiStartIndex, plUInt32 uiCount) const
{
  plUInt32 uiEndIndex = plMath::Min(uiStartIndex + uiCount, m_Data.GetCount());
  return Iterator<T>(m_Data.GetPtr() + uiStartIndex, m_Data.GetPtr() + uiEndIndex, m_Filter);
}

//////////////////////////////////////////////////////////////////////////

PLASMA_ALWAYS_INLINE plUInt32 plRenderDataBatchList::GetBatchCount() const
{
  return m_Batches.GetCount();
}

PLASMA_FORCE_INLINE plRenderDataBatch plRenderDataBatchList::GetBatch(plUInt32 uiIndex) const
{
  plRenderDataBatch batch = m_Batches[uiIndex];
  batch.m_Filter = m_Filter;

  return batch;
}
