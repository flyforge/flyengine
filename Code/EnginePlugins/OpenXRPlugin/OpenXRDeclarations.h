#pragma once

#define XR_LOG_ERROR(code)                                                                                                  \
  do                                                                                                                        \
  {                                                                                                                         \
    auto s = (code);                                                                                                        \
    if (s != XR_SUCCESS)                                                                                                    \
    {                                                                                                                       \
      plLog::Error("OpenXR call '{0}' failed with: {1} in {2}:{3}", PLASMA_STRINGIZE(code), s, PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE); \
    }                                                                                                                       \
  } while (false)

#define XR_SUCCEED_OR_RETURN_LOG(code)                                                                                      \
  do                                                                                                                        \
  {                                                                                                                         \
    auto s = (code);                                                                                                        \
    if (s != XR_SUCCESS)                                                                                                    \
    {                                                                                                                       \
      plLog::Error("OpenXR call '{0}' failed with: {1} in {2}:{3}", PLASMA_STRINGIZE(code), s, PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE); \
      return s;                                                                                                             \
    }                                                                                                                       \
  } while (false)

#define XR_SUCCEED_OR_CLEANUP_LOG(code, cleanup)                                                                            \
  do                                                                                                                        \
  {                                                                                                                         \
    auto s = (code);                                                                                                        \
    if (s != XR_SUCCESS)                                                                                                    \
    {                                                                                                                       \
      plLog::Error("OpenXR call '{0}' failed with: {1} in {2}:{3}", PLASMA_STRINGIZE(code), s, PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE); \
      cleanup();                                                                                                            \
      return s;                                                                                                             \
    }                                                                                                                       \
  } while (false)

static void voidFunction() {}