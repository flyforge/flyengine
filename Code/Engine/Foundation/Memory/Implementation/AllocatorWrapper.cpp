#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/AllocatorWrapper.h>

static thread_local plAllocator* s_pAllocator = nullptr;

plLocalAllocatorWrapper::plLocalAllocatorWrapper(plAllocator* pAllocator)
{
  s_pAllocator = pAllocator;
}

void plLocalAllocatorWrapper::Reset()
{
  s_pAllocator = nullptr;
}

plAllocator* plLocalAllocatorWrapper::GetAllocator()
{
  return s_pAllocator;
}


