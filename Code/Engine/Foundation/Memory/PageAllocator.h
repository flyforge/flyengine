#pragma once

#include <Foundation/Basics.h>

/// \brief This helper class can reserve and allocate whole memory pages.
class PL_FOUNDATION_DLL plPageAllocator
{
public:
  static void* AllocatePage(size_t uiSize);
  static void DeallocatePage(void* pPtr);

  static plAllocatorId GetId();
};
