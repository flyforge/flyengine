
PL_ALWAYS_INLINE plAllocator::plAllocator() = default;

PL_ALWAYS_INLINE plAllocator::~plAllocator() = default;


namespace plMath
{
  // due to #include order issues, we have to forward declare this function here

  PL_FOUNDATION_DLL plUInt64 SafeMultiply64(plUInt64 a, plUInt64 b, plUInt64 c, plUInt64 d);
} // namespace plMath

namespace plInternal
{
  template <typename T>
  struct NewInstance
  {
    PL_ALWAYS_INLINE NewInstance(T* pInstance, plAllocator* pAllocator)
    {
      m_pInstance = pInstance;
      m_pAllocator = pAllocator;
    }

    template <typename U>
    PL_ALWAYS_INLINE NewInstance(NewInstance<U>&& other)
    {
      m_pInstance = other.m_pInstance;
      m_pAllocator = other.m_pAllocator;

      other.m_pInstance = nullptr;
      other.m_pAllocator = nullptr;
    }

    PL_ALWAYS_INLINE NewInstance(std::nullptr_t) {}

    template <typename U>
    PL_ALWAYS_INLINE NewInstance<U> Cast()
    {
      return NewInstance<U>(static_cast<U*>(m_pInstance), m_pAllocator);
    }

    PL_ALWAYS_INLINE operator T*() { return m_pInstance; }

    PL_ALWAYS_INLINE T* operator->() { return m_pInstance; }

    T* m_pInstance = nullptr;
    plAllocator* m_pAllocator = nullptr;
  };

  template <typename T>
  PL_ALWAYS_INLINE bool operator<(const NewInstance<T>& lhs, T* rhs)
  {
    return lhs.m_pInstance < rhs;
  }

  template <typename T>
  PL_ALWAYS_INLINE bool operator<(T* lhs, const NewInstance<T>& rhs)
  {
    return lhs < rhs.m_pInstance;
  }

  template <typename T>
  PL_FORCE_INLINE void Delete(plAllocator* pAllocator, T* pPtr)
  {
    if (pPtr != nullptr)
    {
      plMemoryUtils::Destruct(pPtr, 1);
      pAllocator->Deallocate(pPtr);
    }
  }

  template <typename T>
  PL_FORCE_INLINE T* CreateRawBuffer(plAllocator* pAllocator, size_t uiCount)
  {
    plUInt64 safeAllocationSize = plMath::SafeMultiply64(uiCount, sizeof(T));
    return static_cast<T*>(pAllocator->Allocate(static_cast<size_t>(safeAllocationSize), PL_ALIGNMENT_OF(T))); // Down-cast to size_t for 32-bit
  }

  PL_FORCE_INLINE void DeleteRawBuffer(plAllocator* pAllocator, void* pPtr)
  {
    if (pPtr != nullptr)
    {
      pAllocator->Deallocate(pPtr);
    }
  }

  template <typename T>
  inline plArrayPtr<T> CreateArray(plAllocator* pAllocator, plUInt32 uiCount)
  {
    T* buffer = CreateRawBuffer<T>(pAllocator, uiCount);
    plMemoryUtils::Construct<SkipTrivialTypes>(buffer, uiCount);

    return plArrayPtr<T>(buffer, uiCount);
  }

  template <typename T>
  inline void DeleteArray(plAllocator* pAllocator, plArrayPtr<T> arrayPtr)
  {
    T* buffer = arrayPtr.GetPtr();
    if (buffer != nullptr)
    {
      plMemoryUtils::Destruct(buffer, arrayPtr.GetCount());
      pAllocator->Deallocate(buffer);
    }
  }

  template <typename T>
  PL_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, plAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount, plTypeIsPod)
  {
    return (T*)pAllocator->Reallocate(pPtr, uiCurrentCount * sizeof(T), uiNewCount * sizeof(T), PL_ALIGNMENT_OF(T));
  }

  template <typename T>
  PL_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, plAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount, plTypeIsMemRelocatable)
  {
    return (T*)pAllocator->Reallocate(pPtr, uiCurrentCount * sizeof(T), uiNewCount * sizeof(T), PL_ALIGNMENT_OF(T));
  }

  template <typename T>
  PL_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, plAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount, plTypeIsClass)
  {
    PL_CHECK_AT_COMPILETIME_MSG(!std::is_trivial<T>::value,
      "POD type is treated as class. Use PL_DECLARE_POD_TYPE(YourClass) or PL_DEFINE_AS_POD_TYPE(ExternalClass) to mark it as POD.");

    T* pNewMem = CreateRawBuffer<T>(pAllocator, uiNewCount);
    plMemoryUtils::RelocateConstruct(pNewMem, pPtr, uiCurrentCount);
    DeleteRawBuffer(pAllocator, pPtr);
    return pNewMem;
  }

  template <typename T>
  PL_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, plAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount)
  {
    PL_ASSERT_DEV(uiCurrentCount < uiNewCount, "Shrinking of a buffer is not implemented yet");
    PL_ASSERT_DEV(!(uiCurrentCount == uiNewCount), "Same size passed in twice.");
    if (pPtr == nullptr)
    {
      PL_ASSERT_DEV(uiCurrentCount == 0, "current count must be 0 if ptr is nullptr");

      return CreateRawBuffer<T>(pAllocator, uiNewCount);
    }
    return ExtendRawBuffer(pPtr, pAllocator, uiCurrentCount, uiNewCount, plGetTypeClass<T>());
  }
} // namespace plInternal
