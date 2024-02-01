
template <typename T>
plDynamicArrayBase<T>::plDynamicArrayBase(plAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
}

template <typename T>
plDynamicArrayBase<T>::plDynamicArrayBase(T* pInplaceStorage, plUInt32 uiCapacity, plAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
  m_pAllocator.SetFlags(Storage::External);
  this->m_uiCapacity = uiCapacity;
  this->m_pElements = pInplaceStorage;
}

template <typename T>
plDynamicArrayBase<T>::plDynamicArrayBase(const plDynamicArrayBase<T>& other, plAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
  plArrayBase<T, plDynamicArrayBase<T>>::operator=((plArrayPtr<const T>)other); // redirect this to the plArrayPtr version
}

template <typename T>
plDynamicArrayBase<T>::plDynamicArrayBase(plDynamicArrayBase<T>&& other, plAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
  *this = std::move(other);
}

template <typename T>
plDynamicArrayBase<T>::plDynamicArrayBase(const plArrayPtr<const T>& other, plAllocator* pAllocator)
  : m_pAllocator(pAllocator)
{
  plArrayBase<T, plDynamicArrayBase<T>>::operator=(other);
}

template <typename T>
plDynamicArrayBase<T>::~plDynamicArrayBase()
{
  this->Clear();

  if (m_pAllocator.GetFlags() == Storage::Owned)
  {
    // only delete our storage, if we own it
    PL_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);
  }

  this->m_uiCapacity = 0;
  this->m_pElements = nullptr;
}

template <typename T>
PL_ALWAYS_INLINE void plDynamicArrayBase<T>::operator=(const plDynamicArrayBase<T>& rhs)
{
  plArrayBase<T, plDynamicArrayBase<T>>::operator=((plArrayPtr<const T>)rhs); // redirect this to the plArrayPtr version
}

template <typename T>
inline void plDynamicArrayBase<T>::operator=(plDynamicArrayBase<T>&& rhs) noexcept
{
  // Clear any existing data (calls destructors if necessary)
  this->Clear();

  if (this->m_pAllocator.GetPtr() == rhs.m_pAllocator.GetPtr() && rhs.m_pAllocator.GetFlags() == Storage::Owned) // only move the storage of rhs, if it owns it
  {
    if (this->m_pAllocator.GetFlags() == Storage::Owned)
    {
      // only delete our storage, if we own it
      PL_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);
    }

    // we now own this storage
    this->m_pAllocator.SetFlags(Storage::Owned);

    // move the data over from the other array
    this->m_uiCount = rhs.m_uiCount;
    this->m_uiCapacity = rhs.m_uiCapacity;
    this->m_pElements = rhs.m_pElements;

    // reset the other array to not reference the data anymore
    rhs.m_pElements = nullptr;
    rhs.m_uiCount = 0;
    rhs.m_uiCapacity = 0;
  }
  else
  {
    // Ensure we have enough data.
    this->Reserve(rhs.m_uiCount);
    this->m_uiCount = rhs.m_uiCount;

    plMemoryUtils::RelocateConstruct(
      this->GetElementsPtr(), rhs.GetElementsPtr() /* vital to remap rhs.m_pElements to absolute ptr */, rhs.m_uiCount);

    rhs.m_uiCount = 0;
  }
}

template <typename T>
void plDynamicArrayBase<T>::Swap(plDynamicArrayBase<T>& other)
{
  if (this->m_pAllocator.GetFlags() == Storage::External && other.m_pAllocator.GetFlags() == Storage::External)
  {
    constexpr plUInt32 InplaceStorageSize = 64;

    struct alignas(PL_ALIGNMENT_OF(T)) Tmp
    {
      plUInt8 m_StaticData[InplaceStorageSize * sizeof(T)];
    };

    const plUInt32 localSize = this->m_uiCount;
    const plUInt32 otherLocalSize = other.m_uiCount;

    if (localSize <= InplaceStorageSize && otherLocalSize <= InplaceStorageSize && localSize <= other.m_uiCapacity &&
        otherLocalSize <= this->m_uiCapacity)
    {

      Tmp tmp;
      plMemoryUtils::RelocateConstruct(reinterpret_cast<T*>(tmp.m_StaticData), this->GetElementsPtr(), localSize);
      plMemoryUtils::RelocateConstruct(this->GetElementsPtr(), other.GetElementsPtr(), otherLocalSize);
      plMemoryUtils::RelocateConstruct(other.GetElementsPtr(), reinterpret_cast<T*>(tmp.m_StaticData), localSize);

      plMath::Swap(this->m_pAllocator, other.m_pAllocator);
      plMath::Swap(this->m_uiCount, other.m_uiCount);

      return; // successfully swapped in place
    }

    // temp buffer was insufficient -> fallthrough
  }

  if (this->m_pAllocator.GetFlags() == Storage::External)
  {
    // enforce using own storage
    this->Reserve(this->m_uiCapacity + 1);
  }

  if (other.m_pAllocator.GetFlags() == Storage::External)
  {
    // enforce using own storage
    other.Reserve(other.m_uiCapacity + 1);
  }

  // no external storage involved -> swap pointers
  plMath::Swap(this->m_pAllocator, other.m_pAllocator);
  this->DoSwap(other);
}

template <typename T>
void plDynamicArrayBase<T>::SetCapacity(plUInt32 uiCapacity)
{
  // do NOT early out here, it is vital that this function does its thing even if the old capacity would be sufficient

  if (this->m_pAllocator.GetFlags() == Storage::Owned && uiCapacity > this->m_uiCapacity)
  {
    this->m_pElements = PL_EXTEND_RAW_BUFFER(this->m_pAllocator, this->m_pElements, this->m_uiCount, uiCapacity);
  }
  else
  {
    T* pOldElements = GetElementsPtr();

    T* pNewElements = PL_NEW_RAW_BUFFER(this->m_pAllocator, T, uiCapacity);
    plMemoryUtils::RelocateConstruct(pNewElements, pOldElements, this->m_uiCount);

    if (this->m_pAllocator.GetFlags() == Storage::Owned)
    {
      PL_DELETE_RAW_BUFFER(this->m_pAllocator, pOldElements);
    }

    // after any resize, we definitely own the storage
    this->m_pAllocator.SetFlags(Storage::Owned);
    this->m_pElements = pNewElements;
  }

  this->m_uiCapacity = uiCapacity;
}

template <typename T>
void plDynamicArrayBase<T>::Reserve(plUInt32 uiCapacity)
{
  if (this->m_uiCapacity >= uiCapacity)
    return;

  const plUInt64 uiCurCap64 = static_cast<plUInt64>(this->m_uiCapacity);
  plUInt64 uiNewCapacity64 = uiCurCap64 + (uiCurCap64 / 2);

  uiNewCapacity64 = plMath::Max<plUInt64>(uiNewCapacity64, uiCapacity);

  constexpr plUInt64 uiMaxCapacity = 0xFFFFFFFFllu - (CAPACITY_ALIGNMENT - 1);

  // the maximum value must leave room for the capacity alignment computation below (without overflowing the 32 bit range)
  uiNewCapacity64 = plMath::Min<plUInt64>(uiNewCapacity64, uiMaxCapacity);

  uiNewCapacity64 = (uiNewCapacity64 + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);

  PL_ASSERT_DEV(uiCapacity <= uiNewCapacity64, "The requested capacity of {} elements exceeds the maximum possible capacity of {} elements.", uiCapacity, uiMaxCapacity);

  SetCapacity(static_cast<plUInt32>(uiNewCapacity64 & 0xFFFFFFFF));
}

template <typename T>
void plDynamicArrayBase<T>::Compact()
{
  if (m_pAllocator.GetFlags() == Storage::External)
    return;

  if (this->IsEmpty())
  {
    // completely deallocate all data, if the array is empty.
    PL_DELETE_RAW_BUFFER(this->m_pAllocator, this->m_pElements);
    this->m_uiCapacity = 0;
  }
  else
  {
    const plUInt32 uiNewCapacity = (this->m_uiCount + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
    if (this->m_uiCapacity != uiNewCapacity)
      SetCapacity(uiNewCapacity);
  }
}

template <typename T>
PL_ALWAYS_INLINE T* plDynamicArrayBase<T>::GetElementsPtr()
{
  return this->m_pElements;
}

template <typename T>
PL_ALWAYS_INLINE const T* plDynamicArrayBase<T>::GetElementsPtr() const
{
  return this->m_pElements;
}

template <typename T>
plUInt64 plDynamicArrayBase<T>::GetHeapMemoryUsage() const
{
  if (this->m_pAllocator.GetFlags() == Storage::External)
    return 0;

  return (plUInt64)this->m_uiCapacity * (plUInt64)sizeof(T);
}

template <typename T, typename A>
plDynamicArray<T, A>::plDynamicArray()
  : plDynamicArrayBase<T>(A::GetAllocator())
{
}

template <typename T, typename A>
plDynamicArray<T, A>::plDynamicArray(plAllocator* pAllocator)
  : plDynamicArrayBase<T>(pAllocator)
{
}

template <typename T, typename A>
plDynamicArray<T, A>::plDynamicArray(const plDynamicArray<T, A>& other)
  : plDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
plDynamicArray<T, A>::plDynamicArray(const plDynamicArrayBase<T>& other)
  : plDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
plDynamicArray<T, A>::plDynamicArray(const plArrayPtr<const T>& other)
  : plDynamicArrayBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
plDynamicArray<T, A>::plDynamicArray(plDynamicArray<T, A>&& other)
  : plDynamicArrayBase<T>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A>
plDynamicArray<T, A>::plDynamicArray(plDynamicArrayBase<T>&& other)
  : plDynamicArrayBase<T>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A>
void plDynamicArray<T, A>::operator=(const plDynamicArray<T, A>& rhs)
{
  plDynamicArrayBase<T>::operator=(rhs);
}

template <typename T, typename A>
void plDynamicArray<T, A>::operator=(const plDynamicArrayBase<T>& rhs)
{
  plDynamicArrayBase<T>::operator=(rhs);
}

template <typename T, typename A>
void plDynamicArray<T, A>::operator=(const plArrayPtr<const T>& rhs)
{
  plArrayBase<T, plDynamicArrayBase<T>>::operator=(rhs);
}

template <typename T, typename A>
void plDynamicArray<T, A>::operator=(plDynamicArray<T, A>&& rhs) noexcept
{
  plDynamicArrayBase<T>::operator=(std::move(rhs));
}

template <typename T, typename A>
void plDynamicArray<T, A>::operator=(plDynamicArrayBase<T>&& rhs) noexcept
{
  plDynamicArrayBase<T>::operator=(std::move(rhs));
}

template <typename T, typename AllocatorWrapper>
plArrayPtr<const T* const> plMakeArrayPtr(const plDynamicArray<T*, AllocatorWrapper>& dynArray)
{
  return plArrayPtr<const T* const>(dynArray.GetData(), dynArray.GetCount());
}

template <typename T, typename AllocatorWrapper>
plArrayPtr<const T> plMakeArrayPtr(const plDynamicArray<T, AllocatorWrapper>& dynArray)
{
  return plArrayPtr<const T>(dynArray.GetData(), dynArray.GetCount());
}

template <typename T, typename AllocatorWrapper>
plArrayPtr<T> plMakeArrayPtr(plDynamicArray<T, AllocatorWrapper>& in_dynArray)
{
  return plArrayPtr<T>(in_dynArray.GetData(), in_dynArray.GetCount());
}
