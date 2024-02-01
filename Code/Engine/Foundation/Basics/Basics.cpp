#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/CommonAllocators.h>

#if PL_ENABLED(PL_ALLOC_GUARD_ALLOCATIONS)
using DefaultHeapType = plGuardingAllocator;
using DefaultAlignedHeapType = plGuardingAllocator;
using DefaultStaticsHeapType = plAllocatorWithPolicy<plAllocPolicyGuarding, plAllocatorTrackingMode::AllocationStatsIgnoreLeaks>;
#else
using DefaultHeapType = plHeapAllocator;
using DefaultAlignedHeapType = plAlignedHeapAllocator;
using DefaultStaticsHeapType = plAllocatorWithPolicy<plAllocPolicyHeap, plAllocatorTrackingMode::AllocationStatsIgnoreLeaks>;
#endif

enum
{
  HEAP_ALLOCATOR_BUFFER_SIZE = sizeof(DefaultHeapType),
  ALIGNED_ALLOCATOR_BUFFER_SIZE = sizeof(DefaultAlignedHeapType)
};

alignas(PL_ALIGNMENT_MINIMUM) static plUInt8 s_DefaultAllocatorBuffer[HEAP_ALLOCATOR_BUFFER_SIZE];
alignas(PL_ALIGNMENT_MINIMUM) static plUInt8 s_StaticAllocatorBuffer[HEAP_ALLOCATOR_BUFFER_SIZE];

alignas(PL_ALIGNMENT_MINIMUM) static plUInt8 s_AlignedAllocatorBuffer[ALIGNED_ALLOCATOR_BUFFER_SIZE];

bool plFoundation::s_bIsInitialized = false;
plAllocator* plFoundation::s_pDefaultAllocator = nullptr;
plAllocator* plFoundation::s_pAlignedAllocator = nullptr;

void plFoundation::Initialize()
{
  if (s_bIsInitialized)
    return;

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  plMemoryUtils::ReserveLower4GBAddressSpace();
#endif

  if (s_pDefaultAllocator == nullptr)
  {
    s_pDefaultAllocator = new (s_DefaultAllocatorBuffer) DefaultHeapType("DefaultHeap");
  }

  if (s_pAlignedAllocator == nullptr)
  {
    s_pAlignedAllocator = new (s_AlignedAllocatorBuffer) DefaultAlignedHeapType("AlignedHeap");
  }

  s_bIsInitialized = true;
}

#if defined(PL_CUSTOM_STATIC_ALLOCATOR_FUNC)
extern plAllocator* PL_CUSTOM_STATIC_ALLOCATOR_FUNC();
#endif

plAllocator* plFoundation::GetStaticsAllocator()
{
  static plAllocator* pStaticAllocator = nullptr;

  if (pStaticAllocator == nullptr)
  {
#if defined(PL_CUSTOM_STATIC_ALLOCATOR_FUNC)

#  if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)

#    if PL_ENABLED(PL_PLATFORM_WINDOWS)
    using GetStaticAllocatorFunc = plAllocator* (*)();

    HMODULE hThisModule = GetModuleHandle(nullptr);
    GetStaticAllocatorFunc func = (GetStaticAllocatorFunc)GetProcAddress(hThisModule, PL_CUSTOM_STATIC_ALLOCATOR_FUNC);
    if (func != nullptr)
    {
      pStaticAllocator = (*func)();
      return pStaticAllocator;
    }
#    else
#      error "Customizing static allocator not implemented"
#    endif

#  else
    return PL_CUSTOM_STATIC_ALLOCATOR_FUNC();
#  endif

#endif

    pStaticAllocator = new (s_StaticAllocatorBuffer) DefaultStaticsHeapType("Statics");
  }

  return pStaticAllocator;
}
