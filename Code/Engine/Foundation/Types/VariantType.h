#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Types.h>

class plReflectedClass;
class plVariant;
struct plTime;
class plUuid;
struct plStringView;
struct plTypedObject;
struct plTypedPointer;

using plDataBuffer = plDynamicArray<plUInt8>;
using plVariantArray = plDynamicArray<plVariant>;
using plVariantDictionary = plHashTable<plString, plVariant>;

/// \brief This enum describes the type of data that is currently stored inside the variant.
struct plVariantType
{
  using StorageType = plUInt8;
  /// \brief This enum describes the type of data that is currently stored inside the variant.
  /// Note that changes to this enum require an increase of the reflection version and either
  /// patches to the serializer or a re-export of binary data that contains plVariants.
  enum Enum : plUInt8
  {
    Invalid = 0, ///< The variant stores no (valid) data at the moment.

    /// *** Types that are flagged as 'StandardTypes' (see DetermineTypeFlags) ***
    FirstStandardType = 1,
    Bool,       ///< The variant stores a bool.
    Int8,             ///< The variant stores an plInt8.
    UInt8,            ///< The variant stores an plUInt8.
    Int16,            ///< The variant stores an plInt16.
    UInt16,           ///< The variant stores an plUInt16.
    Int32,            ///< The variant stores an plInt32.
    UInt32,           ///< The variant stores an plUInt32.
    Int64,            ///< The variant stores an plInt64.
    UInt64,           ///< The variant stores an plUInt64.
    Float,            ///< The variant stores a float.
    Double,           ///< The variant stores a double.
    Color,            ///< The variant stores an plColor.
    Vector2,          ///< The variant stores an plVec2.
    Vector3,          ///< The variant stores an plVec3.
    Vector4,          ///< The variant stores an plVec4.
    Vector2I,         ///< The variant stores an plVec2I32.
    Vector3I,         ///< The variant stores an plVec3I32.
    Vector4I,         ///< The variant stores an plVec4I32.
    Vector2U,         ///< The variant stores an plVec2U32.
    Vector3U,         ///< The variant stores an plVec3U32.
    Vector4U,         ///< The variant stores an plVec4U32.
    Quaternion,       ///< The variant stores an plQuat.
    Matrix3,          ///< The variant stores an plMat3. A heap allocation is required to store this data type.
    Matrix4,          ///< The variant stores an plMat4. A heap allocation is required to store this data type.
    Transform,        ///< The variant stores an plTransform. A heap allocation is required to store this data type.
    String,           ///< The variant stores a string. A heap allocation is required to store this data type.
    StringView,       ///< The variant stores an plStringView.
    DataBuffer,       ///< The variant stores an plDataBuffer, a typedef to DynamicArray<plUInt8>. A heap allocation is required to store this data type.
    Time,             ///< The variant stores an plTime value.
    Uuid,             ///< The variant stores an plUuid value.
    Angle,            ///< The variant stores an plAngle value.
    ColorGamma,       ///< The variant stores an plColorGammaUB value.
    HashedString,     ///< The variant stores an plHashedString value.
    TempHashedString, ///< The variant stores an plTempHashedString value.
    LastStandardType,
    /// *** Types that are flagged as 'StandardTypes' (see DetermineTypeFlags) ***

    FirstExtendedType = 64,
    VariantArray,      ///< The variant stores an array of plVariant's. A heap allocation is required to store this data type.
    VariantDictionary, ///< The variant stores a dictionary (hashmap) of plVariant's. A heap allocation is required to store this type.
    TypedPointer,      ///< The variant stores an plTypedPointer value. Reflected type and data queries will match the pointed to object.
    TypedObject,       ///< The variant stores an plTypedObject value. Reflected type and data queries will match the object. A heap allocation is required to store this type if it is larger than 16 bytes or not POD.
    LastExtendedType,  ///< Number of values for plVariant::Type.

    MAX_ENUM_VALUE = LastExtendedType,
    Default = Invalid ///< Default value used by plEnum.
  };
};

PLASMA_DEFINE_AS_POD_TYPE(plVariantType::Enum);

struct plVariantClass
{
  enum Enum
  {
    Invalid,
    DirectCast,     ///< A standard type
    PointerCast,    ///< Any cast to T*
    TypedObject,    ///< plTypedObject cast. Needed because at no point does and plVariant ever store a plTypedObject so it can't be returned as a const reference.
    CustomTypeCast, ///< Custom object types
  };
};

/// \brief A helper struct to convert the C++ type, which is passed as the template argument, into one of the plVariant::Type enum values.
template <typename T>
struct plVariantTypeDeduction
{
  enum
  {
    value = plVariantType::Invalid,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::Invalid
  };

  using StorageType = T;
};

/// \brief Declares a custom variant type, allowing it to be stored by value inside an plVariant.
///
/// Needs to be called from the same header that defines the type.
/// \sa PLASMA_DEFINE_CUSTOM_VARIANT_TYPE
#define PLASMA_DECLARE_CUSTOM_VARIANT_TYPE(TYPE)          \
  template <>                                         \
  struct plVariantTypeDeduction<TYPE>                 \
  {                                                   \
    enum                                              \
    {                                                 \
      value = plVariantType::TypedObject,             \
      forceSharing = false,                           \
      hasReflectedMembers = true,                     \
      classification = plVariantClass::CustomTypeCast \
    };                                                \
    using StorageType = TYPE;                         \
  };

#include <Foundation/Types/Implementation/VariantTypeDeduction_inl.h>
