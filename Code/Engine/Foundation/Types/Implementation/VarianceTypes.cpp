#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plVarianceTypeBase, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Variance", m_fVariance)
  }
    PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVarianceTypeFloat, plVarianceTypeBase, 1, plRTTIDefaultAllocator<plVarianceTypeFloat>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Value", m_Value)
  }
    PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVarianceTypeTime, plVarianceTypeBase, 1, plRTTIDefaultAllocator<plVarianceTypeTime>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Value", m_Value)
  }
    PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVarianceTypeAngle, plVarianceTypeBase, 1, plRTTIDefaultAllocator<plVarianceTypeAngle>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Value", m_Value)
  }
    PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

PL_DEFINE_CUSTOM_VARIANT_TYPE(plVarianceTypeFloat);
PL_DEFINE_CUSTOM_VARIANT_TYPE(plVarianceTypeTime);
PL_DEFINE_CUSTOM_VARIANT_TYPE(plVarianceTypeAngle);

PL_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_VarianceTypes);
