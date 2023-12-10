#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/Strings/String.h>

/// Build string implementation for HRESULT.
PLASMA_FOUNDATION_DLL plString plHRESULTtoString(plMinWindows::HRESULT result);

/// Conversion of HRESULT to plResult.
PLASMA_ALWAYS_INLINE plResult plToResult(plMinWindows::HRESULT result)
{
  return result >= 0 ? PLASMA_SUCCESS : PLASMA_FAILURE;
}

#define PLASMA_HRESULT_TO_FAILURE(code)   \
  do                                  \
  {                                   \
    plMinWindows::HRESULT s = (code); \
    if (s < 0)                        \
      return PLASMA_FAILURE;              \
  } while (false)

#define PLASMA_HRESULT_TO_FAILURE_LOG(code)                                                      \
  do                                                                                         \
  {                                                                                          \
    plMinWindows::HRESULT s = (code);                                                        \
    if (s < 0)                                                                               \
    {                                                                                        \
      plLog::Error("Call '{0}' failed with: {1}", PLASMA_STRINGIZE(code), plHRESULTtoString(s)); \
      return PLASMA_FAILURE;                                                                     \
    }                                                                                        \
  } while (false)

#define PLASMA_HRESULT_TO_LOG(code)                                                              \
  do                                                                                         \
  {                                                                                          \
    plMinWindows::HRESULT s = (code);                                                        \
    if (s < 0)                                                                               \
    {                                                                                        \
      plLog::Error("Call '{0}' failed with: {1}", PLASMA_STRINGIZE(code), plHRESULTtoString(s)); \
    }                                                                                        \
  } while (false)

#define PLASMA_NO_RETURNVALUE

#define PLASMA_HRESULT_TO_LOG_RET(code, ret)                                                     \
  do                                                                                         \
  {                                                                                          \
    plMinWindows::HRESULT s = (code);                                                        \
    if (s < 0)                                                                               \
    {                                                                                        \
      plLog::Error("Call '{0}' failed with: {1}", PLASMA_STRINGIZE(code), plHRESULTtoString(s)); \
      return ret;                                                                            \
    }                                                                                        \
  } while (false)

#define PLASMA_HRESULT_TO_ASSERT(code)                                                                  \
  do                                                                                                \
  {                                                                                                 \
    plMinWindows::HRESULT s = (code);                                                               \
    PLASMA_ASSERT_DEV(s >= 0, "Call '{0}' failed with: {1}", PLASMA_STRINGIZE(code), plHRESULTtoString(s)); \
  } while (false)
