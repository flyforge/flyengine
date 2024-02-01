#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <Foundation/Reflection/Implementation/RTTI.h>

bool plReflectedClass::IsInstanceOf(const plRTTI* pType) const
{
  return GetDynamicRTTI()->IsDerivedFrom(pType);
}


