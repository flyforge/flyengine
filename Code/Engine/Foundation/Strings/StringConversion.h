#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringView.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
// Include our windows.h header first to get rid of defines.
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
// For HString, HStringReference and co.
#  include <wrl/wrappers/corewrappers.h>
#endif

/// \brief A very simple string class that should only be used to temporarily convert text to the OSes native wchar_t convention (16 or 32
/// Bit).
///
/// This should be used when one needs to output text via some function that only accepts wchar_t strings.
/// DO NOT use this for storage or anything else that is not temporary.
/// wchar_t is 16 Bit on Windows and 32 Bit on most other platforms. This class will always automatically convert to the correct format.
class PL_FOUNDATION_DLL plStringWChar
{
public:
  plStringWChar(plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringWChar(const plUInt16* pUtf16, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringWChar(const plUInt32* pUtf32, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringWChar(const wchar_t* pUtf32, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringWChar(plStringView sUtf8, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());

  void operator=(const plUInt16* pUtf16);
  void operator=(const plUInt32* pUtf32);
  void operator=(const wchar_t* pUtf32);
  void operator=(plStringView sUtf8);

  PL_ALWAYS_INLINE operator const wchar_t*() const { return &m_Data[0]; }
  PL_ALWAYS_INLINE const wchar_t* GetData() const { return &m_Data[0]; }
  PL_ALWAYS_INLINE plUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr plUInt32 BufferSize = 1024;
  plHybridArray<wchar_t, BufferSize> m_Data;
};


/// \brief A small string class that converts any other encoding to Utf8.
///
/// Use this class only temporarily. Do not use it for storage.
class PL_FOUNDATION_DLL plStringUtf8
{
public:
  plStringUtf8(plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringUtf8(const char* szUtf8, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringUtf8(const plUInt16* pUtf16, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringUtf8(const plUInt32* pUtf32, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringUtf8(const wchar_t* pWChar, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());

#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
  plStringUtf8(const Microsoft::WRL::Wrappers::HString& hstring, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringUtf8(const HSTRING& hstring, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
#endif

  void operator=(const char* szUtf8);
  void operator=(const plUInt16* pUtf16);
  void operator=(const plUInt32* pUtf32);
  void operator=(const wchar_t* pWChar);

#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
  void operator=(const Microsoft::WRL::Wrappers::HString& hstring);
  void operator=(const HSTRING& hstring);
#endif

  PL_ALWAYS_INLINE operator const char*() const
  {
    return &m_Data[0];
  }
  PL_ALWAYS_INLINE const char* GetData() const
  {
    return &m_Data[0];
  }
  PL_ALWAYS_INLINE plUInt32 GetElementCount() const
  {
    return m_Data.GetCount() - 1; /* exclude the '\0' terminator */
  }
  PL_ALWAYS_INLINE operator plStringView() const
  {
    return GetView();
  }
  PL_ALWAYS_INLINE plStringView GetView() const
  {
    return plStringView(&m_Data[0], GetElementCount());
  }

private:
  static constexpr plUInt32 BufferSize = 1024;
  plHybridArray<char, BufferSize> m_Data;
};



/// \brief A very simple class to convert text to Utf16 encoding.
///
/// Use this class only temporarily, if you need to output something in Utf16 format, e.g. for writing it to a file.
/// Never use this for storage.
/// When working with OS functions that expect '16 Bit strings', use plStringWChar instead.
class PL_FOUNDATION_DLL plStringUtf16
{
public:
  plStringUtf16(plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringUtf16(const char* szUtf8, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringUtf16(const plUInt16* pUtf16, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringUtf16(const plUInt32* pUtf32, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringUtf16(const wchar_t* pUtf32, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const plUInt16* pUtf16);
  void operator=(const plUInt32* pUtf32);
  void operator=(const wchar_t* pUtf32);

  PL_ALWAYS_INLINE const plUInt16* GetData() const { return &m_Data[0]; }
  PL_ALWAYS_INLINE plUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr plUInt32 BufferSize = 1024;
  plHybridArray<plUInt16, BufferSize> m_Data;
};



/// \brief This class only exists for completeness.
///
/// There should be no case where it is preferred over other classes.
class PL_FOUNDATION_DLL plStringUtf32
{
public:
  plStringUtf32(plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringUtf32(const char* szUtf8, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringUtf32(const plUInt16* pUtf16, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringUtf32(const plUInt32* pUtf32, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());
  plStringUtf32(const wchar_t* pWChar, plAllocator* pAllocator = plFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const plUInt16* pUtf16);
  void operator=(const plUInt32* pUtf32);
  void operator=(const wchar_t* pWChar);

  PL_ALWAYS_INLINE const plUInt32* GetData() const { return &m_Data[0]; }
  PL_ALWAYS_INLINE plUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr plUInt32 BufferSize = 1024;
  plHybridArray<plUInt32, BufferSize> m_Data;
};


#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)

/// \brief A very simple string class that should only be used to temporarily convert text to the OSes native HString (on UWP platforms).
///
/// This should be used when one needs to output text via some function that only accepts HString strings.
/// DO NOT use this for storage or anything else that is not temporary.
class PL_FOUNDATION_DLL plStringHString
{
public:
  plStringHString();
  plStringHString(const char* szUtf8);
  plStringHString(const plUInt16* szUtf16);
  plStringHString(const plUInt32* szUtf32);
  plStringHString(const wchar_t* szWChar);

  void operator=(const char* szUtf8);
  void operator=(const plUInt16* szUtf16);
  void operator=(const plUInt32* szUtf32);
  void operator=(const wchar_t* szWChar);

  /// \brief Unfortunately you cannot assign HStrings, so you cannot copy the result to another HString, you have to use this result
  /// directly
  PL_ALWAYS_INLINE const Microsoft::WRL::Wrappers::HString& GetData() const { return m_Data; }

private:
  Microsoft::WRL::Wrappers::HString m_Data;
};

#endif


#include <Foundation/Strings/Implementation/StringConversion_inl.h>
