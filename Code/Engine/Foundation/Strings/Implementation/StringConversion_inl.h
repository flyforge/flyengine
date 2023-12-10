#pragma once

#include <Foundation/ThirdParty/utf8/utf8.h>

#include <Foundation/Strings/UnicodeUtils.h>

// **************** plStringWChar ****************

inline plStringWChar::plStringWChar(plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline plStringWChar::plStringWChar(const plUInt16* pUtf16, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf16;
}

inline plStringWChar::plStringWChar(const plUInt32* pUtf32, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf32;
}

inline plStringWChar::plStringWChar(const wchar_t* pWChar, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = pWChar;
}

inline plStringWChar::plStringWChar(plStringView sUtf8, plAllocatorBase* pAllocator /*= plFoundation::GetDefaultAllocator()*/)
{
  *this = sUtf8;
}


// **************** plStringUtf8 ****************

inline plStringUtf8::plStringUtf8(plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline plStringUtf8::plStringUtf8(const char* szUtf8, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf8;
}

inline plStringUtf8::plStringUtf8(const plUInt16* pUtf16, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf16;
}

inline plStringUtf8::plStringUtf8(const plUInt32* pUtf32, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf32;
}

inline plStringUtf8::plStringUtf8(const wchar_t* pWChar, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = pWChar;
}

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)

inline plStringUtf8::plStringUtf8(
  const Microsoft::WRL::Wrappers::HString& hstring, plAllocatorBase* pAllocator /*= plFoundation::GetDefaultAllocator()*/)
  : m_Data(pAllocator)
{
  *this = hstring;
}

inline plStringUtf8::plStringUtf8(const HSTRING& hstring, plAllocatorBase* pAllocator /*= plFoundation::GetDefaultAllocator()*/)
{
  *this = hstring;
}

#endif

// **************** plStringUtf16 ****************

inline plStringUtf16::plStringUtf16(plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline plStringUtf16::plStringUtf16(const char* szUtf8, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf8;
}

inline plStringUtf16::plStringUtf16(const plUInt16* pUtf16, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf16;
}

inline plStringUtf16::plStringUtf16(const plUInt32* pUtf32, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf32;
}

inline plStringUtf16::plStringUtf16(const wchar_t* pWChar, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = pWChar;
}



// **************** plStringUtf32 ****************

inline plStringUtf32::plStringUtf32(plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline plStringUtf32::plStringUtf32(const char* szUtf8, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = szUtf8;
}

inline plStringUtf32::plStringUtf32(const plUInt16* pUtf16, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf16;
}

inline plStringUtf32::plStringUtf32(const plUInt32* pUtf32, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = pUtf32;
}

inline plStringUtf32::plStringUtf32(const wchar_t* pWChar, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = pWChar;
}
