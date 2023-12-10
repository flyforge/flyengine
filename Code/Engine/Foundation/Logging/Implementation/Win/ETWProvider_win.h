#pragma once

#include <Foundation/Basics.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)

#  include <Foundation/FoundationInternal.h>
#  include <Foundation/Logging/Log.h>

PLASMA_FOUNDATION_INTERNAL_HEADER

class plETWProvider
{
public:
  plETWProvider();
  ~plETWProvider();

  void LogMessge(plLogMsgType::Enum eventType, plUInt8 uiIndentation, plStringView sText);

  static plETWProvider& GetInstance();
};
#endif
