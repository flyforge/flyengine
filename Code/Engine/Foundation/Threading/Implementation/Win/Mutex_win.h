#ifdef PL_MUTEX_WIN_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define PL_MUTEX_WIN_INL_H_INCLUDED

#if PL_ENABLED(PL_COMPILER_MSVC) && PL_ENABLED(PL_PLATFORM_ARCH_X86)

extern "C"
{
  // The main purpose of this little hack here is to have Mutex::Lock and Mutex::Unlock inline-able without including windows.h
  // The hack however does only work on the MSVC compiler. See fall back code below.

  // First define two functions which are binary compatible with EnterCriticalSection and LeaveCriticalSection
  __declspec(dllimport) void __stdcall plWinEnterCriticalSection(plMutexHandle* handle);
  __declspec(dllimport) void __stdcall plWinLeaveCriticalSection(plMutexHandle* handle);
  __declspec(dllimport) plMinWindows::BOOL __stdcall plWinTryEnterCriticalSection(plMutexHandle* handle);

  // Now redirect them through linker flags to the correct implementation
#  if PL_ENABLED(PL_PLATFORM_32BIT)
#    pragma comment(linker, "/alternatename:__imp__plWinEnterCriticalSection@4=__imp__EnterCriticalSection@4")
#    pragma comment(linker, "/alternatename:__imp__plWinLeaveCriticalSection@4=__imp__LeaveCriticalSection@4")
#    pragma comment(linker, "/alternatename:__imp__plWinTryEnterCriticalSection@4=__imp__TryEnterCriticalSection@4")
#  else
#    pragma comment(linker, "/alternatename:__imp_plWinEnterCriticalSection=__imp_EnterCriticalSection")
#    pragma comment(linker, "/alternatename:__imp_plWinLeaveCriticalSection=__imp_LeaveCriticalSection")
#    pragma comment(linker, "/alternatename:__imp_plWinTryEnterCriticalSection=__imp_TryEnterCriticalSection")
#  endif
}

inline void plMutex::Lock()
{
  plWinEnterCriticalSection(&m_hHandle);
  ++m_iLockCount;
}

inline void plMutex::Unlock()
{
  --m_iLockCount;
  plWinLeaveCriticalSection(&m_hHandle);
}

inline plResult plMutex::TryLock()
{
  if (plWinTryEnterCriticalSection(&m_hHandle) != 0)
  {
    ++m_iLockCount;
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

#else

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

inline void plMutex::Lock()
{
  EnterCriticalSection((CRITICAL_SECTION*)&m_hHandle);
  ++m_iLockCount;
}

inline void plMutex::Unlock()
{
  --m_iLockCount;
  LeaveCriticalSection((CRITICAL_SECTION*)&m_hHandle);
}

inline plResult plMutex::TryLock()
{
  if (TryEnterCriticalSection((CRITICAL_SECTION*)&m_hHandle) != 0)
  {
    ++m_iLockCount;
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

#endif
