#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Rational.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Variant.h>

plFormatString::plFormatString(const plStringBuilder& s)
{
  m_sString = s.GetView();
}

const char* plFormatString::GetTextCStr(plStringBuilder& out_sString) const
{
  out_sString = m_sString;
  return out_sString.GetData();
}

plStringView plFormatString::BuildFormattedText(plStringBuilder& ref_sStorage, plStringView* pArgs, plUInt32 uiNumArgs) const
{
  plStringView sString = m_sString;

  plUInt32 uiLastParam = -1;

  ref_sStorage.Clear();
  while (!sString.IsEmpty())
  {
    if (sString.StartsWith("%"))
    {
      if (sString.TrimWordStart("%%"))
      {
        ref_sStorage.Append("%"_plsv);
      }
      else
      {
        PLASMA_ASSERT_DEBUG(false, "Single percentage signs are not allowed in plFormatString. Did you forgot to migrate a printf-style "
                               "string? Use double percentage signs for the actual character.");
      }
    }
    else if (sString.GetElementCount() >= 3 && *sString.GetStartPointer() == '{' && *(sString.GetStartPointer() + 1) >= '0' && *(sString.GetStartPointer() + 1) <= '9' && *(sString.GetStartPointer() + 2) == '}')
    {
      uiLastParam = *(sString.GetStartPointer() + 1) - '0';
      PLASMA_ASSERT_DEV(uiLastParam < uiNumArgs, "Too many placeholders in format string");

      if (uiLastParam < uiNumArgs)
      {
        ref_sStorage.Append(pArgs[uiLastParam]);
      }

      sString.ChopAwayFirstCharacterAscii();
      sString.ChopAwayFirstCharacterAscii();
      sString.ChopAwayFirstCharacterAscii();
    }
    else if (sString.TrimWordStart("{}"))
    {
      ++uiLastParam;
      PLASMA_ASSERT_DEV(uiLastParam < uiNumArgs, "Too many placeholders in format string");

      if (uiLastParam < uiNumArgs)
      {
        ref_sStorage.Append(pArgs[uiLastParam]);
      }
    }
    else
    {
      const plUInt32 character = sString.GetCharacter();
      ref_sStorage.Append(character);
      sString.ChopAwayFirstCharacterUtf8();
    }
  }

  return ref_sStorage.GetView();
}

//////////////////////////////////////////////////////////////////////////

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgI& arg)
{
  plUInt32 writepos = 0;
  plStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase);
  szTmp[writepos] = '\0';
  return plStringView(szTmp, szTmp + writepos);
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, plInt64 iArg)
{
  plUInt32 writepos = 0;
  plStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, iArg, 1, false, 10);
  szTmp[writepos] = '\0';
  return plStringView(szTmp, szTmp + writepos);
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, plInt32 iArg)
{
  return BuildString(szTmp, uiLength, (plInt64)iArg);
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgU& arg)
{
  plUInt32 writepos = 0;
  plStringUtils::OutputFormattedUInt(szTmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_uiBase, arg.m_bUpperCase);
  szTmp[writepos] = '\0';
  return plStringView(szTmp, szTmp + writepos);
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, plUInt64 uiArg)
{
  plUInt32 writepos = 0;
  plStringUtils::OutputFormattedUInt(szTmp, uiLength, writepos, uiArg, 1, false, 10, false);
  szTmp[writepos] = '\0';
  return plStringView(szTmp, szTmp + writepos);
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, plUInt32 uiArg)
{
  return BuildString(szTmp, uiLength, (plUInt64)uiArg);
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgF& arg)
{
  plUInt32 writepos = 0;
  plStringUtils::OutputFormattedFloat(szTmp, uiLength, writepos, arg.m_Value, arg.m_uiWidth, arg.m_bPadWithZeros, arg.m_iPrecision, arg.m_bScientific);
  szTmp[writepos] = '\0';
  return plStringView(szTmp, szTmp + writepos);
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, double fArg)
{
  plUInt32 writepos = 0;
  plStringUtils::OutputFormattedFloat(szTmp, uiLength, writepos, fArg, 1, false, -1, false);
  szTmp[writepos] = '\0';
  return plStringView(szTmp, szTmp + writepos);
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, bool bArg)
{
  if (bArg)
    return "true";

  return "false";
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const char* szArg)
{
  return szArg;
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const wchar_t* pArg)
{
  const char* start = szTmp;
  if (pArg != nullptr)
  {
    // Code points in UTF-8 can be up to 4 byte, so the end pointer is 3 byte "earlier" than for
    // for a single byte character. One byte for trailing zero is already accounted for in uiLength.
    const char* tmpEnd = szTmp + uiLength - 3u;
    while (*pArg != '\0' && szTmp < tmpEnd)
    {
      // decode utf8 to utf32
      const plUInt32 uiUtf32 = plUnicodeUtils::DecodeWCharToUtf32(pArg);

      // encode utf32 to wchar_t
      plUnicodeUtils::EncodeUtf32ToUtf8(uiUtf32, szTmp);
    }
  }

  // Append terminator. As the extra byte for trailing zero is accounted for in uiLength, this is safe.
  *szTmp = '\0';

  return start;
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plString& sArg)
{
  return plStringView(sArg.GetData(), sArg.GetData() + sArg.GetElementCount());
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plHashedString& sArg)
{
  return plStringView(sArg.GetData(), sArg.GetData() + sArg.GetString().GetElementCount());
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plStringBuilder& sArg)
{
  return plStringView(sArg.GetData(), sArg.GetData() + sArg.GetElementCount());
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plUntrackedString& sArg)
{
  return plStringView(sArg.GetData(), sArg.GetData() + sArg.GetElementCount());
}

const plStringView& BuildString(char* szTmp, plUInt32 uiLength, const plStringView& sArg)
{
  return sArg;
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgC& arg)
{
  szTmp[0] = arg.m_Value;
  szTmp[1] = '\0';

  return plStringView(&szTmp[0], &szTmp[1]);
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgP& arg)
{
  plStringUtils::snprintf(szTmp, uiLength, "%p", arg.m_Value);
  return plStringView(szTmp);
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, plResult arg)
{
  if (arg.Failed())
    return "<failed>";
  else
    return "<succeeded>";
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plVariant& arg)
{
  plString sString = arg.ConvertTo<plString>();
  plStringUtils::snprintf(szTmp, uiLength, "%s", sString.GetData());
  return plStringView(szTmp);
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plAngle& arg)
{
  plUInt32 writepos = 0;
  plStringUtils::OutputFormattedFloat(szTmp, uiLength - 2, writepos, arg.GetDegree(), 1, false, 1, false);

  // Utf-8 representation of the degree sign
  szTmp[writepos + 0] = (char)0xC2;
  szTmp[writepos + 1] = (char)0xB0;
  szTmp[writepos + 2] = '\0';

  return plStringView(szTmp, szTmp + writepos + 2);
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plRational& arg)
{
  plUInt32 writepos = 0;

  if (arg.IsIntegral())
  {
    plStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, arg.GetIntegralResult(), 1, false, 10);

    return plStringView(szTmp, szTmp + writepos);
  }
  else
  {
    plStringUtils::snprintf(szTmp, uiLength, "%i/%i", arg.GetNumerator(), arg.GetDenominator());

    return plStringView(szTmp);
  }
}

plStringView BuildString(char* pTmp, plUInt32 uiLength, const plTime& arg)
{
  plUInt32 writepos = 0;

  const double fAbsSec = plMath::Abs(arg.GetSeconds());

  if (fAbsSec < 0.000001)
  {
    plStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, writepos, arg.GetNanoseconds(), 1, false, 1, false, true);
    // szTmp[writepos++] = ' ';
    pTmp[writepos++] = 'n';
    pTmp[writepos++] = 's';
  }
  else if (fAbsSec < 0.001)
  {
    plStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, writepos, arg.GetMicroseconds(), 1, false, 1, false, true);

    // szTmp[writepos++] = ' ';
    // Utf-8 representation of the microsecond (us) sign
    pTmp[writepos++] = (char)0xC2;
    pTmp[writepos++] = (char)0xB5;
    pTmp[writepos++] = 's';
  }
  else if (fAbsSec < 1.0)
  {
    plStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, writepos, arg.GetMilliseconds(), 1, false, 1, false, true);

    // tmp[writepos++] = ' ';
    pTmp[writepos++] = 'm';
    pTmp[writepos++] = 's';
  }
  else if (fAbsSec < 60.0)
  {
    plStringUtils::OutputFormattedFloat(pTmp, uiLength - 5, writepos, arg.GetSeconds(), 1, false, 1, false, true);

    // szTmp[writepos++] = ' ';
    pTmp[writepos++] = 's';
    pTmp[writepos++] = 'e';
    pTmp[writepos++] = 'c';
  }
  else if (fAbsSec < 60.0 * 60.0)
  {
    double tRem = fAbsSec;

    plInt32 iMin = static_cast<plInt32>(plMath::Trunc(tRem / 60.0));
    tRem -= iMin * 60;
    iMin *= plMath::Sign(static_cast<plInt32>(arg.GetSeconds()));

    const plInt32 iSec = static_cast<plInt32>(plMath::Trunc(tRem));

    writepos = plStringUtils::snprintf(pTmp, uiLength, "%imin %isec", iMin, iSec);
  }
  else
  {
    double tRem = fAbsSec;

    plInt32 iHrs = static_cast<plInt32>(plMath::Trunc(tRem / (60.0 * 60.0)));
    tRem -= iHrs * 60 * 60;
    iHrs *= plMath::Sign(static_cast<plInt32>(arg.GetSeconds()));

    const plInt32 iMin = static_cast<plInt32>(plMath::Trunc(tRem / 60.0));
    tRem -= iMin * 60;

    const plInt32 iSec = static_cast<plInt32>(plMath::Trunc(tRem));

    writepos = plStringUtils::snprintf(pTmp, uiLength, "%ih %imin %isec", iHrs, iMin, iSec);
  }

  pTmp[writepos] = '\0';
  return plStringView(pTmp, pTmp + writepos);
}

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgHumanReadable& arg)
{
  plUInt32 suffixIndex = 0;
  plUInt64 divider = 1;
  double absValue = plMath::Abs(arg.m_Value);
  while (absValue / divider >= arg.m_Base && suffixIndex < arg.m_SuffixCount - 1)
  {
    divider *= arg.m_Base;
    ++suffixIndex;
  }

  plUInt32 writepos = 0;
  if (divider == 1 && plMath::Fraction(arg.m_Value) == 0.0)
  {
    plStringUtils::OutputFormattedInt(szTmp, uiLength, writepos, static_cast<plInt64>(arg.m_Value), 1, false, 10);
  }
  else
  {
    plStringUtils::OutputFormattedFloat(szTmp, uiLength, writepos, arg.m_Value / divider, 1, false, 2, false);
  }
  plStringUtils::Copy(szTmp + writepos, uiLength - writepos, arg.m_Suffixes[suffixIndex]);

  return plStringView(szTmp);
}

plArgSensitive::BuildStringCallback plArgSensitive::s_BuildStringCB = nullptr;

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgSensitive& arg)
{
  if (plArgSensitive::s_BuildStringCB)
  {
    return plArgSensitive::s_BuildStringCB(szTmp, uiLength, arg);
  }

  return arg.m_sSensitiveInfo;
}

plStringView plArgSensitive::BuildString_SensitiveUserData_Hash(char* szTmp, plUInt32 uiLength, const plArgSensitive& arg)
{
  const plUInt32 len = arg.m_sSensitiveInfo.GetElementCount();

  if (len == 0)
    return plStringView();

  if (!plStringUtils::IsNullOrEmpty(arg.m_szContext))
  {
    plStringUtils::snprintf(
      szTmp, uiLength, "sud:%s#%08x($%u)", arg.m_szContext, plHashingUtils::xxHash32(arg.m_sSensitiveInfo.GetStartPointer(), len), len);
  }
  else
  {
    plStringUtils::snprintf(szTmp, uiLength, "sud:#%08x($%u)", plHashingUtils::xxHash32(arg.m_sSensitiveInfo.GetStartPointer(), len), len);
  }

  return szTmp;
}

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgErrorCode& arg)
{
  LPVOID lpMsgBuf = nullptr;
  if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, arg.m_ErrorCode,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&lpMsgBuf, 0, nullptr) == 0)
  {
    DWORD err = GetLastError();
    plStringUtils::snprintf(szTmp, uiLength, "%i (FormatMessageW failed with error code %i)", arg.m_ErrorCode, err);
    return plStringView(szTmp);
  }

  LPWSTR pCRLF = wcschr((LPWSTR)lpMsgBuf, L'\r');
  if (pCRLF != nullptr)
  {
    // remove the \r\n that FormatMessageW always appends
    *pCRLF = L'\0';
  }

  // we need a bigger boat
  static thread_local char FullMessage[256];

  plStringUtils::snprintf(FullMessage, PLASMA_ARRAY_SIZE(FullMessage), "%i (\"%s\")", arg.m_ErrorCode, plStringUtf8((LPWSTR)lpMsgBuf).GetData());
  LocalFree(lpMsgBuf);
  return plStringView(FullMessage);
}
#endif

PLASMA_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_FormatString);
