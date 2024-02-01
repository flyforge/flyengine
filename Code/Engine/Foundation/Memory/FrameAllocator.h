#pragma once

#include <Foundation/Memory/LinearAllocator.h>

/// \brief A double buffered stack allocator
class PL_FOUNDATION_DLL plDoubleBufferedLinearAllocator
{
public:
  using StackAllocatorType = plLinearAllocator<plAllocatorTrackingMode::Basics>;

  plDoubleBufferedLinearAllocator(plStringView sName, plAllocator* pParent);
  ~plDoubleBufferedLinearAllocator();

  PL_ALWAYS_INLINE plAllocator* GetCurrentAllocator() const { return m_pCurrentAllocator; }

  void Swap();
  void Reset();

private:
  StackAllocatorType* m_pCurrentAllocator;
  StackAllocatorType* m_pOtherAllocator;
};

class PL_FOUNDATION_DLL plFrameAllocator
{
public:
  PL_ALWAYS_INLINE static plAllocator* GetCurrentAllocator() { return s_pAllocator->GetCurrentAllocator(); }

  static void Swap();
  static void Reset();

private:
  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, FrameAllocator);

  static void Startup();
  static void Shutdown();

  static plDoubleBufferedLinearAllocator* s_pAllocator;
};
