
template <typename T, plUInt32 C>
plStaticArray<T, C>::plStaticArray()
{
  PLASMA_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;
}

template <typename T, plUInt32 C>
plStaticArray<T, C>::plStaticArray(const plStaticArray<T, C>& rhs)
{
  PLASMA_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;
  *this = (plArrayPtr<const T>)rhs; // redirect this to the plArrayPtr version
}

template <typename T, plUInt32 C>
template <plUInt32 OtherCapacity>
plStaticArray<T, C>::plStaticArray(const plStaticArray<T, OtherCapacity>& rhs)
{
  PLASMA_CHECK_AT_COMPILETIME(OtherCapacity <= C);

  PLASMA_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;

  *this = (plArrayPtr<const T>)rhs; // redirect this to the plArrayPtr version
}

template <typename T, plUInt32 C>
plStaticArray<T, C>::plStaticArray(const plArrayPtr<const T>& rhs)
{
  PLASMA_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
  this->m_uiCapacity = C;

  *this = rhs;
}

template <typename T, plUInt32 C>
plStaticArray<T, C>::~plStaticArray()
{
  this->Clear();
  PLASMA_ASSERT_DEBUG(this->m_pElements == nullptr, "static arrays should not use m_pElements");
}

template <typename T, plUInt32 C>
PLASMA_ALWAYS_INLINE T* plStaticArray<T, C>::GetStaticArray()
{
  return reinterpret_cast<T*>(m_Data);
}

template <typename T, plUInt32 C>
PLASMA_FORCE_INLINE const T* plStaticArray<T, C>::GetStaticArray() const
{
  return reinterpret_cast<const T*>(m_Data);
}

template <typename T, plUInt32 C>
PLASMA_FORCE_INLINE void plStaticArray<T, C>::Reserve(plUInt32 uiCapacity)
{
  PLASMA_ASSERT_DEV(uiCapacity <= C, "The static array has a fixed capacity of {0}, cannot reserve more elements than that.", C);
  // Nothing to do here
}

template <typename T, plUInt32 C>
PLASMA_ALWAYS_INLINE void plStaticArray<T, C>::operator=(const plStaticArray<T, C>& rhs)
{
  *this = (plArrayPtr<const T>)rhs; // redirect this to the plArrayPtr version
}

template <typename T, plUInt32 C>
template <plUInt32 OtherCapacity>
PLASMA_ALWAYS_INLINE void plStaticArray<T, C>::operator=(const plStaticArray<T, OtherCapacity>& rhs)
{
  *this = (plArrayPtr<const T>)rhs; // redirect this to the plArrayPtr version
}

template <typename T, plUInt32 C>
PLASMA_ALWAYS_INLINE void plStaticArray<T, C>::operator=(const plArrayPtr<const T>& rhs)
{
  plArrayBase<T, plStaticArray<T, C>>::operator=(rhs);
}

template <typename T, plUInt32 C>
PLASMA_FORCE_INLINE T* plStaticArray<T, C>::GetElementsPtr()
{
  return GetStaticArray();
}

template <typename T, plUInt32 C>
PLASMA_FORCE_INLINE const T* plStaticArray<T, C>::GetElementsPtr() const
{
  return GetStaticArray();
}
