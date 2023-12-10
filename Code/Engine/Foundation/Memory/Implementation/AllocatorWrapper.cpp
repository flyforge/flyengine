#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/AllocatorWrapper.h>

static thread_local plAllocatorBase* s_pAllocator = nullptr;

plLocalAllocatorWrapper::plLocalAllocatorWrapper(plAllocatorBase* pAllocator)
{
  s_pAllocator = pAllocator;
}

void plLocalAllocatorWrapper::Reset()
{
  s_pAllocator = nullptr;
}

plAllocatorBase* plLocalAllocatorWrapper::GetAllocator()
{
  return s_pAllocator;
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_AllocatorWrapper);
