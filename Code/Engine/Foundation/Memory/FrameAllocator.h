#pragma once

#include <Foundation/Memory/StackAllocator.h>

/// \brief A double buffered stack allocator
class PLASMA_FOUNDATION_DLL plDoubleBufferedStackAllocator
{
public:
  using StackAllocatorType = plStackAllocator<plMemoryTrackingFlags::RegisterAllocator>;

  plDoubleBufferedStackAllocator(plStringView sName, plAllocatorBase* pParent);
  ~plDoubleBufferedStackAllocator();

  PLASMA_ALWAYS_INLINE plAllocatorBase* GetCurrentAllocator() const { return m_pCurrentAllocator; }

  void Swap();
  void Reset();

private:
  StackAllocatorType* m_pCurrentAllocator;
  StackAllocatorType* m_pOtherAllocator;
};

class PLASMA_FOUNDATION_DLL plFrameAllocator
{
public:
  PLASMA_ALWAYS_INLINE static plAllocatorBase* GetCurrentAllocator() { return s_pAllocator->GetCurrentAllocator(); }

  static void Swap();
  static void Reset();

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, FrameAllocator);

  static void Startup();
  static void Shutdown();

  static plDoubleBufferedStackAllocator* s_pAllocator;
};
