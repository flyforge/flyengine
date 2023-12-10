#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <stdlib.h>

plString plEnvironmentVariableUtils::GetValueStringImpl(plStringView sName, plStringView sDefault)
{
  plStringBuilder tmp;
  const char* value = getenv(sName.GetData(tmp));
  return value != nullptr ? value : sDefault;
}

plResult plEnvironmentVariableUtils::SetValueStringImpl(plStringView sName, plStringView sValue)
{
  plStringBuilder tmp, tmp2;
  if (setenv(sName.GetData(tmp), sValue.GetData(tmp2), 1) == 0)
    return PLASMA_SUCCESS;
  else
    return PLASMA_FAILURE;
}

bool plEnvironmentVariableUtils::IsVariableSetImpl(plStringView sName)
{
  plStringBuilder tmp;
  return getenv(sName.GetData(tmp)) != nullptr;
}

plResult plEnvironmentVariableUtils::UnsetVariableImpl(plStringView sName)
{
  plStringBuilder tmp;
  if (unsetenv(sName.GetData(tmp)) == 0)
    return PLASMA_SUCCESS;
  else
    return PLASMA_FAILURE;
}
