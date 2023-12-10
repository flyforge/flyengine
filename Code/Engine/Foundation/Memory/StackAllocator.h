#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/Policies/StackAllocation.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

template <plUInt32 TrackingFlags = plMemoryTrackingFlags::Default>
class plStackAllocator : public plAllocator<plMemoryPolicies::plStackAllocation, TrackingFlags>
{
public:
  plStackAllocator(plStringView sName, plAllocatorBase* pParent);
  ~plStackAllocator();

  virtual void* Allocate(size_t uiSize, size_t uiAlign, plMemoryUtils::DestructorFunction destructorFunc) override;
  virtual void Deallocate(void* pPtr) override;

  /// \brief
  ///   Resets the allocator freeing all memory.
  void Reset();

private:
  struct DestructData
  {
    PLASMA_DECLARE_POD_TYPE();

    plMemoryUtils::DestructorFunction m_Func;
    void* m_Ptr;
  };

  plMutex m_Mutex;
  plDynamicArray<DestructData> m_DestructData;
  plHashTable<void*, plUInt32> m_PtrToDestructDataIndexTable;
};

#include <Foundation/Memory/Implementation/StackAllocator_inl.h>
