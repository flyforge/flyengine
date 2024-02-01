#include <Foundation/FoundationInternal.h>
PL_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/EnvironmentVariableUtils.h>

plString plEnvironmentVariableUtils::GetValueStringImpl(plStringView sName, plStringView sDefault)
{
  PL_ASSERT_NOT_IMPLEMENTED
  return "";
}

plResult plEnvironmentVariableUtils::SetValueStringImpl(plStringView sName, plStringView szValue)
{
  PL_ASSERT_NOT_IMPLEMENTED
  return PL_FAILURE;
}

bool plEnvironmentVariableUtils::IsVariableSetImpl(plStringView sName)
{
  return false;
}

plResult plEnvironmentVariableUtils::UnsetVariableImpl(plStringView sName)
{
  return PL_FAILURE;
}
