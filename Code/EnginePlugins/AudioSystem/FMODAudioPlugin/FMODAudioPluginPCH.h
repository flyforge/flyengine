#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#include <fmod_studio.hpp>

#define PLASMA_FMOD_ASSERT(func)                                                                      \
  {                                                                                               \
    auto fmodErrorCode = func;                                                                    \
    if (fmodErrorCode != FMOD_OK)                                                                 \
      plLog::Error("FMOD call failed: '" PLASMA_STRINGIZE(func) "' - Error code {0}", fmodErrorCode); \
  }
// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library
