#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(const plStringBuilder& rhs, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(plStringBuilder&& rhs, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = std::move(rhs);
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(const plStringBuilder& rhs)
  : plHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(plStringBuilder&& rhs)
  : plHybridStringBase<Size>(std::move(rhs), A::GetAllocator())
{
}

template <plUInt16 Size>
void plHybridStringBase<Size>::operator=(const plStringBuilder& rhs)
{
  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = rhs.m_Data;
}

template <plUInt16 Size>
void plHybridStringBase<Size>::operator=(plStringBuilder&& rhs)
{
  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = std::move(rhs.m_Data);
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE void plHybridString<Size, A>::operator=(const plStringBuilder& rhs)
{
  plHybridStringBase<Size>::operator=(rhs);
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE void plHybridString<Size, A>::operator=(plStringBuilder&& rhs)
{
  plHybridStringBase<Size>::operator=(std::move(rhs));
}

template <plUInt16 Size>
void plHybridStringBase<Size>::ReadAll(plStreamReader& inout_stream)
{
  Clear();

  plHybridArray<plUInt8, 1024 * 4> Bytes(m_Data.GetAllocator());
  plUInt8 Temp[1024];

  while (true)
  {
    const plUInt32 uiRead = (plUInt32)inout_stream.ReadBytes(Temp, 1024);

    if (uiRead == 0)
      break;

    Bytes.PushBackRange(plArrayPtr<plUInt8>(Temp, uiRead));
  }

  Bytes.PushBack('\0');

  *this = (const char*)&Bytes[0];
}
