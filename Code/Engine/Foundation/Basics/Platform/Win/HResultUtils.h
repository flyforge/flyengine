#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/Strings/String.h>

/// Build string implementation for HRESULT.
PL_FOUNDATION_DLL plString plHRESULTtoString(plMinWindows::HRESULT result);

/// Conversion of HRESULT to plResult.
PL_ALWAYS_INLINE plResult plToResult(plMinWindows::HRESULT result)
{
  return result >= 0 ? PL_SUCCESS : PL_FAILURE;
}

#define PL_HRESULT_TO_FAILURE(code)   \
  do                                  \
  {                                   \
    plMinWindows::HRESULT s = (code); \
    if (s < 0)                        \
      return PL_FAILURE;              \
  } while (false)

#define PL_HRESULT_TO_FAILURE_LOG(code)                                                      \
  do                                                                                         \
  {                                                                                          \
    plMinWindows::HRESULT s = (code);                                                        \
    if (s < 0)                                                                               \
    {                                                                                        \
      plLog::Error("Call '{0}' failed with: {1}", PL_STRINGIZE(code), plHRESULTtoString(s)); \
      return PL_FAILURE;                                                                     \
    }                                                                                        \
  } while (false)

#define PL_HRESULT_TO_LOG(code)                                                              \
  do                                                                                         \
  {                                                                                          \
    plMinWindows::HRESULT s = (code);                                                        \
    if (s < 0)                                                                               \
    {                                                                                        \
      plLog::Error("Call '{0}' failed with: {1}", PL_STRINGIZE(code), plHRESULTtoString(s)); \
    }                                                                                        \
  } while (false)

#define PL_NO_RETURNVALUE

#define PL_HRESULT_TO_LOG_RET(code, ret)                                                     \
  do                                                                                         \
  {                                                                                          \
    plMinWindows::HRESULT s = (code);                                                        \
    if (s < 0)                                                                               \
    {                                                                                        \
      plLog::Error("Call '{0}' failed with: {1}", PL_STRINGIZE(code), plHRESULTtoString(s)); \
      return ret;                                                                            \
    }                                                                                        \
  } while (false)

#define PL_HRESULT_TO_ASSERT(code)                                                                  \
  do                                                                                                \
  {                                                                                                 \
    plMinWindows::HRESULT s = (code);                                                               \
    PL_ASSERT_DEV(s >= 0, "Call '{0}' failed with: {1}", PL_STRINGIZE(code), plHRESULTtoString(s)); \
  } while (false)
