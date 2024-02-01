#pragma once

/*
You can classify bytes in a UTF-8 stream as follows:
  With the high bit set to 0, it's a single byte value.
  With the two high bits set to 10, it's a continuation byte (the second, third or fourth byte in a UTF-8 multi-byte sequence).
  Otherwise, it's the first byte of a multi-byte sequence and the number of leading 1 bits indicates how many bytes there are in total for
this sequence (110... means two bytes, 1110... means three bytes, etc).
*/

PL_ALWAYS_INLINE bool plUnicodeUtils::IsUtf8StartByte(char iByte)
{
  // valid utf8 start bytes are 0x0-------, 0x110-----, 0x1110----, 0x11110---, etc
  return ((iByte & 0x80) == 0) || ((iByte & 0xE0) == 0xC0) || ((iByte & 0xF0) == 0xE0) || ((iByte & 0xF8) == 0xF0) || ((iByte & 0xFC) == 0xF8);
}

PL_ALWAYS_INLINE bool plUnicodeUtils::IsUtf8ContinuationByte(char iByte)
{
  // check whether the two upper bits are set to '10'
  return (iByte & 0xC0) == 0x80;
}

PL_ALWAYS_INLINE bool plUnicodeUtils::IsASCII(plUInt32 uiChar)
{
  return (uiChar <= 127);
}

inline plUInt32 plUnicodeUtils::GetUtf8SequenceLength(char iFirstByte)
{
  const plUInt32 uiBit7 = iFirstByte & PL_BIT(7);
  const plUInt32 uiBit6 = iFirstByte & PL_BIT(6);
  const plUInt32 uiBit5 = iFirstByte & PL_BIT(5);
  const plUInt32 uiBit4 = iFirstByte & PL_BIT(4);

  if (uiBit7 == 0) // ASCII character '0xxxxxxx'
    return 1;

  PL_IGNORE_UNUSED(uiBit6);
  PL_ASSERT_DEV(uiBit6 != 0, "Invalid Leading UTF-8 Byte.");

  if (uiBit5 == 0) // '110xxxxx'
    return 2;
  if (uiBit4 == 0) // '1110xxxx'
    return 3;

  // '1111xxxx'
  return 4;
}

template <typename ByteIterator>
plUInt32 plUnicodeUtils::DecodeUtf8ToUtf32(ByteIterator& ref_szUtf8Iterator)
{
  return utf8::unchecked::next(ref_szUtf8Iterator);
}

template <typename UInt16Iterator>
bool plUnicodeUtils::IsUtf16Surrogate(UInt16Iterator& ref_szUtf16Iterator)
{
  uint32_t cp = utf8::internal::mask16(*ref_szUtf16Iterator);
  return utf8::internal::is_lead_surrogate(cp);
}

template <typename UInt16Iterator>
plUInt32 plUnicodeUtils::DecodeUtf16ToUtf32(UInt16Iterator& ref_szUtf16Iterator)
{
  uint32_t cp = utf8::internal::mask16(*ref_szUtf16Iterator++);
  if (utf8::internal::is_lead_surrogate(cp))
  {
    uint32_t trail_surrogate = utf8::internal::mask16(*ref_szUtf16Iterator++);
    cp = (cp << 10) + trail_surrogate + utf8::internal::SURROGATE_OFFSET;
  }

  return cp;
}

template <typename WCharIterator>
plUInt32 plUnicodeUtils::DecodeWCharToUtf32(WCharIterator& ref_szWCharIterator)
{
  if constexpr (sizeof(wchar_t) == 2)
  {
    return DecodeUtf16ToUtf32(ref_szWCharIterator);
  }
  else // sizeof(wchar_t) == 4
  {
    const plUInt32 uiResult = *ref_szWCharIterator;
    ++ref_szWCharIterator;
    return uiResult;
  }
}

template <typename ByteIterator>
void plUnicodeUtils::EncodeUtf32ToUtf8(plUInt32 uiUtf32, ByteIterator& ref_szUtf8Output)
{
  ref_szUtf8Output = utf8::unchecked::utf32to8(&uiUtf32, &uiUtf32 + 1, ref_szUtf8Output);
}

template <typename UInt16Iterator>
void plUnicodeUtils::EncodeUtf32ToUtf16(plUInt32 uiUtf32, UInt16Iterator& ref_szUtf16Output)
{
  if (uiUtf32 > 0xffff)
  {
    // make a surrogate pair
    *ref_szUtf16Output++ = static_cast<uint16_t>((uiUtf32 >> 10) + utf8::internal::LEAD_OFFSET);
    *ref_szUtf16Output++ = static_cast<uint16_t>((uiUtf32 & 0x3ff) + utf8::internal::TRAIL_SURROGATE_MIN);
  }
  else
    *ref_szUtf16Output++ = static_cast<uint16_t>(uiUtf32);
}

template <typename WCharIterator>
void plUnicodeUtils::EncodeUtf32ToWChar(plUInt32 uiUtf32, WCharIterator& ref_szWCharOutput)
{
  if constexpr (sizeof(wchar_t) == 2)
  {
    EncodeUtf32ToUtf16(uiUtf32, ref_szWCharOutput);
  }
  else
  {
    *ref_szWCharOutput = static_cast<wchar_t>(uiUtf32);
    ++ref_szWCharOutput;
  }
}

inline plUInt32 plUnicodeUtils::ConvertUtf8ToUtf32(const char* pFirstChar)
{
  return utf8::unchecked::peek_next(pFirstChar);
}

inline plUInt32 plUnicodeUtils::GetSizeForCharacterInUtf8(plUInt32 uiCharacter)
{
  // Basically implements this: http://en.wikipedia.org/wiki/Utf8#Description

  if (uiCharacter <= 0x0000007f)
    return 1;

  if (uiCharacter <= 0x000007ff)
    return 2;

  if (uiCharacter <= 0x0000ffff)
    return 3;

  // UTF-8 can use up to 6 bytes to encode a code point
  // however some committee agreed that never more than 4 bytes are used (no need for more than 21 Bits)
  // this implementation assumes in several places, that the UTF-8 encoding never uses more than 4 bytes

  PL_ASSERT_DEV(uiCharacter <= 0x0010ffff, "Invalid Unicode Codepoint");
  return 4;
}

inline bool plUnicodeUtils::IsValidUtf8(const char* szString, const char* szStringEnd)
{
  if (szStringEnd == GetMaxStringEnd<char>())
    szStringEnd = szString + strlen(szString);

  return utf8::is_valid(szString, szStringEnd);
}

inline bool plUnicodeUtils::SkipUtf8Bom(const char*& ref_szUtf8)
{
  PL_ASSERT_DEBUG(ref_szUtf8 != nullptr, "This function expects non nullptr pointers");

  if (utf8::starts_with_bom(ref_szUtf8, ref_szUtf8 + 4))
  {
    ref_szUtf8 += 3;
    return true;
  }

  return false;
}

inline bool plUnicodeUtils::SkipUtf16BomLE(const plUInt16*& ref_pUtf16)
{
  PL_ASSERT_DEBUG(ref_pUtf16 != nullptr, "This function expects non nullptr pointers");

  if (*ref_pUtf16 == plUnicodeUtils::Utf16BomLE)
  {
    ++ref_pUtf16;
    return true;
  }

  return false;
}

inline bool plUnicodeUtils::SkipUtf16BomBE(const plUInt16*& ref_pUtf16)
{
  PL_ASSERT_DEBUG(ref_pUtf16 != nullptr, "This function expects non nullptr pointers");

  if (*ref_pUtf16 == plUnicodeUtils::Utf16BomBE)
  {
    ++ref_pUtf16;
    return true;
  }

  return false;
}

inline plResult plUnicodeUtils::MoveToNextUtf8(const char*& ref_szUtf8, plUInt32 uiNumCharacters)
{
  PL_ASSERT_DEBUG(ref_szUtf8 != nullptr, "Invalid string pointer to advance!");

  while (uiNumCharacters > 0)
  {
    if (*ref_szUtf8 == '\0')
      return PL_FAILURE;

    do
    {
      ++ref_szUtf8;
    } while (IsUtf8ContinuationByte(*ref_szUtf8));

    --uiNumCharacters;
  }

  return PL_SUCCESS;
}

inline plResult plUnicodeUtils::MoveToNextUtf8(const char*& ref_szUtf8, const char* szUtf8End, plUInt32 uiNumCharacters)
{
  PL_ASSERT_DEBUG(ref_szUtf8 != nullptr, "Invalid string pointer to advance!");

  while (uiNumCharacters > 0)
  {
    if (ref_szUtf8 >= szUtf8End || *ref_szUtf8 == '\0')
      return PL_FAILURE;

    do
    {
      ++ref_szUtf8;
    } while ((ref_szUtf8 < szUtf8End) && IsUtf8ContinuationByte(*ref_szUtf8));

    --uiNumCharacters;
  }

  return PL_SUCCESS;
}

inline plResult plUnicodeUtils::MoveToPriorUtf8(const char*& ref_szUtf8, const char* szUtf8Start, plUInt32 uiNumCharacters)
{
  PL_ASSERT_DEBUG(ref_szUtf8 != nullptr, "Invalid string pointer to advance!");

  while (uiNumCharacters > 0)
  {
    if (ref_szUtf8 <= szUtf8Start)
      return PL_FAILURE;

    do
    {
      --ref_szUtf8;
    } while (IsUtf8ContinuationByte(*ref_szUtf8));

    --uiNumCharacters;
  }

  return PL_SUCCESS;
}
template <typename T>
constexpr T* plUnicodeUtils::GetMaxStringEnd()
{
  return reinterpret_cast<T*>(-1);
}
