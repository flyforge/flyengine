#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Memory/LargeBlockAllocator.h>

struct plBlockStorageType
{
  enum Enum
  {
    Compact,
    FreeList
  };
};

template <typename T, plUInt32 BlockSizeInByte, plBlockStorageType::Enum StorageType>
class plBlockStorage
{
public:
  class ConstIterator
  {
  public:
    const T& operator*() const;
    const T* operator->() const;

    operator const T*() const;

    void Next();
    bool IsValid() const;

    void operator++();

  protected:
    friend class plBlockStorage<T, BlockSizeInByte, StorageType>;

    ConstIterator(const plBlockStorage<T, BlockSizeInByte, StorageType>& storage, plUInt32 uiStartIndex, plUInt32 uiCount);

    T& CurrentElement() const;

    const plBlockStorage<T, BlockSizeInByte, StorageType>& m_Storage;
    plUInt32 m_uiCurrentIndex;
    plUInt32 m_uiEndIndex;
  };

  class Iterator : public ConstIterator
  {
  public:
    T& operator*();
    T* operator->();

    operator T*();

  private:
    friend class plBlockStorage<T, BlockSizeInByte, StorageType>;

    Iterator(const plBlockStorage<T, BlockSizeInByte, StorageType>& storage, plUInt32 uiStartIndex, plUInt32 uiCount);
  };

  plBlockStorage(plLargeBlockAllocator<BlockSizeInByte>* pBlockAllocator, plAllocator* pAllocator);
  ~plBlockStorage();

  void Clear();

  T* Create();
  void Delete(T* pObject);
  void Delete(T* pObject, T*& out_pMovedObject);

  plUInt32 GetCount() const;
  Iterator GetIterator(plUInt32 uiStartIndex = 0, plUInt32 uiCount = plInvalidIndex);
  ConstIterator GetIterator(plUInt32 uiStartIndex = 0, plUInt32 uiCount = plInvalidIndex) const;

private:
  void Delete(T* pObject, T*& out_pMovedObject, plTraitInt<plBlockStorageType::Compact>);
  void Delete(T* pObject, T*& out_pMovedObject, plTraitInt<plBlockStorageType::FreeList>);

  plLargeBlockAllocator<BlockSizeInByte>* m_pBlockAllocator;

  plDynamicArray<plDataBlock<T, BlockSizeInByte>> m_Blocks;
  plUInt32 m_uiCount = 0;

  plUInt32 m_uiFreelistStart = plInvalidIndex;

  plDynamicBitfield m_UsedEntries;
};

#include <Foundation/Memory/Implementation/BlockStorage_inl.h>
