#pragma once

namespace plMinWindows
{
  using BOOL = int;
  using DWORD = unsigned long;
  using UINT = unsigned int;
  using LPSTR = char*;
  struct plHINSTANCE;
  using HINSTANCE = plHINSTANCE*;
  using HMODULE = HINSTANCE;
  struct plHWND;
  using HWND = plHWND*;
  using HRESULT = long;
  using HANDLE = void*;

#if PLASMA_ENABLED(PLASMA_PLATFORM_64BIT)
  using WPARAM = plUInt64;
  using LPARAM = plUInt64;
#else
  using WPARAM = plUInt32;
  using LPARAM = plUInt32;
#endif

  template <typename T>
  struct ToNativeImpl
  {
  };

  template <typename T>
  struct FromNativeImpl
  {
  };

  /// Helper function to convert plMinWindows types into native windows.h types.
  /// Include IncludeWindows.h before using it.
  template <typename T>
  PLASMA_ALWAYS_INLINE typename ToNativeImpl<T>::type ToNative(T t)
  {
    return ToNativeImpl<T>::ToNative(t);
  }

  /// Helper function to native windows.h types to plMinWindows types.
  /// Include IncludeWindows.h before using it.
  template <typename T>
  PLASMA_ALWAYS_INLINE typename FromNativeImpl<T>::type FromNative(T t)
  {
    return FromNativeImpl<T>::FromNative(t);
  }
} // namespace plMinWindows
#define PLASMA_WINDOWS_CALLBACK __stdcall
#define PLASMA_WINDOWS_WINAPI __stdcall
#define PLASMA_WINDOWS_INVALID_HANDLE_VALUE ((void*)(long long)-1)
