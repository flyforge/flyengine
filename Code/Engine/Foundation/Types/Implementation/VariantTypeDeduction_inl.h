


/// \cond

template <>
struct plVariantTypeDeduction<bool>
{
  static constexpr plVariantType::Enum value = plVariantType::Bool;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = bool;
  using ReturnType = bool;
};

template <>
struct plVariantTypeDeduction<plInt8>
{
  static constexpr plVariantType::Enum value = plVariantType::Int8;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plInt8;
};

template <>
struct plVariantTypeDeduction<plUInt8>
{
  static constexpr plVariantType::Enum value = plVariantType::UInt8;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plUInt8;
};

template <>
struct plVariantTypeDeduction<plInt16>
{
  static constexpr plVariantType::Enum value = plVariantType::Int16;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plInt16;
};

template <>
struct plVariantTypeDeduction<plUInt16>
{
  static constexpr plVariantType::Enum value = plVariantType::UInt16;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plUInt16;
};

template <>
struct plVariantTypeDeduction<plInt32>
{
  static constexpr plVariantType::Enum value = plVariantType::Int32;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plInt32;
};

template <>
struct plVariantTypeDeduction<plUInt32>
{
  static constexpr plVariantType::Enum value = plVariantType::UInt32;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plUInt32;
};

template <>
struct plVariantTypeDeduction<plInt64>
{
  static constexpr plVariantType::Enum value = plVariantType::Int64;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plInt64;
};

template <>
struct plVariantTypeDeduction<plUInt64>
{
  static constexpr plVariantType::Enum value = plVariantType::UInt64;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plUInt64;
};

template <>
struct plVariantTypeDeduction<float>
{
  static constexpr plVariantType::Enum value = plVariantType::Float;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = float;
};

template <>
struct plVariantTypeDeduction<double>
{
  static constexpr plVariantType::Enum value = plVariantType::Double;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = double;
};

template <>
struct plVariantTypeDeduction<plColor>
{
  static constexpr plVariantType::Enum value = plVariantType::Color;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plColor;
};

template <>
struct plVariantTypeDeduction<plColorGammaUB>
{
  static constexpr plVariantType::Enum value = plVariantType::ColorGamma;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plColorGammaUB;
};

template <>
struct plVariantTypeDeduction<plVec2>
{
  static constexpr plVariantType::Enum value = plVariantType::Vector2;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plVec2;
};

template <>
struct plVariantTypeDeduction<plVec3>
{
  static constexpr plVariantType::Enum value = plVariantType::Vector3;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plVec3;
};

template <>
struct plVariantTypeDeduction<plVec4>
{
  static constexpr plVariantType::Enum value = plVariantType::Vector4;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plVec4;
};

template <>
struct plVariantTypeDeduction<plVec2I32>
{
  static constexpr plVariantType::Enum value = plVariantType::Vector2I;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plVec2I32;
};

template <>
struct plVariantTypeDeduction<plVec3I32>
{
  static constexpr plVariantType::Enum value = plVariantType::Vector3I;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plVec3I32;
};

template <>
struct plVariantTypeDeduction<plVec4I32>
{
  static constexpr plVariantType::Enum value = plVariantType::Vector4I;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plVec4I32;
};

template <>
struct plVariantTypeDeduction<plVec2U32>
{
  static constexpr plVariantType::Enum value = plVariantType::Vector2U;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plVec2U32;
};

template <>
struct plVariantTypeDeduction<plVec3U32>
{
  static constexpr plVariantType::Enum value = plVariantType::Vector3U;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plVec3U32;
};

template <>
struct plVariantTypeDeduction<plVec4U32>
{
  static constexpr plVariantType::Enum value = plVariantType::Vector4U;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plVec4U32;
};

template <>
struct plVariantTypeDeduction<plQuat>
{
  static constexpr plVariantType::Enum value = plVariantType::Quaternion;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plQuat;
};

template <>
struct plVariantTypeDeduction<plMat3>
{
  static constexpr plVariantType::Enum value = plVariantType::Matrix3;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plMat3;
};

template <>
struct plVariantTypeDeduction<plMat4>
{
  static constexpr plVariantType::Enum value = plVariantType::Matrix4;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plMat4;
};

template <>
struct plVariantTypeDeduction<plTransform>
{
  static constexpr plVariantType::Enum value = plVariantType::Transform;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plTransform;
};

template <>
struct plVariantTypeDeduction<plString>
{
  static constexpr plVariantType::Enum value = plVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plString;
};

template <>
struct plVariantTypeDeduction<plUntrackedString>
{
  static constexpr plVariantType::Enum value = plVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plString;
};

template <>
struct plVariantTypeDeduction<plStringView>
{
  static constexpr plVariantType::Enum value = plVariantType::StringView;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plStringView;
};

template <>
struct plVariantTypeDeduction<plDataBuffer>
{
  static constexpr plVariantType::Enum value = plVariantType::DataBuffer;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plDataBuffer;
};

template <>
struct plVariantTypeDeduction<char*>
{
  static constexpr plVariantType::Enum value = plVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plString;
};

template <>
struct plVariantTypeDeduction<const char*>
{
  static constexpr plVariantType::Enum value = plVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plString;
};

template <size_t N>
struct plVariantTypeDeduction<char[N]>
{
  static constexpr plVariantType::Enum value = plVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plString;
};

template <size_t N>
struct plVariantTypeDeduction<const char[N]>
{
  static constexpr plVariantType::Enum value = plVariantType::String;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plString;
};

template <>
struct plVariantTypeDeduction<plTime>
{
  static constexpr plVariantType::Enum value = plVariantType::Time;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plTime;
};

template <>
struct plVariantTypeDeduction<plUuid>
{
  static constexpr plVariantType::Enum value = plVariantType::Uuid;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plUuid;
};

template <>
struct plVariantTypeDeduction<plAngle>
{
  static constexpr plVariantType::Enum value = plVariantType::Angle;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plAngle;
};

template <>
struct plVariantTypeDeduction<plHashedString>
{
  static constexpr plVariantType::Enum value = plVariantType::HashedString;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plHashedString;
};

template <>
struct plVariantTypeDeduction<plTempHashedString>
{
  static constexpr plVariantType::Enum value = plVariantType::TempHashedString;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plTempHashedString;
};

template <>
struct plVariantTypeDeduction<plVariantArray>
{
  static constexpr plVariantType::Enum value = plVariantType::VariantArray;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plVariantArray;
};

template <>
struct plVariantTypeDeduction<plArrayPtr<plVariant>>
{
  static constexpr plVariantType::Enum value = plVariantType::VariantArray;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plVariantArray;
};


template <>
struct plVariantTypeDeduction<plVariantDictionary>
{
  static constexpr plVariantType::Enum value = plVariantType::VariantDictionary;
  static constexpr bool forceSharing = true;
  static constexpr bool hasReflectedMembers = false;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plVariantDictionary;
};

namespace plInternal
{
  template <int v>
  struct PointerDeductionHelper
  {
  };

  template <>
  struct PointerDeductionHelper<0>
  {
    using StorageType = void*;
  };

  template <>
  struct PointerDeductionHelper<1>
  {
    using StorageType = plReflectedClass*;
  };
} // namespace plInternal

template <>
struct plVariantTypeDeduction<plTypedPointer>
{
  static constexpr plVariantType::Enum value = plVariantType::TypedPointer;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::DirectCast;

  using StorageType = plTypedPointer;
};

template <typename T>
struct plVariantTypeDeduction<T*>
{
  static constexpr plVariantType::Enum value = plVariantType::TypedPointer;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::PointerCast;

  using StorageType = plTypedPointer;
};

template <>
struct plVariantTypeDeduction<plTypedObject>
{
  static constexpr plVariantType::Enum value = plVariantType::TypedObject;
  static constexpr bool forceSharing = false;
  static constexpr bool hasReflectedMembers = true;
  static constexpr plVariantClass::Enum classification = plVariantClass::TypedObject;

  using StorageType = plTypedObject;
};

/// \endcond
