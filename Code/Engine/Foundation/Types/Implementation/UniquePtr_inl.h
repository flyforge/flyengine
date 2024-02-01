
template <typename T>
PL_ALWAYS_INLINE plUniquePtr<T>::plUniquePtr() = default;

template <typename T>
template <typename U>
PL_ALWAYS_INLINE plUniquePtr<T>::plUniquePtr(const plInternal::NewInstance<U>& instance)
{
  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;
}

template <typename T>
template <typename U>
PL_ALWAYS_INLINE plUniquePtr<T>::plUniquePtr(U* pInstance, plAllocator* pAllocator)
{
  m_pInstance = pInstance;
  m_pAllocator = pAllocator;
}

template <typename T>
template <typename U>
PL_ALWAYS_INLINE plUniquePtr<T>::plUniquePtr(plUniquePtr<U>&& other)
{
  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance = nullptr;
  other.m_pAllocator = nullptr;
}

template <typename T>
PL_ALWAYS_INLINE plUniquePtr<T>::plUniquePtr(std::nullptr_t)
{
}

template <typename T>
PL_ALWAYS_INLINE plUniquePtr<T>::~plUniquePtr()
{
  Clear();
}

template <typename T>
template <typename U>
PL_ALWAYS_INLINE plUniquePtr<T>& plUniquePtr<T>::operator=(const plInternal::NewInstance<U>& instance)
{
  Clear();

  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;

  return *this;
}

template <typename T>
template <typename U>
PL_ALWAYS_INLINE plUniquePtr<T>& plUniquePtr<T>::operator=(plUniquePtr<U>&& other)
{
  Clear();

  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance = nullptr;
  other.m_pAllocator = nullptr;

  return *this;
}

template <typename T>
PL_ALWAYS_INLINE plUniquePtr<T>& plUniquePtr<T>::operator=(std::nullptr_t)
{
  Clear();

  return *this;
}

template <typename T>
PL_ALWAYS_INLINE T* plUniquePtr<T>::Release()
{
  T* pInstance = m_pInstance;

  m_pInstance = nullptr;
  m_pAllocator = nullptr;

  return pInstance;
}

template <typename T>
PL_ALWAYS_INLINE T* plUniquePtr<T>::Release(plAllocator*& out_pAllocator)
{
  T* pInstance = m_pInstance;
  out_pAllocator = m_pAllocator;

  m_pInstance = nullptr;
  m_pAllocator = nullptr;

  return pInstance;
}

template <typename T>
PL_ALWAYS_INLINE T* plUniquePtr<T>::Borrow() const
{
  return m_pInstance;
}

template <typename T>
PL_ALWAYS_INLINE void plUniquePtr<T>::Clear()
{
  PL_DELETE(m_pAllocator, m_pInstance);

  m_pInstance = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
PL_ALWAYS_INLINE T& plUniquePtr<T>::operator*() const
{
  return *m_pInstance;
}

template <typename T>
PL_ALWAYS_INLINE T* plUniquePtr<T>::operator->() const
{
  return m_pInstance;
}

template <typename T>
PL_ALWAYS_INLINE plUniquePtr<T>::operator bool() const
{
  return m_pInstance != nullptr;
}

template <typename T>
PL_ALWAYS_INLINE bool plUniquePtr<T>::operator==(const plUniquePtr<T>& rhs) const
{
  return m_pInstance == rhs.m_pInstance;
}

template <typename T>
PL_ALWAYS_INLINE bool plUniquePtr<T>::operator!=(const plUniquePtr<T>& rhs) const
{
  return m_pInstance != rhs.m_pInstance;
}

template <typename T>
PL_ALWAYS_INLINE bool plUniquePtr<T>::operator<(const plUniquePtr<T>& rhs) const
{
  return m_pInstance < rhs.m_pInstance;
}

template <typename T>
PL_ALWAYS_INLINE bool plUniquePtr<T>::operator<=(const plUniquePtr<T>& rhs) const
{
  return !(rhs < *this);
}

template <typename T>
PL_ALWAYS_INLINE bool plUniquePtr<T>::operator>(const plUniquePtr<T>& rhs) const
{
  return rhs < *this;
}

template <typename T>
PL_ALWAYS_INLINE bool plUniquePtr<T>::operator>=(const plUniquePtr<T>& rhs) const
{
  return !(*this < rhs);
}

template <typename T>
PL_ALWAYS_INLINE bool plUniquePtr<T>::operator==(std::nullptr_t) const
{
  return m_pInstance == nullptr;
}

template <typename T>
PL_ALWAYS_INLINE bool plUniquePtr<T>::operator!=(std::nullptr_t) const
{
  return m_pInstance != nullptr;
}

template <typename T>
PL_ALWAYS_INLINE bool plUniquePtr<T>::operator<(std::nullptr_t) const
{
  return m_pInstance < nullptr;
}

template <typename T>
PL_ALWAYS_INLINE bool plUniquePtr<T>::operator<=(std::nullptr_t) const
{
  return m_pInstance <= nullptr;
}

template <typename T>
PL_ALWAYS_INLINE bool plUniquePtr<T>::operator>(std::nullptr_t) const
{
  return m_pInstance > nullptr;
}

template <typename T>
PL_ALWAYS_INLINE bool plUniquePtr<T>::operator>=(std::nullptr_t) const
{
  return m_pInstance >= nullptr;
}

//////////////////////////////////////////////////////////////////////////
// free functions

template <typename T>
PL_ALWAYS_INLINE bool operator==(const plUniquePtr<T>& lhs, const T* rhs)
{
  return lhs.Borrow() == rhs;
}

template <typename T>
PL_ALWAYS_INLINE bool operator==(const plUniquePtr<T>& lhs, T* rhs)
{
  return lhs.Borrow() == rhs;
}

template <typename T>
PL_ALWAYS_INLINE bool operator!=(const plUniquePtr<T>& lhs, const T* rhs)
{
  return lhs.Borrow() != rhs;
}

template <typename T>
PL_ALWAYS_INLINE bool operator!=(const plUniquePtr<T>& lhs, T* rhs)
{
  return lhs.Borrow() != rhs;
}

template <typename T>
PL_ALWAYS_INLINE bool operator==(const T* lhs, const plUniquePtr<T>& rhs)
{
  return lhs == rhs.Borrow();
}

template <typename T>
PL_ALWAYS_INLINE bool operator==(T* lhs, const plUniquePtr<T>& rhs)
{
  return lhs == rhs.Borrow();
}

template <typename T>
PL_ALWAYS_INLINE bool operator!=(const T* lhs, const plUniquePtr<T>& rhs)
{
  return lhs != rhs.Borrow();
}

template <typename T>
PL_ALWAYS_INLINE bool operator!=(T* lhs, const plUniquePtr<T>& rhs)
{
  return lhs != rhs.Borrow();
}
