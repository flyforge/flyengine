#pragma once

// Deactivate Doxygen document generation for the following block.
/// \cond

#include <Foundation/Basics/Platform/Win/MinWindows.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
struct alignas(4) plMutexHandle
{
  plUInt8 data[24];
};
#else
struct alignas(8) plMutexHandle
{
  plUInt8 data[40];
};
#endif


#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
struct alignas(4) plConditionVariableHandle
{
  plUInt8 data[4];
};
#else
struct alignas(8) plConditionVariableHandle
{
  plUInt8 data[8];
};
#endif



using plThreadHandle = plMinWindows::HANDLE;
using plThreadID = plMinWindows::DWORD;
using plOSThreadEntryPoint = plMinWindows::DWORD(__stdcall*)(void* lpThreadParameter);
using plSemaphoreHandle = plMinWindows::HANDLE;

#define PLASMA_THREAD_CLASS_ENTRY_POINT plMinWindows::DWORD __stdcall plThreadClassEntryPoint(void* lpThreadParameter);

struct plConditionVariableData
{
  plConditionVariableHandle m_ConditionVariable;
};

/// \endcond
