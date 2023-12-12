/// \cond

template <>
struct plVariantTypeDeduction<bool>
{
  enum
  {
    value = plVariantType::Bool,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = bool;
  using ReturnType = bool;
};

template <>
struct plVariantTypeDeduction<plInt8>
{
  enum
  {
    value = plVariantType::Int8,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plInt8;
};

template <>
struct plVariantTypeDeduction<plUInt8>
{
  enum
  {
    value = plVariantType::UInt8,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plUInt8;
};

template <>
struct plVariantTypeDeduction<plInt16>
{
  enum
  {
    value = plVariantType::Int16,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plInt16;
};

template <>
struct plVariantTypeDeduction<plUInt16>
{
  enum
  {
    value = plVariantType::UInt16,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plUInt16;
};

template <>
struct plVariantTypeDeduction<plInt32>
{
  enum
  {
    value = plVariantType::Int32,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plInt32;
};

template <>
struct plVariantTypeDeduction<plUInt32>
{
  enum
  {
    value = plVariantType::UInt32,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plUInt32;
};

template <>
struct plVariantTypeDeduction<plInt64>
{
  enum
  {
    value = plVariantType::Int64,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plInt64;
};

template <>
struct plVariantTypeDeduction<plUInt64>
{
  enum
  {
    value = plVariantType::UInt64,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plUInt64;
};

template <>
struct plVariantTypeDeduction<float>
{
  enum
  {
    value = plVariantType::Float,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = float;
};

template <>
struct plVariantTypeDeduction<double>
{
  enum
  {
    value = plVariantType::Double,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = double;
};

template <>
struct plVariantTypeDeduction<plColor>
{
  enum
  {
    value = plVariantType::Color,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plColor;
};

template <>
struct plVariantTypeDeduction<plColorGammaUB>
{
  enum
  {
    value = plVariantType::ColorGamma,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plColorGammaUB;
};

template <>
struct plVariantTypeDeduction<plVec2>
{
  enum
  {
    value = plVariantType::Vector2,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plVec2;
};

template <>
struct plVariantTypeDeduction<plVec3>
{
  enum
  {
    value = plVariantType::Vector3,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plVec3;
};

template <>
struct plVariantTypeDeduction<plVec4>
{
  enum
  {
    value = plVariantType::Vector4,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plVec4;
};

template <>
struct plVariantTypeDeduction<plVec2I32>
{
  enum
  {
    value = plVariantType::Vector2I,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plVec2I32;
};

template <>
struct plVariantTypeDeduction<plVec3I32>
{
  enum
  {
    value = plVariantType::Vector3I,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plVec3I32;
};

template <>
struct plVariantTypeDeduction<plVec4I32>
{
  enum
  {
    value = plVariantType::Vector4I,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plVec4I32;
};

template <>
struct plVariantTypeDeduction<plVec2U32>
{
  enum
  {
    value = plVariantType::Vector2U,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plVec2U32;
};

template <>
struct plVariantTypeDeduction<plVec3U32>
{
  enum
  {
    value = plVariantType::Vector3U,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plVec3U32;
};

template <>
struct plVariantTypeDeduction<plVec4U32>
{
  enum
  {
    value = plVariantType::Vector4U,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plVec4U32;
};

template <>
struct plVariantTypeDeduction<plQuat>
{
  enum
  {
    value = plVariantType::Quaternion,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plQuat;
};

template <>
struct plVariantTypeDeduction<plMat3>
{
  enum
  {
    value = plVariantType::Matrix3,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plMat3;
};

template <>
struct plVariantTypeDeduction<plMat4>
{
  enum
  {
    value = plVariantType::Matrix4,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plMat4;
};

template <>
struct plVariantTypeDeduction<plTransform>
{
  enum
  {
    value = plVariantType::Transform,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plTransform;
};

template <>
struct plVariantTypeDeduction<plString>
{
  enum
  {
    value = plVariantType::String,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plString;
};

template <>
struct plVariantTypeDeduction<plUntrackedString>
{
  enum
  {
    value = plVariantType::String,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plString;
};

template <>
struct plVariantTypeDeduction<plStringView>
{
  enum
  {
    value = plVariantType::StringView,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plStringView;
};

template <>
struct plVariantTypeDeduction<plDataBuffer>
{
  enum
  {
    value = plVariantType::DataBuffer,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plDataBuffer;
};

template <>
struct plVariantTypeDeduction<char*>
{
  enum
  {
    value = plVariantType::String,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plString;
};

template <>
struct plVariantTypeDeduction<const char*>
{
  enum
  {
    value = plVariantType::String,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plString;
};

template <size_t N>
struct plVariantTypeDeduction<char[N]>
{
  enum
  {
    value = plVariantType::String,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plString;
};

template <size_t N>
struct plVariantTypeDeduction<const char[N]>
{
  enum
  {
    value = plVariantType::String,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plString;
};

template <>
struct plVariantTypeDeduction<plTime>
{
  enum
  {
    value = plVariantType::Time,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plTime;
};

template <>
struct plVariantTypeDeduction<plUuid>
{
  enum
  {
    value = plVariantType::Uuid,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plUuid;
};

template <>
struct plVariantTypeDeduction<plAngle>
{
  enum
  {
    value = plVariantType::Angle,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plAngle;
};

template <>
struct plVariantTypeDeduction<plHashedString>
{
  enum
  {
    value = plVariantType::HashedString,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plHashedString;
};

template <>
struct plVariantTypeDeduction<plTempHashedString>
{
  enum
  {
    value = plVariantType::TempHashedString,
    forceSharing = false,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plTempHashedString;
};

template <>
struct plVariantTypeDeduction<plVariantArray>
{
  enum
  {
    value = plVariantType::VariantArray,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plVariantArray;
};

template <>
struct plVariantTypeDeduction<plArrayPtr<plVariant>>
{
  enum
  {
    value = plVariantType::VariantArray,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plVariantArray;
};


template <>
struct plVariantTypeDeduction<plVariantDictionary>
{
  enum
  {
    value = plVariantType::VariantDictionary,
    forceSharing = true,
    hasReflectedMembers = false,
    classification = plVariantClass::DirectCast
  };

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
  enum
  {
    value = plVariantType::TypedPointer,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::DirectCast
  };

  using StorageType = plTypedPointer;
};

template <typename T>
struct plVariantTypeDeduction<T*>
{
  enum
  {
    value = plVariantType::TypedPointer,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::PointerCast
  };

  using StorageType = plTypedPointer;
};

template <>
struct plVariantTypeDeduction<plTypedObject>
{
  enum
  {
    value = plVariantType::TypedObject,
    forceSharing = false,
    hasReflectedMembers = true,
    classification = plVariantClass::TypedObject
  };

  using StorageType = plTypedObject;
};

/// \endcond
