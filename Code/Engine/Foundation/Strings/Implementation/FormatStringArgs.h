#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringView.h>

class plStringBuilder;
class plVariant;
class plAngle;
class plRational;
struct plTime;

struct plArgI
{
  inline explicit plArgI(plInt64 value, plUInt8 uiWidth = 1, bool bPadWithZeros = false, plUInt8 uiBase = 10)
    : m_Value(value)
    , m_uiWidth(uiWidth)
    , m_bPadWithZeros(bPadWithZeros)
    , m_uiBase(uiBase)
  {
  }

  plInt64 m_Value;
  plUInt8 m_uiWidth;
  bool m_bPadWithZeros;
  plUInt8 m_uiBase;
};

struct plArgU
{
  inline explicit plArgU(plUInt64 value, plUInt8 uiWidth = 1, bool bPadWithZeros = false, plUInt8 uiBase = 10, bool bUpperCase = false)
    : m_Value(value)
    , m_uiWidth(uiWidth)
    , m_bPadWithZeros(bPadWithZeros)
    , m_bUpperCase(bUpperCase)
    , m_uiBase(uiBase)
  {
  }

  plUInt64 m_Value;
  plUInt8 m_uiWidth;
  bool m_bPadWithZeros;
  bool m_bUpperCase;
  plUInt8 m_uiBase;
};

struct plArgF
{
  inline explicit plArgF(double value, plInt8 iPrecision = -1, bool bScientific = false, plUInt8 uiWidth = 1, bool bPadWithZeros = false)
    : m_Value(value)
    , m_uiWidth(uiWidth)
    , m_bPadWithZeros(bPadWithZeros)
    , m_bScientific(bScientific)
    , m_iPrecision(iPrecision)
  {
  }

  double m_Value;
  plUInt8 m_uiWidth;
  bool m_bPadWithZeros;
  bool m_bScientific;
  plInt8 m_iPrecision;
};

struct plArgC
{
  inline explicit plArgC(char value)
    : m_Value(value)
  {
  }

  char m_Value;
};

struct plArgP
{
  inline explicit plArgP(const void* value)
    : m_Value(value)
  {
  }

  const void* m_Value;
};


/// \brief Formats a given number such that it will be in format [0, base){suffix} with suffix
/// representing a power of base. Resulting numbers are output with a precision of 2 fractional digits
/// and fractional digits are subject to rounding, so numbers at the upper boundary of [0, base)
/// may be rounded up to the next power of base.
///
/// E.g.: For the default case base is 1000 and suffixes are the SI unit suffixes (i.e. K for kilo, M for mega etc.)
///       Thus 0 remains 0, 1 remains 1, 1000 becomes 1.00K, and 2534000 becomes 2.53M. But 999.999 will
///       end up being displayed as 1000.00K for base 1000 due to rounding.
struct plArgHumanReadable
{
  inline plArgHumanReadable(const double value, const plUInt64 uiBase, const char* const* const pSuffixes, plUInt32 uiSuffixCount)
    : m_Value(value)
    , m_Base(uiBase)
    , m_Suffixes(pSuffixes)
    , m_SuffixCount(uiSuffixCount)
  {
  }

  inline plArgHumanReadable(const plInt64 value, const plUInt64 uiBase, const char* const* const pSuffixes, plUInt32 uiSuffixCount)
    : plArgHumanReadable(static_cast<double>(value), uiBase, pSuffixes, uiSuffixCount)
  {
  }

  inline explicit plArgHumanReadable(const double value)
    : plArgHumanReadable(value, 1000u, m_DefaultSuffixes, PLASMA_ARRAY_SIZE(m_DefaultSuffixes))
  {
  }

  inline explicit plArgHumanReadable(const plInt64 value)
    : plArgHumanReadable(static_cast<double>(value), 1000u, m_DefaultSuffixes, PLASMA_ARRAY_SIZE(m_DefaultSuffixes))
  {
  }

  const double m_Value;
  const plUInt64 m_Base;
  const char* const* const m_Suffixes;
  const char* const m_DefaultSuffixes[6] = {"", "K", "M", "G", "T", "P"};
  const plUInt32 m_SuffixCount;
};

struct plArgFileSize : public plArgHumanReadable
{
  inline explicit plArgFileSize(const plUInt64 value)
    : plArgHumanReadable(static_cast<double>(value), 1024u, m_ByteSuffixes, PLASMA_ARRAY_SIZE(m_ByteSuffixes))
  {
  }

  const char* const m_ByteSuffixes[6] = {"B", "KB", "MB", "GB", "TB", "PB"};
};

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
struct plArgErrorCode
{
  inline explicit plArgErrorCode(plUInt32 uiErrorCode)
    : m_ErrorCode(uiErrorCode)
  {
  }

  plUInt32 m_ErrorCode;
};
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgErrorCode& arg);

#endif

/// \brief Wraps a string that may contain sensitive information, such as user file paths.
///
/// The application can specify a function to scramble this type of information. By default no such function is set.
/// A general purpose function is provided with 'BuildString_SensitiveUserData_Hash()'
///
/// \param sSensitiveInfo The information that may need to be scrambled.
/// \param szContext A custom string to identify the 'context', ie. what type of sensitive data is being scrambled.
///        This may be passed through unmodified, or can guide the scrambling function to choose how to output the sensitive data.
struct plArgSensitive
{
  inline explicit plArgSensitive(const plStringView& sSensitiveInfo, const char* szContext = nullptr)
    : m_sSensitiveInfo(sSensitiveInfo)
    , m_szContext(szContext)
  {
  }

  const plStringView m_sSensitiveInfo;
  const char* m_szContext;

  using BuildStringCallback = plStringView (*)(char*, plUInt32, const plArgSensitive&);
  PLASMA_FOUNDATION_DLL static BuildStringCallback s_BuildStringCB;

  /// \brief Set s_BuildStringCB to this function to enable scrambling of sensitive data.
  PLASMA_FOUNDATION_DLL static plStringView BuildString_SensitiveUserData_Hash(char* szTmp, plUInt32 uiLength, const plArgSensitive& arg);
};

PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgI& arg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, plInt64 iArg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, plInt32 iArg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgU& arg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, plUInt64 uiArg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, plUInt32 uiArg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgF& arg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, double fArg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, bool bArg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const char* szArg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const wchar_t* pArg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plStringBuilder& sArg);
PLASMA_FOUNDATION_DLL const plStringView& BuildString(char* szTmp, plUInt32 uiLength, const plStringView& sArg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgC& arg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgP& arg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, plResult arg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plVariant& arg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plAngle& arg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plRational& arg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgHumanReadable& arg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plTime& arg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plArgSensitive& arg);


#if PLASMA_ENABLED(PLASMA_COMPILER_GCC) || PLASMA_ENABLED(PLASMA_COMPILER_CLANG)

// on these platforms "long int" is a different type from "long long int"

PLASMA_ALWAYS_INLINE plStringView BuildString(char* szTmp, plUInt32 uiLength, long int iArg)
{
  return BuildString(szTmp, uiLength, static_cast<plInt64>(iArg));
}

PLASMA_ALWAYS_INLINE plStringView BuildString(char* szTmp, plUInt32 uiLength, unsigned long int uiArg)
{
  return BuildString(szTmp, uiLength, static_cast<plUInt64>(uiArg));
}

#endif
