
PLASMA_ALWAYS_INLINE plAllocatorBase::plAllocatorBase() = default;

PLASMA_ALWAYS_INLINE plAllocatorBase::~plAllocatorBase() = default;


namespace plMath
{
  // due to #include order issues, we have to forward declare this function here

  PLASMA_FOUNDATION_DLL plUInt64 SafeMultiply64(plUInt64 a, plUInt64 b, plUInt64 c, plUInt64 d);
} // namespace plMath

namespace plInternal
{
  template <typename T>
  struct NewInstance
  {
    PLASMA_ALWAYS_INLINE NewInstance(T* pInstance, plAllocatorBase* pAllocator)
    {
      m_pInstance = pInstance;
      m_pAllocator = pAllocator;
    }

    template <typename U>
    PLASMA_ALWAYS_INLINE NewInstance(NewInstance<U>&& other)
    {
      m_pInstance = other.m_pInstance;
      m_pAllocator = other.m_pAllocator;

      other.m_pInstance = nullptr;
      other.m_pAllocator = nullptr;
    }

    PLASMA_ALWAYS_INLINE NewInstance(std::nullptr_t) {}

    template <typename U>
    PLASMA_ALWAYS_INLINE NewInstance<U> Cast()
    {
      return NewInstance<U>(static_cast<U*>(m_pInstance), m_pAllocator);
    }

    PLASMA_ALWAYS_INLINE operator T*() { return m_pInstance; }

    PLASMA_ALWAYS_INLINE T* operator->() { return m_pInstance; }

    T* m_pInstance = nullptr;
    plAllocatorBase* m_pAllocator = nullptr;
  };

  template <typename T>
  PLASMA_ALWAYS_INLINE bool operator<(const NewInstance<T>& lhs, T* rhs)
  {
    return lhs.m_pInstance < rhs;
  }

  template <typename T>
  PLASMA_ALWAYS_INLINE bool operator<(T* lhs, const NewInstance<T>& rhs)
  {
    return lhs < rhs.m_pInstance;
  }

  template <typename T>
  PLASMA_FORCE_INLINE void Delete(plAllocatorBase* pAllocator, T* pPtr)
  {
    if (pPtr != nullptr)
    {
      plMemoryUtils::Destruct(pPtr, 1);
      pAllocator->Deallocate(pPtr);
    }
  }

  template <typename T>
  PLASMA_FORCE_INLINE T* CreateRawBuffer(plAllocatorBase* pAllocator, size_t uiCount)
  {
    plUInt64 safeAllocationSize = plMath::SafeMultiply64(uiCount, sizeof(T));
    return static_cast<T*>(pAllocator->Allocate(static_cast<size_t>(safeAllocationSize), PLASMA_ALIGNMENT_OF(T))); // Down-cast to size_t for 32-bit
  }

  PLASMA_FORCE_INLINE void DeleteRawBuffer(plAllocatorBase* pAllocator, void* pPtr)
  {
    if (pPtr != nullptr)
    {
      pAllocator->Deallocate(pPtr);
    }
  }

  template <typename T>
  inline plArrayPtr<T> CreateArray(plAllocatorBase* pAllocator, plUInt32 uiCount)
  {
    T* buffer = CreateRawBuffer<T>(pAllocator, uiCount);
    plMemoryUtils::Construct(buffer, uiCount);

    return plArrayPtr<T>(buffer, uiCount);
  }

  template <typename T>
  inline void DeleteArray(plAllocatorBase* pAllocator, plArrayPtr<T> arrayPtr)
  {
    T* buffer = arrayPtr.GetPtr();
    if (buffer != nullptr)
    {
      plMemoryUtils::Destruct(buffer, arrayPtr.GetCount());
      pAllocator->Deallocate(buffer);
    }
  }

  template <typename T>
  PLASMA_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, plAllocatorBase* pAllocator, size_t uiCurrentCount, size_t uiNewCount, plTypeIsPod)
  {
    return (T*)pAllocator->Reallocate(pPtr, uiCurrentCount * sizeof(T), uiNewCount * sizeof(T), PLASMA_ALIGNMENT_OF(T));
  }

  template <typename T>
  PLASMA_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, plAllocatorBase* pAllocator, size_t uiCurrentCount, size_t uiNewCount, plTypeIsMemRelocatable)
  {
    return (T*)pAllocator->Reallocate(pPtr, uiCurrentCount * sizeof(T), uiNewCount * sizeof(T), PLASMA_ALIGNMENT_OF(T));
  }

  template <typename T>
  PLASMA_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, plAllocatorBase* pAllocator, size_t uiCurrentCount, size_t uiNewCount, plTypeIsClass)
  {
    PLASMA_CHECK_AT_COMPILETIME_MSG(!std::is_trivial<T>::value,
      "POD type is treated as class. Use PLASMA_DECLARE_POD_TYPE(YourClass) or PLASMA_DEFINE_AS_POD_TYPE(ExternalClass) to mark it as POD.");

    T* pNewMem = CreateRawBuffer<T>(pAllocator, uiNewCount);
    plMemoryUtils::RelocateConstruct(pNewMem, pPtr, uiCurrentCount);
    DeleteRawBuffer(pAllocator, pPtr);
    return pNewMem;
  }

  template <typename T>
  PLASMA_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, plAllocatorBase* pAllocator, size_t uiCurrentCount, size_t uiNewCount)
  {
    PLASMA_ASSERT_DEV(uiCurrentCount < uiNewCount, "Shrinking of a buffer is not implemented yet");
    PLASMA_ASSERT_DEV(!(uiCurrentCount == uiNewCount), "Same size passed in twice.");
    if (pPtr == nullptr)
    {
      PLASMA_ASSERT_DEV(uiCurrentCount == 0, "current count must be 0 if ptr is nullptr");

      return CreateRawBuffer<T>(pAllocator, uiNewCount);
    }
    return ExtendRawBuffer(pPtr, pAllocator, uiCurrentCount, uiNewCount, plGetTypeClass<T>());
  }
} // namespace plInternal
