
template <typename Type>
plProcessingStreamIterator<Type>::plProcessingStreamIterator(const plProcessingStream* pStream, plUInt64 uiNumElements, plUInt64 uiStartIndex)

{
  PLASMA_ASSERT_DEV(pStream != nullptr, "Stream pointer may not be null!");
  PLASMA_ASSERT_DEV(pStream->GetElementSize() == sizeof(Type), "Data size missmatch");

  m_uiElementStride = pStream->GetElementStride();

  m_pCurrentPtr = plMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<ptrdiff_t>(uiStartIndex * m_uiElementStride));
  m_pEndPtr = plMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<ptrdiff_t>((uiStartIndex + uiNumElements) * m_uiElementStride));
}

template <typename Type>
PLASMA_ALWAYS_INLINE Type& plProcessingStreamIterator<Type>::Current() const
{
  return *static_cast<Type*>(m_pCurrentPtr);
}

template <typename Type>
PLASMA_ALWAYS_INLINE bool plProcessingStreamIterator<Type>::HasReachedEnd() const
{
  return m_pCurrentPtr >= m_pEndPtr;
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plProcessingStreamIterator<Type>::Advance()
{
  m_pCurrentPtr = plMemoryUtils::AddByteOffset(m_pCurrentPtr, static_cast<ptrdiff_t>(m_uiElementStride));
}

template <typename Type>
PLASMA_ALWAYS_INLINE void plProcessingStreamIterator<Type>::Advance(plUInt32 uiNumElements)
{
  m_pCurrentPtr = plMemoryUtils::AddByteOffset(m_pCurrentPtr, static_cast<ptrdiff_t>(m_uiElementStride * uiNumElements));
}
