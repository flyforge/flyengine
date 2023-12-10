#pragma once

#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

namespace plMemoryPolicies
{
  class plGuardedAllocation
  {
  public:
    plGuardedAllocation(plAllocatorBase* pParent);
    PLASMA_ALWAYS_INLINE ~plGuardedAllocation() = default;

    void* Allocate(size_t uiSize, size_t uiAlign);
    void Deallocate(void* pPtr);

    PLASMA_ALWAYS_INLINE plAllocatorBase* GetParent() const { return nullptr; }

  private:
    plMutex m_Mutex;

    plUInt32 m_uiPageSize;

    plStaticRingBuffer<void*, (1 << 16)> m_AllocationsToFreeLater;
  };
} // namespace plMemoryPolicies
