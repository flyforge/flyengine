#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Basics/Platform/Win/MinWindows.h>
#  include <type_traits>

template <typename plType, typename WindowsType, bool mustBeConvertible>
void plVerifyWindowsType()
{
  static_assert(sizeof(plType) == sizeof(WindowsType), "pl <=> windows.h size mismatch");
  static_assert(alignof(plType) == alignof(WindowsType), "pl <=> windows.h alignment mismatch");
  static_assert(std::is_pointer<plType>::value == std::is_pointer<WindowsType>::value, "pl <=> windows.h pointer type mismatch");
  static_assert(!mustBeConvertible || plConversionTest<plType, WindowsType>::exists == 1, "pl <=> windows.h conversion failure");
  static_assert(!mustBeConvertible || plConversionTest<WindowsType, plType>::exists == 1, "windows.h <=> pl conversion failure");
};

void CALLBACK WindowsCallbackTest1();
void PL_WINDOWS_CALLBACK WindowsCallbackTest2();
void WINAPI WindowsWinapiTest1();
void PL_WINDOWS_WINAPI WindowsWinapiTest2();

// Will never be called and thus removed by the linker
void plCheckWindowsTypeSizes()
{
  plVerifyWindowsType<plMinWindows::DWORD, DWORD, true>();
  plVerifyWindowsType<plMinWindows::UINT, UINT, true>();
  plVerifyWindowsType<plMinWindows::BOOL, BOOL, true>();
  plVerifyWindowsType<plMinWindows::LPARAM, LPARAM, true>();
  plVerifyWindowsType<plMinWindows::WPARAM, WPARAM, true>();
  plVerifyWindowsType<plMinWindows::HINSTANCE, HINSTANCE, false>();
  plVerifyWindowsType<plMinWindows::HMODULE, HMODULE, false>();
  plVerifyWindowsType<plMinWindows::LPSTR, LPSTR, true>();
  plVerifyWindowsType<plMinWindows::HWND, HWND, false>();
  plVerifyWindowsType<plMinWindows::HRESULT, HRESULT, true>();

  static_assert(std::is_same<decltype(&WindowsCallbackTest1), decltype(&WindowsCallbackTest2)>::value, "PL_WINDOWS_CALLBACK does not match CALLBACK");
  static_assert(std::is_same<decltype(&WindowsWinapiTest1), decltype(&WindowsWinapiTest2)>::value, "PL_WINDOWS_WINAPI does not match WINAPI");

  // Clang doesn't allow us to do this check at compile time
#  if PL_DISABLED(PL_COMPILER_CLANG)
  static_assert(PL_WINDOWS_INVALID_HANDLE_VALUE == INVALID_HANDLE_VALUE, "PL_WINDOWS_INVALID_HANDLE_VALUE does not match INVALID_HANDLE_VALUE");
#  endif
}
#endif


