#pragma once

struct plNullAllocatorWrapper
{
  PL_FORCE_INLINE static plAllocator* GetAllocator()
  {
    PL_REPORT_FAILURE("This method should never be called");
    return nullptr;
  }
};

struct plDefaultAllocatorWrapper
{
  PL_ALWAYS_INLINE static plAllocator* GetAllocator() { return plFoundation::GetDefaultAllocator(); }
};

struct plStaticsAllocatorWrapper
{
  PL_ALWAYS_INLINE static plAllocator* GetAllocator() { return plFoundation::GetStaticsAllocator(); }
};

struct plAlignedAllocatorWrapper
{
  PL_ALWAYS_INLINE static plAllocator* GetAllocator() { return plFoundation::GetAlignedAllocator(); }
};

struct PL_FOUNDATION_DLL plLocalAllocatorWrapper
{
  plLocalAllocatorWrapper(plAllocator* pAllocator);

  void Reset();

  static plAllocator* GetAllocator();
};
