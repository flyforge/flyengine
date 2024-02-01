#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Memory/MemoryUtils.h>

template <typename T>
PL_ALWAYS_INLINE plHashableStruct<T>::plHashableStruct()
{
  plMemoryUtils::ZeroFill<T>(static_cast<T*>(this), 1);
}

template <typename T>
PL_ALWAYS_INLINE plHashableStruct<T>::plHashableStruct(const plHashableStruct<T>& other)
{
  plMemoryUtils::RawByteCopy(this, &other, sizeof(T));
}

template <typename T>
PL_ALWAYS_INLINE void plHashableStruct<T>::operator=(const plHashableStruct<T>& other)
{
  if (this != &other)
  {
    plMemoryUtils::RawByteCopy(this, &other, sizeof(T));
  }
}

template <typename T>
PL_ALWAYS_INLINE plUInt32 plHashableStruct<T>::CalculateHash() const
{
  return plHashingUtils::xxHash32(this, sizeof(T));
}
