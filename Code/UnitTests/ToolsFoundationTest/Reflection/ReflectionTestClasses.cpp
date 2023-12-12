#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plIntegerStruct, plNoBase, 1, plRTTIDefaultAllocator<plIntegerStruct>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Int8", GetInt8, SetInt8),
    PLASMA_ACCESSOR_PROPERTY("UInt8", GetUInt8, SetUInt8),
    PLASMA_MEMBER_PROPERTY("Int16", m_iInt16),
    PLASMA_MEMBER_PROPERTY("UInt16", m_iUInt16),
    PLASMA_ACCESSOR_PROPERTY("Int32", GetInt32, SetInt32),
    PLASMA_ACCESSOR_PROPERTY("UInt32", GetUInt32, SetUInt32),
    PLASMA_MEMBER_PROPERTY("Int64", m_iInt64),
    PLASMA_MEMBER_PROPERTY("UInt64", m_iUInt64),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;


PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plFloatStruct, plNoBase, 1, plRTTIDefaultAllocator<plFloatStruct>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Float", GetFloat, SetFloat),
    PLASMA_ACCESSOR_PROPERTY("Double", GetDouble, SetDouble),
    PLASMA_ACCESSOR_PROPERTY("Time", GetTime, SetTime),
    PLASMA_ACCESSOR_PROPERTY("Angle", GetAngle, SetAngle),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;


PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPODClass, 1, plRTTIDefaultAllocator<plPODClass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Integer", m_IntegerStruct),
    PLASMA_MEMBER_PROPERTY("Float", m_FloatStruct),
    PLASMA_ACCESSOR_PROPERTY("Bool", GetBool, SetBool),
    PLASMA_ACCESSOR_PROPERTY("Color", GetColor, SetColor),
    PLASMA_MEMBER_PROPERTY("ColorUB", m_Color2),
    PLASMA_ACCESSOR_PROPERTY("String", GetString, SetString),
    PLASMA_ACCESSOR_PROPERTY("Buffer", GetBuffer, SetBuffer),
    PLASMA_ACCESSOR_PROPERTY("VarianceAngle", GetCustom, SetCustom),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMathClass, 1, plRTTIDefaultAllocator<plMathClass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Vec2", GetVec2, SetVec2),
    PLASMA_ACCESSOR_PROPERTY("Vec3", GetVec3, SetVec3),
    PLASMA_ACCESSOR_PROPERTY("Vec4", GetVec4, SetVec4),
    PLASMA_MEMBER_PROPERTY("Vec2I", m_Vec2I),
    PLASMA_MEMBER_PROPERTY("Vec3I", m_Vec3I),
    PLASMA_MEMBER_PROPERTY("Vec4I", m_Vec4I),
    PLASMA_ACCESSOR_PROPERTY("Quat", GetQuat, SetQuat),
    PLASMA_ACCESSOR_PROPERTY("Mat3", GetMat3, SetMat3),
    PLASMA_ACCESSOR_PROPERTY("Mat4", GetMat4, SetMat4),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plExampleEnum, 1)
  PLASMA_ENUM_CONSTANTS(plExampleEnum::Value1, plExampleEnum::Value2)
  PLASMA_ENUM_CONSTANT(plExampleEnum::Value3),
PLASMA_END_STATIC_REFLECTED_ENUM;


PLASMA_BEGIN_STATIC_REFLECTED_BITFLAGS(plExampleBitflags, 1)
  PLASMA_BITFLAGS_CONSTANTS(plExampleBitflags::Value1, plExampleBitflags::Value2)
  PLASMA_BITFLAGS_CONSTANT(plExampleBitflags::Value3),
PLASMA_END_STATIC_REFLECTED_BITFLAGS;


PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEnumerationsClass, 1, plRTTIDefaultAllocator<plEnumerationsClass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_ACCESSOR_PROPERTY("Enum", plExampleEnum, GetEnum, SetEnum),
    PLASMA_BITFLAGS_ACCESSOR_PROPERTY("Bitflags", plExampleBitflags, GetBitflags, SetBitflags),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


PLASMA_BEGIN_STATIC_REFLECTED_TYPE(InnerStruct, plNoBase, 1, plRTTIDefaultAllocator<InnerStruct>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("IP1", m_fP1),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;


PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(OuterClass, 1, plRTTIDefaultAllocator<OuterClass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Inner", m_Inner1),
    PLASMA_MEMBER_PROPERTY("OP1", m_fP1),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(ExtendedOuterClass, 1, plRTTIDefaultAllocator<ExtendedOuterClass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("MORE", m_more),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plObjectTest, 1, plRTTIDefaultAllocator<plObjectTest>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("MemberClass", m_MemberClass),
    PLASMA_ARRAY_MEMBER_PROPERTY("StandardTypeArray", m_StandardTypeArray),
    PLASMA_ARRAY_MEMBER_PROPERTY("ClassArray", m_ClassArray),
    PLASMA_ARRAY_MEMBER_PROPERTY("ClassPtrArray", m_ClassPtrArray)->AddFlags(plPropertyFlags::PointerOwner),
    PLASMA_SET_ACCESSOR_PROPERTY("StandardTypeSet", GetStandardTypeSet, StandardTypeSetInsert, StandardTypeSetRemove),
    PLASMA_SET_MEMBER_PROPERTY("SubObjectSet", m_SubObjectSet)->AddFlags(plPropertyFlags::PointerOwner),
    PLASMA_MAP_MEMBER_PROPERTY("StandardTypeMap", m_StandardTypeMap),
    PLASMA_MAP_MEMBER_PROPERTY("ClassMap", m_ClassMap),
    PLASMA_MAP_MEMBER_PROPERTY("ClassPtrMap", m_ClassPtrMap)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMirrorTest, 1, plRTTIDefaultAllocator<plMirrorTest>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Math", m_math),
    PLASMA_MEMBER_PROPERTY("Object", m_object),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plArrayPtr<const plString> plObjectTest::GetStandardTypeSet() const
{
  return m_StandardTypeSet;
}

void plObjectTest::StandardTypeSetInsert(const plString& value)
{
  if (!m_StandardTypeSet.Contains(value))
    m_StandardTypeSet.PushBack(value);
}

void plObjectTest::StandardTypeSetRemove(const plString& value)
{
  m_StandardTypeSet.RemoveAndCopy(value);
}
