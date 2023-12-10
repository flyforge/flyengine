#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <Foundation/Reflection/Implementation/RTTI.h>

bool plReflectedClass::IsInstanceOf(const plRTTI* pType) const
{
  return GetDynamicRTTI()->IsDerivedFrom(pType);
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_DynamicRTTI);
