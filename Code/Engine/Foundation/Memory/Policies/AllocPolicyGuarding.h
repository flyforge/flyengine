#pragma once

#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

class plAllocPolicyGuarding
{
public:
  plAllocPolicyGuarding(plAllocator* pParent);
  PL_ALWAYS_INLINE ~plAllocPolicyGuarding() = default;

  void* Allocate(size_t uiSize, size_t uiAlign);
  void Deallocate(void* pPtr);

  PL_ALWAYS_INLINE plAllocator* GetParent() const { return nullptr; }

private:
  plMutex m_Mutex;

  plUInt32 m_uiPageSize;

  plStaticRingBuffer<void*, (1 << 16)> m_AllocationsToFreeLater;
};
