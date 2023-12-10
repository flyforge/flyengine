#include <Foundation/Containers/HybridArray.h>

template <typename T, plUInt32 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
plHybridArray<T, Size, AllocatorWrapper>::plHybridArray()
  : plDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
}

template <typename T, plUInt32 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
plHybridArray<T, Size, AllocatorWrapper>::plHybridArray(plAllocatorBase* pAllocator)
  : plDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, pAllocator)
{
}

template <typename T, plUInt32 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
plHybridArray<T, Size, AllocatorWrapper>::plHybridArray(const plHybridArray<T, Size, AllocatorWrapper>& other)
  : plDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
  *this = other;
}

template <typename T, plUInt32 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
plHybridArray<T, Size, AllocatorWrapper>::plHybridArray(const plArrayPtr<const T>& other)
  : plDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
  *this = other;
}

template <typename T, plUInt32 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
plHybridArray<T, Size, AllocatorWrapper>::plHybridArray(plHybridArray<T, Size, AllocatorWrapper>&& other)
  : plDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, other.GetAllocator())
{
  *this = std::move(other);
}

template <typename T, plUInt32 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
void plHybridArray<T, Size, AllocatorWrapper>::operator=(const plHybridArray<T, Size, AllocatorWrapper>& rhs)
{
  plDynamicArray<T, AllocatorWrapper>::operator=(rhs);
}

template <typename T, plUInt32 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
void plHybridArray<T, Size, AllocatorWrapper>::operator=(const plArrayPtr<const T>& rhs)
{
  plDynamicArray<T, AllocatorWrapper>::operator=(rhs);
}

template <typename T, plUInt32 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
void plHybridArray<T, Size, AllocatorWrapper>::operator=(plHybridArray<T, Size, AllocatorWrapper>&& rhs) noexcept
{
  plDynamicArray<T, AllocatorWrapper>::operator=(std::move(rhs));
}
