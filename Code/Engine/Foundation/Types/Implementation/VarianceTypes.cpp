#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plVarianceTypeBase, plNoBase, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Variance", m_fVariance)
  }
    PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plVarianceTypeFloat, plVarianceTypeBase, 1, plRTTIDefaultAllocator<plVarianceTypeFloat>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Value", m_Value)
  }
    PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plVarianceTypeTime, plVarianceTypeBase, 1, plRTTIDefaultAllocator<plVarianceTypeTime>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Value", m_Value)
  }
    PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plVarianceTypeAngle, plVarianceTypeBase, 1, plRTTIDefaultAllocator<plVarianceTypeAngle>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Value", m_Value)
  }
    PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plVarianceTypeFloat);
PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plVarianceTypeTime);
PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plVarianceTypeAngle);

PLASMA_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_VarianceTypes);
