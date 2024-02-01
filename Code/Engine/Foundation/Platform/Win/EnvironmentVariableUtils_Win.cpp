#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Logging/Log.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/System/EnvironmentVariableUtils.h>
#  include <Foundation/Threading/Mutex.h>
#  include <Foundation/Utilities/ConversionUtils.h>

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <intsafe.h>

plString plEnvironmentVariableUtils::GetValueStringImpl(plStringView sName, plStringView szDefault)
{
  plStringWChar szwName(sName);
  wchar_t szStaticValueBuffer[64] = {0};
  size_t uiRequiredSize = 0;

  errno_t res = _wgetenv_s(&uiRequiredSize, szStaticValueBuffer, szwName);

  // Variable doesn't exist
  if (uiRequiredSize == 0)
  {
    return szDefault;
  }

  // Succeeded
  if (res == 0)
  {
    return plString(szStaticValueBuffer);
  }
  // Static buffer was too small, do a heap allocation to query the value
  else if (res == ERANGE)
  {
    PL_ASSERT_DEV(uiRequiredSize != SIZE_T_MAX, "");
    const size_t uiDynamicSize = uiRequiredSize + 1;
    wchar_t* szDynamicBuffer = PL_DEFAULT_NEW_RAW_BUFFER(wchar_t, uiDynamicSize);
    plMemoryUtils::ZeroFill(szDynamicBuffer, uiDynamicSize);

    res = _wgetenv_s(&uiRequiredSize, szDynamicBuffer, uiDynamicSize, szwName);

    if (res != 0)
    {
      plLog::Error("Error getting environment variable \"{0}\" with dynamic buffer.", sName);
      PL_DEFAULT_DELETE_RAW_BUFFER(szDynamicBuffer);
      return szDefault;
    }
    else
    {
      plString retVal(szDynamicBuffer);
      PL_DEFAULT_DELETE_RAW_BUFFER(szDynamicBuffer);
      return retVal;
    }
  }
  else
  {
    plLog::Warning("Couldn't get environment variable value for \"{0}\", got {1} as a result.", sName, res);
    return szDefault;
  }
}

plResult plEnvironmentVariableUtils::SetValueStringImpl(plStringView sName, plStringView szValue)
{
  plStringWChar szwName(sName);
  plStringWChar szwValue(szValue);

  if (_wputenv_s(szwName, szwValue) == 0)
    return PL_SUCCESS;
  else
    return PL_FAILURE;
}

bool plEnvironmentVariableUtils::IsVariableSetImpl(plStringView sName)
{
  plStringWChar szwName(sName);
  wchar_t szStaticValueBuffer[16] = {0};
  size_t uiRequiredSize = 0;

  errno_t res = _wgetenv_s(&uiRequiredSize, szStaticValueBuffer, szwName);

  if (res == 0 || res == ERANGE)
  {
    // Variable doesn't exist if uiRequiredSize is 0
    return uiRequiredSize > 0;
  }
  else
  {
    plLog::Error("plEnvironmentVariableUtils::IsVariableSet(\"{0}\") got {1} from _wgetenv_s.", sName, res);
    return false;
  }
}

plResult plEnvironmentVariableUtils::UnsetVariableImpl(plStringView sName)
{
  plStringWChar szwName(sName);

  if (_wputenv_s(szwName, L"") == 0)
    return PL_SUCCESS;
  else
    return PL_FAILURE;
}

#endif


