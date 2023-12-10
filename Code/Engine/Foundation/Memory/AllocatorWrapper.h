#pragma once

struct plNullAllocatorWrapper
{
  PLASMA_FORCE_INLINE static plAllocatorBase* GetAllocator()
  {
    PLASMA_REPORT_FAILURE("This method should never be called");
    return nullptr;
  }
};

struct plDefaultAllocatorWrapper
{
  PLASMA_ALWAYS_INLINE static plAllocatorBase* GetAllocator() { return plFoundation::GetDefaultAllocator(); }
};

struct plStaticAllocatorWrapper
{
  PLASMA_ALWAYS_INLINE static plAllocatorBase* GetAllocator() { return plFoundation::GetStaticAllocator(); }
};

struct plAlignedAllocatorWrapper
{
  PLASMA_ALWAYS_INLINE static plAllocatorBase* GetAllocator() { return plFoundation::GetAlignedAllocator(); }
};

struct PLASMA_FOUNDATION_DLL plLocalAllocatorWrapper
{
  plLocalAllocatorWrapper(plAllocatorBase* pAllocator);

  void Reset();

  static plAllocatorBase* GetAllocator();
};
