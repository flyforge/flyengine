#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Transform.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plEnumBase, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plBitflagsBase, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plReflectedClass, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

// *********************************************
// ***** Standard POD Types for Properties *****

PL_BEGIN_STATIC_REFLECTED_TYPE(bool, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(float, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(double, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plInt8, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plUInt8, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plInt16, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plUInt16, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plInt32, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plUInt32, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plInt64, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plUInt64, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plConstCharPtr, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plTime, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(MakeFromNanoseconds, In, "Nanoseconds")->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(MakeFromMicroseconds, In, "Microseconds")->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(MakeFromMilliseconds, In, "Milliseconds")->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(MakeFromSeconds, In, "Seconds")->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(MakeFromMinutes, In, "Minutes")->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(MakeFromHours, In, "Hours")->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(MakeZero)->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(AsFloatInSeconds),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plColor, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("r", r),
    PL_MEMBER_PROPERTY("g", g),
    PL_MEMBER_PROPERTY("b", b),
    PL_MEMBER_PROPERTY("a", a),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(float, float, float),
    PL_CONSTRUCTOR_PROPERTY(float, float, float, float),
    PL_CONSTRUCTOR_PROPERTY(plColorLinearUB),
    PL_CONSTRUCTOR_PROPERTY(plColorGammaUB),
    PL_SCRIPT_FUNCTION_PROPERTY(MakeRGBA, In, "R", In, "G", In, "B", In, "A")->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(MakeHSV, In, "Hue", In, "Saturation", In, "Value")->AddFlags(plPropertyFlags::Const),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plColorBaseUB, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("r", r),
    PL_MEMBER_PROPERTY("g", g),
    PL_MEMBER_PROPERTY("b", b),
    PL_MEMBER_PROPERTY("a", a),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plUInt8, plUInt8, plUInt8),
    PL_CONSTRUCTOR_PROPERTY(plUInt8, plUInt8, plUInt8, plUInt8),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plColorGammaUB, plColorBaseUB, 1, plRTTINoAllocator)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plUInt8, plUInt8, plUInt8),
    PL_CONSTRUCTOR_PROPERTY(plUInt8, plUInt8, plUInt8, plUInt8),
    PL_CONSTRUCTOR_PROPERTY(const plColor&),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plColorLinearUB, plColorBaseUB, 1, plRTTINoAllocator)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plUInt8, plUInt8, plUInt8),
    PL_CONSTRUCTOR_PROPERTY(plUInt8, plUInt8, plUInt8, plUInt8),
    PL_CONSTRUCTOR_PROPERTY(const plColor&),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVec2, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("x", x),
    PL_MEMBER_PROPERTY("y", y),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(float),
    PL_CONSTRUCTOR_PROPERTY(float, float),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVec3, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("x", x),
    PL_MEMBER_PROPERTY("y", y),
    PL_MEMBER_PROPERTY("z", z),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(float),
    PL_CONSTRUCTOR_PROPERTY(float, float, float),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVec4, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("x", x),
    PL_MEMBER_PROPERTY("y", y),
    PL_MEMBER_PROPERTY("z", z),
    PL_MEMBER_PROPERTY("w", w),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(float),
    PL_CONSTRUCTOR_PROPERTY(float, float, float, float),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVec2I32, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("x", x),
    PL_MEMBER_PROPERTY("y", y),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plInt32),
    PL_CONSTRUCTOR_PROPERTY(plInt32, plInt32),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVec3I32, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("x", x),
    PL_MEMBER_PROPERTY("y", y),
    PL_MEMBER_PROPERTY("z", z),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plInt32),
    PL_CONSTRUCTOR_PROPERTY(plInt32, plInt32, plInt32),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVec4I32, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("x", x),
    PL_MEMBER_PROPERTY("y", y),
    PL_MEMBER_PROPERTY("z", z),
    PL_MEMBER_PROPERTY("w", w),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plInt32),
    PL_CONSTRUCTOR_PROPERTY(plInt32, plInt32, plInt32, plInt32),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVec2U32, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("x", x),
    PL_MEMBER_PROPERTY("y", y),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plUInt32),
    PL_CONSTRUCTOR_PROPERTY(plUInt32, plUInt32),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVec3U32, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("x", x),
    PL_MEMBER_PROPERTY("y", y),
    PL_MEMBER_PROPERTY("z", z),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plUInt32),
    PL_CONSTRUCTOR_PROPERTY(plUInt32, plUInt32, plUInt32),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVec4U32, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("x", x),
    PL_MEMBER_PROPERTY("y", y),
    PL_MEMBER_PROPERTY("z", z),
    PL_MEMBER_PROPERTY("w", w),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plUInt32),
    PL_CONSTRUCTOR_PROPERTY(plUInt32, plUInt32, plUInt32, plUInt32),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plQuat, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("x", x),
    PL_MEMBER_PROPERTY("y", y),
    PL_MEMBER_PROPERTY("z", z),
    PL_MEMBER_PROPERTY("w", w),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(float, float, float, float),
    PL_SCRIPT_FUNCTION_PROPERTY(MakeFromAxisAndAngle, In, "Axis", In, "Angle")->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(MakeShortestRotation, In, "DirFrom", In, "DirTo")->AddFlags(plPropertyFlags::Const),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plMat3, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plMat4, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plTransform, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Position", m_vPosition),
    PL_MEMBER_PROPERTY("Rotation", m_qRotation),
    PL_MEMBER_PROPERTY("Scale", m_vScale),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plVec3, plQuat),
    PL_CONSTRUCTOR_PROPERTY(plVec3, plQuat, plVec3),
    PL_SCRIPT_FUNCTION_PROPERTY(Make, In, "Position", In, "Rotation", In, "Scale")->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(MakeLocalTransform, In, "Parent", In, "GlobalChild")->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(MakeGlobalTransform, In, "Parent", In, "LocalChild")->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(TransformPosition, In, "Position"),
    PL_SCRIPT_FUNCTION_PROPERTY(TransformDirection, In, "Direction"),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_ENUM(plBasisAxis, 1)
PL_ENUM_CONSTANT(plBasisAxis::PositiveX),
PL_ENUM_CONSTANT(plBasisAxis::PositiveY),
PL_ENUM_CONSTANT(plBasisAxis::PositiveZ),
PL_ENUM_CONSTANT(plBasisAxis::NegativeX),
PL_ENUM_CONSTANT(plBasisAxis::NegativeY),
PL_ENUM_CONSTANT(plBasisAxis::NegativeZ),
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_TYPE(plUuid, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVariant, plNoBase, 3, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVariantArray, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVariantDictionary, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plString, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plUntrackedString, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plStringView, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plDataBuffer, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plHashedString, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plTempHashedString, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plAngle, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(MakeFromDegree, In, "Degree")->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(MakeFromRadian, In, "Radian")->AddFlags(plPropertyFlags::Const),
    PL_SCRIPT_FUNCTION_PROPERTY(GetNormalizedRange),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plFloatInterval, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Start", m_StartValue),
    PL_MEMBER_PROPERTY("End", m_EndValue),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plIntInterval, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Start", m_StartValue),
    PL_MEMBER_PROPERTY("End", m_EndValue),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

// **********************************************************************
// ***** Various RTTI infos that can't be put next to their classes *****

PL_BEGIN_STATIC_REFLECTED_BITFLAGS(plTypeFlags, 1)
PL_BITFLAGS_CONSTANTS(plTypeFlags::StandardType, plTypeFlags::IsEnum, plTypeFlags::Bitflags, plTypeFlags::Class, plTypeFlags::Abstract, plTypeFlags::Phantom, plTypeFlags::Minimal)
PL_END_STATIC_REFLECTED_BITFLAGS;

PL_BEGIN_STATIC_REFLECTED_BITFLAGS(plPropertyFlags, 1)
PL_BITFLAGS_CONSTANTS(plPropertyFlags::StandardType, plPropertyFlags::IsEnum, plPropertyFlags::Bitflags, plPropertyFlags::Class)
PL_BITFLAGS_CONSTANTS(plPropertyFlags::Const, plPropertyFlags::Reference, plPropertyFlags::Pointer)
PL_BITFLAGS_CONSTANTS(plPropertyFlags::PointerOwner, plPropertyFlags::ReadOnly, plPropertyFlags::Hidden, plPropertyFlags::Phantom)
PL_END_STATIC_REFLECTED_BITFLAGS;

PL_BEGIN_STATIC_REFLECTED_ENUM(plFunctionType, 1)
PL_ENUM_CONSTANTS(plFunctionType::Member, plFunctionType::StaticMember, plFunctionType::Constructor)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plVariantType, 1)
PL_ENUM_CONSTANTS(plVariantType::Invalid, plVariantType::Bool, plVariantType::Int8, plVariantType::UInt8, plVariantType::Int16, plVariantType::UInt16)
PL_ENUM_CONSTANTS(plVariantType::Int32, plVariantType::UInt32, plVariantType::Int64, plVariantType::UInt64, plVariantType::Float, plVariantType::Double)
PL_ENUM_CONSTANTS(plVariantType::Color, plVariantType::Vector2, plVariantType::Vector3, plVariantType::Vector4)
PL_ENUM_CONSTANTS(plVariantType::Vector2I, plVariantType::Vector3I, plVariantType::Vector4I, plVariantType::Vector2U, plVariantType::Vector3U, plVariantType::Vector4U)
PL_ENUM_CONSTANTS(plVariantType::Quaternion, plVariantType::Matrix3, plVariantType::Matrix4, plVariantType::Transform)
PL_ENUM_CONSTANTS(plVariantType::String, plVariantType::StringView, plVariantType::DataBuffer, plVariantType::Time, plVariantType::Uuid, plVariantType::Angle, plVariantType::ColorGamma)
PL_ENUM_CONSTANTS(plVariantType::HashedString, plVariantType::TempHashedString)
PL_ENUM_CONSTANTS(plVariantType::VariantArray, plVariantType::VariantDictionary, plVariantType::TypedPointer, plVariantType::TypedObject)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plPropertyCategory, 1)
PL_ENUM_CONSTANTS(plPropertyCategory::Constant, plPropertyCategory::Member, plPropertyCategory::Function, plPropertyCategory::Array, plPropertyCategory::Set, plPropertyCategory::Map)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

PL_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_StandardTypes);
