#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

plString plEnvironmentVariableUtils::GetValueStringImpl(plStringView sName, plStringView sDefault)
{
  PLASMA_ASSERT_NOT_IMPLEMENTED
  return "";
}

plResult plEnvironmentVariableUtils::SetValueStringImpl(plStringView sName, plStringView szValue)
{
  PLASMA_ASSERT_NOT_IMPLEMENTED
  return PLASMA_FAILURE;
}

bool plEnvironmentVariableUtils::IsVariableSetImpl(plStringView sName)
{
  return false;
}

plResult plEnvironmentVariableUtils::UnsetVariableImpl(plStringView sName)
{
  return PLASMA_FAILURE;
}
