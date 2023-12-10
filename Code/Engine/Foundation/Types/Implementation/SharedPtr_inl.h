
template <typename T>
PLASMA_ALWAYS_INLINE plSharedPtr<T>::plSharedPtr()
{
  m_pInstance = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
template <typename U>
PLASMA_ALWAYS_INLINE plSharedPtr<T>::plSharedPtr(const plInternal::NewInstance<U>& instance)
{
  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;

  AddReferenceIfValid();
}

template <typename T>
template <typename U>
PLASMA_ALWAYS_INLINE plSharedPtr<T>::plSharedPtr(U* pInstance, plAllocatorBase* pAllocator)
{
  m_pInstance = pInstance;
  m_pAllocator = pAllocator;

  AddReferenceIfValid();
}

template <typename T>
PLASMA_ALWAYS_INLINE plSharedPtr<T>::plSharedPtr(const plSharedPtr<T>& other)
{
  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  AddReferenceIfValid();
}

template <typename T>
template <typename U>
PLASMA_ALWAYS_INLINE plSharedPtr<T>::plSharedPtr(const plSharedPtr<U>& other)
{
  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  AddReferenceIfValid();
}

template <typename T>
template <typename U>
PLASMA_ALWAYS_INLINE plSharedPtr<T>::plSharedPtr(plSharedPtr<U>&& other)
{
  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance = nullptr;
  other.m_pAllocator = nullptr;
}

template <typename T>
template <typename U>
PLASMA_ALWAYS_INLINE plSharedPtr<T>::plSharedPtr(plUniquePtr<U>&& other)
{
  m_pInstance = other.Release(m_pAllocator);

  AddReferenceIfValid();
}

template <typename T>
PLASMA_ALWAYS_INLINE plSharedPtr<T>::plSharedPtr(std::nullptr_t)
{
  m_pInstance = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
PLASMA_ALWAYS_INLINE plSharedPtr<T>::~plSharedPtr()
{
  ReleaseReferenceIfValid();
}

template <typename T>
template <typename U>
PLASMA_ALWAYS_INLINE plSharedPtr<T>& plSharedPtr<T>::operator=(const plInternal::NewInstance<U>& instance)
{
  ReleaseReferenceIfValid();

  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;

  AddReferenceIfValid();

  return *this;
}

template <typename T>
PLASMA_ALWAYS_INLINE plSharedPtr<T>& plSharedPtr<T>::operator=(const plSharedPtr<T>& other)
{
  if (m_pInstance != other.m_pInstance)
  {
    ReleaseReferenceIfValid();

    m_pInstance = other.m_pInstance;
    m_pAllocator = other.m_pAllocator;

    AddReferenceIfValid();
  }

  return *this;
}

template <typename T>
template <typename U>
PLASMA_ALWAYS_INLINE plSharedPtr<T>& plSharedPtr<T>::operator=(const plSharedPtr<U>& other)
{
  if (m_pInstance != other.m_pInstance)
  {
    ReleaseReferenceIfValid();

    m_pInstance = other.m_pInstance;
    m_pAllocator = other.m_pAllocator;

    AddReferenceIfValid();
  }

  return *this;
}

template <typename T>
template <typename U>
PLASMA_ALWAYS_INLINE plSharedPtr<T>& plSharedPtr<T>::operator=(plSharedPtr<U>&& other)
{
  if (m_pInstance != other.m_pInstance)
  {
    ReleaseReferenceIfValid();

    m_pInstance = other.m_pInstance;
    m_pAllocator = other.m_pAllocator;

    other.m_pInstance = nullptr;
    other.m_pAllocator = nullptr;
  }

  return *this;
}

template <typename T>
template <typename U>
PLASMA_ALWAYS_INLINE plSharedPtr<T>& plSharedPtr<T>::operator=(plUniquePtr<U>&& other)
{
  ReleaseReferenceIfValid();

  m_pInstance = other.Release(m_pAllocator);

  AddReferenceIfValid();

  return *this;
}

template <typename T>
PLASMA_ALWAYS_INLINE plSharedPtr<T>& plSharedPtr<T>::operator=(std::nullptr_t)
{
  ReleaseReferenceIfValid();

  return *this;
}

template <typename T>
PLASMA_ALWAYS_INLINE T* plSharedPtr<T>::Borrow() const
{
  return m_pInstance;
}

template <typename T>
PLASMA_ALWAYS_INLINE void plSharedPtr<T>::Clear()
{
  ReleaseReferenceIfValid();
}

template <typename T>
PLASMA_ALWAYS_INLINE T& plSharedPtr<T>::operator*() const
{
  return *m_pInstance;
}

template <typename T>
PLASMA_ALWAYS_INLINE T* plSharedPtr<T>::operator->() const
{
  return m_pInstance;
}

template <typename T>
PLASMA_ALWAYS_INLINE plSharedPtr<T>::operator const T*() const
{
  return m_pInstance;
}

template <typename T>
PLASMA_ALWAYS_INLINE plSharedPtr<T>::operator T*()
{
  return m_pInstance;
}

template <typename T>
PLASMA_ALWAYS_INLINE plSharedPtr<T>::operator bool() const
{
  return m_pInstance != nullptr;
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plSharedPtr<T>::operator==(const plSharedPtr<T>& rhs) const
{
  return m_pInstance == rhs.m_pInstance;
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plSharedPtr<T>::operator!=(const plSharedPtr<T>& rhs) const
{
  return m_pInstance != rhs.m_pInstance;
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plSharedPtr<T>::operator<(const plSharedPtr<T>& rhs) const
{
  return m_pInstance < rhs.m_pInstance;
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plSharedPtr<T>::operator<=(const plSharedPtr<T>& rhs) const
{
  return !(rhs < *this);
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plSharedPtr<T>::operator>(const plSharedPtr<T>& rhs) const
{
  return rhs < *this;
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plSharedPtr<T>::operator>=(const plSharedPtr<T>& rhs) const
{
  return !(*this < rhs);
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plSharedPtr<T>::operator==(std::nullptr_t) const
{
  return m_pInstance == nullptr;
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plSharedPtr<T>::operator!=(std::nullptr_t) const
{
  return m_pInstance != nullptr;
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plSharedPtr<T>::operator<(std::nullptr_t) const
{
  return m_pInstance < nullptr;
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plSharedPtr<T>::operator<=(std::nullptr_t) const
{
  return m_pInstance <= nullptr;
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plSharedPtr<T>::operator>(std::nullptr_t) const
{
  return m_pInstance > nullptr;
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plSharedPtr<T>::operator>=(std::nullptr_t) const
{
  return m_pInstance >= nullptr;
}

template <typename T>
PLASMA_ALWAYS_INLINE void plSharedPtr<T>::AddReferenceIfValid()
{
  if (m_pInstance != nullptr)
  {
    m_pInstance->AddRef();
  }
}

template <typename T>
PLASMA_ALWAYS_INLINE void plSharedPtr<T>::ReleaseReferenceIfValid()
{
  if (m_pInstance != nullptr)
  {
    if (m_pInstance->ReleaseRef() == 0)
    {
      auto pNonConstInstance = const_cast<typename plTypeTraits<T>::NonConstType*>(m_pInstance);
      PLASMA_DELETE(m_pAllocator, pNonConstInstance);
    }

    m_pInstance = nullptr;
    m_pAllocator = nullptr;
  }
}
