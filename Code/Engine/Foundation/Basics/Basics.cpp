#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/CommonAllocators.h>

#if PLASMA_ENABLED(PLASMA_USE_GUARDED_ALLOCATIONS)
using DefaultHeapType = plGuardedAllocator;
using DefaultAlignedHeapType = plGuardedAllocator;
using DefaultStaticHeapType = plGuardedAllocator;
#else
using DefaultHeapType = plHeapAllocator;
using DefaultAlignedHeapType = plAlignedHeapAllocator;
using DefaultStaticHeapType = plHeapAllocator;
#endif

enum
{
  HEAP_ALLOCATOR_BUFFER_SIZE = sizeof(DefaultHeapType),
  ALIGNED_ALLOCATOR_BUFFER_SIZE = sizeof(DefaultAlignedHeapType)
};

alignas(PLASMA_ALIGNMENT_MINIMUM) static plUInt8 s_DefaultAllocatorBuffer[HEAP_ALLOCATOR_BUFFER_SIZE];
alignas(PLASMA_ALIGNMENT_MINIMUM) static plUInt8 s_StaticAllocatorBuffer[HEAP_ALLOCATOR_BUFFER_SIZE];

alignas(PLASMA_ALIGNMENT_MINIMUM) static plUInt8 s_AlignedAllocatorBuffer[ALIGNED_ALLOCATOR_BUFFER_SIZE];

bool plFoundation::s_bIsInitialized = false;
plAllocatorBase* plFoundation::s_pDefaultAllocator = nullptr;
plAllocatorBase* plFoundation::s_pAlignedAllocator = nullptr;

void plFoundation::Initialize()
{
  if (s_bIsInitialized)
    return;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
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

#if defined(PLASMA_CUSTOM_STATIC_ALLOCATOR_FUNC)
extern plAllocatorBase* PLASMA_CUSTOM_STATIC_ALLOCATOR_FUNC();
#endif

plAllocatorBase* plFoundation::GetStaticAllocator()
{
  static plAllocatorBase* pStaticAllocator = nullptr;

  if (pStaticAllocator == nullptr)
  {
#if defined(PLASMA_CUSTOM_STATIC_ALLOCATOR_FUNC)

#  if PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)

#    if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    using GetStaticAllocatorFunc = plAllocatorBase* (*)();

    HMODULE hThisModule = GetModuleHandle(nullptr);
    GetStaticAllocatorFunc func = (GetStaticAllocatorFunc)GetProcAddress(hThisModule, PLASMA_CUSTOM_STATIC_ALLOCATOR_FUNC);
    if (func != nullptr)
    {
      pStaticAllocator = (*func)();
      return pStaticAllocator;
    }
#    else
#      error "Customizing static allocator not implemented"
#    endif

#  else
    return PLASMA_CUSTOM_STATIC_ALLOCATOR_FUNC();
#  endif

#endif

    pStaticAllocator = new (s_StaticAllocatorBuffer) DefaultStaticHeapType(PLASMA_STATIC_ALLOCATOR_NAME);
  }

  return pStaticAllocator;
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_Basics_Basics);
