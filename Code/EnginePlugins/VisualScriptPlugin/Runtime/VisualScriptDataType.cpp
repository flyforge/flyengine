#include <Core/CorePCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/World.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plVisualScriptDataType, 1)
  PL_ENUM_CONSTANT(plVisualScriptDataType::Invalid),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Bool),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Byte),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Int),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Int64),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Float),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Double),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Color),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Vector3),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Quaternion),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Transform),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Time),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Angle),
  PL_ENUM_CONSTANT(plVisualScriptDataType::String),
  PL_ENUM_CONSTANT(plVisualScriptDataType::HashedString),
  PL_ENUM_CONSTANT(plVisualScriptDataType::GameObject),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Component),
  PL_ENUM_CONSTANT(plVisualScriptDataType::TypedPointer),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Variant),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Array),
  PL_ENUM_CONSTANT(plVisualScriptDataType::Map),
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

namespace
{
  static plVariantType::Enum s_ScriptDataTypeVariantTypes[] = {
    plVariantType::Invalid, // Invalid,

    plVariantType::Bool,              // Bool,
    plVariantType::UInt8,             // Byte,
    plVariantType::Int32,             // Int,
    plVariantType::Int64,             // Int64,
    plVariantType::Float,             // Float,
    plVariantType::Double,            // Double,
    plVariantType::Color,             // Color,
    plVariantType::Vector3,           // Vector3,
    plVariantType::Quaternion,        // Quaternion,
    plVariantType::Transform,         // Transform,
    plVariantType::Time,              // Time,
    plVariantType::Angle,             // Angle,
    plVariantType::String,            // String,
    plVariantType::HashedString,      // HashedString,
    plVariantType::TypedObject,       // GameObject,
    plVariantType::TypedObject,       // Component,
    plVariantType::TypedPointer,      // TypedPointer,
    plVariantType::Invalid,           // Variant,
    plVariantType::VariantArray,      // Array,
    plVariantType::VariantDictionary, // Map,
    plVariantType::TypedObject,       // Coroutine,
  };
  static_assert(PL_ARRAY_SIZE(s_ScriptDataTypeVariantTypes) == (size_t)plVisualScriptDataType::Count);

  static plUInt32 s_ScriptDataTypeSizes[] = {
    0, // Invalid,

    sizeof(bool),                           // Bool,
    sizeof(plUInt8),                        // Byte,
    sizeof(plInt32),                        // Int,
    sizeof(plInt64),                        // Int64,
    sizeof(float),                          // Float,
    sizeof(double),                         // Double,
    sizeof(plColor),                        // Color,
    sizeof(plVec3),                         // Vector3,
    sizeof(plQuat),                         // Quaternion,
    sizeof(plTransform),                    // Transform,
    sizeof(plTime),                         // Time,
    sizeof(plAngle),                        // Angle,
    sizeof(plString),                       // String,
    sizeof(plHashedString),                 // HashedString,
    sizeof(plVisualScriptGameObjectHandle), // GameObject,
    sizeof(plVisualScriptComponentHandle),  // Component,
    sizeof(plTypedPointer),                 // TypedPointer,
    sizeof(plVariant),                      // Variant,
    sizeof(plVariantArray),                 // Array,
    sizeof(plVariantDictionary),            // Map,
    sizeof(plScriptCoroutineHandle),        // Coroutine,
  };
  static_assert(PL_ARRAY_SIZE(s_ScriptDataTypeSizes) == (size_t)plVisualScriptDataType::Count);

  static plUInt32 s_ScriptDataTypeAlignments[] = {
    0, // Invalid,

    PL_ALIGNMENT_OF(bool),                           // Bool,
    PL_ALIGNMENT_OF(plUInt8),                        // Byte,
    PL_ALIGNMENT_OF(plInt32),                        // Int,
    PL_ALIGNMENT_OF(plInt64),                        // Int64,
    PL_ALIGNMENT_OF(float),                          // Float,
    PL_ALIGNMENT_OF(double),                         // Double,
    PL_ALIGNMENT_OF(plColor),                        // Color,
    PL_ALIGNMENT_OF(plVec3),                         // Vector3,
    PL_ALIGNMENT_OF(plQuat),                         // Quaternion,
    PL_ALIGNMENT_OF(plTransform),                    // Transform,
    PL_ALIGNMENT_OF(plTime),                         // Time,
    PL_ALIGNMENT_OF(plAngle),                        // Angle,
    PL_ALIGNMENT_OF(plString),                       // String,
    PL_ALIGNMENT_OF(plHashedString),                 // HashedString,
    PL_ALIGNMENT_OF(plVisualScriptGameObjectHandle), // GameObject,
    PL_ALIGNMENT_OF(plVisualScriptComponentHandle),  // Component,
    PL_ALIGNMENT_OF(plTypedPointer),                 // TypedPointer,
    PL_ALIGNMENT_OF(plVariant),                      // Variant,
    PL_ALIGNMENT_OF(plVariantArray),                 // Array,
    PL_ALIGNMENT_OF(plVariantDictionary),            // Map,
    PL_ALIGNMENT_OF(plScriptCoroutineHandle),        // Coroutine,
  };
  static_assert(PL_ARRAY_SIZE(s_ScriptDataTypeAlignments) == (size_t)plVisualScriptDataType::Count);

  static const char* s_ScriptDataTypeNames[] = {
    "Invalid",

    "Bool",
    "Byte",
    "Int",
    "Int64",
    "Float",
    "Double",
    "Color",
    "Vector3",
    "Quaternion",
    "Transform",
    "Time",
    "Angle",
    "String",
    "HashedString",
    "GameObject",
    "Component",
    "TypedPointer",
    "Variant",
    "Array",
    "Map",
    "Coroutine",
    "", // Count,
    "Enum",
  };
  static_assert(PL_ARRAY_SIZE(s_ScriptDataTypeNames) == (size_t)plVisualScriptDataType::ExtendedCount);
} // namespace

// static
plVariantType::Enum plVisualScriptDataType::GetVariantType(Enum dataType)
{
  PL_ASSERT_DEBUG(dataType >= 0 && dataType < PL_ARRAY_SIZE(s_ScriptDataTypeVariantTypes), "Out of bounds access");
  return s_ScriptDataTypeVariantTypes[dataType];
}

// static
plVisualScriptDataType::Enum plVisualScriptDataType::FromVariantType(plVariantType::Enum variantType)
{
  switch (variantType)
  {
    case plVariantType::Bool:
      return Bool;
    case plVariantType::Int8:
    case plVariantType::UInt8:
      return Byte;
    case plVariantType::Int16:
    case plVariantType::UInt16:
    case plVariantType::Int32:
    case plVariantType::UInt32:
      return Int;
    case plVariantType::Int64:
    case plVariantType::UInt64:
      return Int64;
    case plVariantType::Float:
      return Float;
    case plVariantType::Double:
      return Double;
    case plVariantType::Color:
      return Color;
    case plVariantType::Vector3:
      return Vector3;
    case plVariantType::Quaternion:
      return Quaternion;
    case plVariantType::Transform:
      return Transform;
    case plVariantType::Time:
      return Time;
    case plVariantType::Angle:
      return Angle;
    case plVariantType::String:
    case plVariantType::StringView:
      return String;
    case plVariantType::HashedString:
    case plVariantType::TempHashedString:
      return HashedString;
    case plVariantType::VariantArray:
      return Array;
    case plVariantType::VariantDictionary:
      return Map;
    default:
      return Invalid;
  }
}

plProcessingStream::DataType plVisualScriptDataType::GetStreamDataType(Enum dataType)
{
  // We treat plColor and plVec4 as the same in the visual script <=> expression binding
  // so ensure that they have the same size and layout
  static_assert(sizeof(plColor) == sizeof(plVec4));
  static_assert(offsetof(plColor, r) == offsetof(plVec4, x));
  static_assert(offsetof(plColor, g) == offsetof(plVec4, y));
  static_assert(offsetof(plColor, b) == offsetof(plVec4, z));
  static_assert(offsetof(plColor, a) == offsetof(plVec4, w));

  switch (dataType)
  {
    case Int:
      return plProcessingStream::DataType::Int;
    case Float:
      return plProcessingStream::DataType::Float;
    case Vector3:
      return plProcessingStream::DataType::Float3;
    case Color:
      return plProcessingStream::DataType::Float4;
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }

  return plProcessingStream::DataType::Float;
}

// static
const plRTTI* plVisualScriptDataType::GetRtti(Enum dataType)
{
  // Define table here to prevent issues with static initialization order
  static const plRTTI* s_Rttis[] = {
    nullptr, // Invalid,

    plGetStaticRTTI<bool>(),                    // Bool,
    plGetStaticRTTI<plUInt8>(),                 // Byte,
    plGetStaticRTTI<plInt32>(),                 // Int,
    plGetStaticRTTI<plInt64>(),                 // Int64,
    plGetStaticRTTI<float>(),                   // Float,
    plGetStaticRTTI<double>(),                  // Double,
    plGetStaticRTTI<plColor>(),                 // Color,
    plGetStaticRTTI<plVec3>(),                  // Vector3,
    plGetStaticRTTI<plQuat>(),                  // Quaternion,
    plGetStaticRTTI<plTransform>(),             // Transform,
    plGetStaticRTTI<plTime>(),                  // Time,
    plGetStaticRTTI<plAngle>(),                 // Angle,
    plGetStaticRTTI<plString>(),                // String,
    plGetStaticRTTI<plHashedString>(),          // HashedString,
    plGetStaticRTTI<plGameObjectHandle>(),      // GameObject,
    plGetStaticRTTI<plComponentHandle>(),       // Component,
    nullptr,                                    // TypedPointer,
    plGetStaticRTTI<plVariant>(),               // Variant,
    plGetStaticRTTI<plVariantArray>(),          // Array,
    plGetStaticRTTI<plVariantDictionary>(),     // Map,
    plGetStaticRTTI<plScriptCoroutineHandle>(), // Coroutine,
    nullptr,                                    // Count,
    nullptr,                                    // EnumValue,
  };
  static_assert(PL_ARRAY_SIZE(s_Rttis) == (size_t)plVisualScriptDataType::ExtendedCount);

  PL_ASSERT_DEBUG(dataType >= 0 && dataType < PL_ARRAY_SIZE(s_Rttis), "Out of bounds access");
  return s_Rttis[dataType];
}

// static
plVisualScriptDataType::Enum plVisualScriptDataType::FromRtti(const plRTTI* pRtti)
{
  Enum res = FromVariantType(pRtti->GetVariantType());
  if (res != Invalid)
    return res;

  if (pRtti->IsDerivedFrom<plGameObject>() || pRtti == plGetStaticRTTI<plGameObjectHandle>())
    return GameObject;

  if (pRtti->IsDerivedFrom<plComponent>() || pRtti == plGetStaticRTTI<plComponentHandle>())
    return Component;

  if (pRtti == plGetStaticRTTI<plScriptCoroutineHandle>())
    return Coroutine;

  if (pRtti->GetTypeFlags().IsSet(plTypeFlags::Class))
    return TypedPointer;

  if (pRtti->GetTypeFlags().IsSet(plTypeFlags::IsEnum))
    return EnumValue;

  if (pRtti == plGetStaticRTTI<plVariant>())
    return Variant;

  return Invalid;
}

// static
plUInt32 plVisualScriptDataType::GetStorageSize(Enum dataType)
{
  PL_ASSERT_DEBUG(dataType >= 0 && dataType < PL_ARRAY_SIZE(s_ScriptDataTypeSizes), "Out of bounds access");
  return s_ScriptDataTypeSizes[dataType];
}

// static
plUInt32 plVisualScriptDataType::GetStorageAlignment(Enum dataType)
{
  PL_ASSERT_DEBUG(dataType >= 0 && dataType < PL_ARRAY_SIZE(s_ScriptDataTypeAlignments), "Out of bounds access");
  return s_ScriptDataTypeAlignments[dataType];
}

// static
const char* plVisualScriptDataType::GetName(Enum dataType)
{
  if (dataType == AnyPointer)
  {
    return "Pointer";
  }
  else if (dataType == Any)
  {
    return "Any";
  }

  PL_ASSERT_DEBUG(dataType >= 0 && dataType < PL_ARRAY_SIZE(s_ScriptDataTypeNames), "Out of bounds access");
  return s_ScriptDataTypeNames[dataType];
}

// static
bool plVisualScriptDataType::CanConvertTo(Enum sourceDataType, Enum targetDataType)
{
  if (sourceDataType == targetDataType ||
      targetDataType == String ||
      targetDataType == HashedString ||
      targetDataType == Variant)
    return true;

  if ((IsNumber(sourceDataType) || sourceDataType == EnumValue) &&
      (IsNumber(targetDataType) || targetDataType == EnumValue))
    return true;

  return false;
}

//////////////////////////////////////////////////////////////////////////

plGameObject* plVisualScriptGameObjectHandle::GetPtr(plUInt32 uiExecutionCounter) const
{
  if (m_uiExecutionCounter == uiExecutionCounter || m_Handle.GetInternalID().m_Data == 0)
  {
    return m_Ptr;
  }

  m_Ptr = nullptr;
  m_uiExecutionCounter = uiExecutionCounter;

  if (plWorld* pWorld = plWorld::GetWorld(m_Handle))
  {
    bool objectExists = pWorld->TryGetObject(m_Handle, m_Ptr);
    PL_IGNORE_UNUSED(objectExists);
  }

  return m_Ptr;
}

plComponent* plVisualScriptComponentHandle::GetPtr(plUInt32 uiExecutionCounter) const
{
  if (m_uiExecutionCounter == uiExecutionCounter || m_Handle.GetInternalID().m_Data == 0)
  {
    return m_Ptr;
  }

  m_Ptr = nullptr;
  m_uiExecutionCounter = uiExecutionCounter;

  if (plWorld* pWorld = plWorld::GetWorld(m_Handle))
  {
    bool componentExists = pWorld->TryGetComponent(m_Handle, m_Ptr);
    PL_IGNORE_UNUSED(componentExists);
  }

  return m_Ptr;
}
