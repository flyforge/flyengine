#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Utilities/ConversionUtils.h>

// The POSIX functions are not thread safe by definition.
static plMutex s_EnvVarMutex;


plString plEnvironmentVariableUtils::GetValueString(plStringView sName, plStringView sDefault /*= nullptr*/)
{
  PLASMA_ASSERT_DEV(!sName.IsEmpty(), "Null or empty name passed to plEnvironmentVariableUtils::GetValueString()");

  PLASMA_LOCK(s_EnvVarMutex);

  return GetValueStringImpl(sName, sDefault);
}

plResult plEnvironmentVariableUtils::SetValueString(plStringView sName, plStringView sValue)
{
  PLASMA_LOCK(s_EnvVarMutex);

  return SetValueStringImpl(sName, sValue);
}

plInt32 plEnvironmentVariableUtils::GetValueInt(plStringView sName, plInt32 iDefault /*= -1*/)
{
  PLASMA_LOCK(s_EnvVarMutex);

  plString value = GetValueString(sName);

  if (value.IsEmpty())
    return iDefault;

  plInt32 iRetVal = 0;
  if (plConversionUtils::StringToInt(value, iRetVal).Succeeded())
    return iRetVal;
  else
    return iDefault;
}

plResult plEnvironmentVariableUtils::SetValueInt(plStringView sName, plInt32 iValue)
{
  plStringBuilder sb;
  sb.Format("{}", iValue);

  return SetValueString(sName, sb);
}

bool plEnvironmentVariableUtils::IsVariableSet(plStringView sName)
{
  PLASMA_LOCK(s_EnvVarMutex);

  return IsVariableSetImpl(sName);
}

plResult plEnvironmentVariableUtils::UnsetVariable(plStringView sName)
{
  PLASMA_LOCK(s_EnvVarMutex);

  return UnsetVariableImpl(sName);
}

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/System/Implementation/Win/EnvironmentVariableUtils_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  include <Foundation/System/Implementation/Win/EnvironmentVariableUtils_win_uwp.h>
#else
#  include <Foundation/System/Implementation/Posix/EnvironmentVariableUtils_posix.h>
#endif


PLASMA_STATICLINK_FILE(Foundation, Foundation_System_Implementation_EnvironmentVariableUtils);
