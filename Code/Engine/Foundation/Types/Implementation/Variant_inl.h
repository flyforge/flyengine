
#define PLASMA_MSVC_WARNING_NUMBER 4702 // Unreachable code for some reason
#include <Foundation/Basics/Compiler/MSVC/DisableWarning_MSVC.h>

PLASMA_ALWAYS_INLINE plVariant::plVariant()
{
  m_uiType = Type::Invalid;
  m_bIsShared = false;
}

#include <Foundation/Basics/Compiler/MSVC/RestoreWarning_MSVC.h>

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plVariant& other)
{
  CopyFrom(other);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(plVariant&& other) noexcept
{
  MoveFrom(std::move(other));
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const bool& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plInt8& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plUInt8& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plInt16& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plUInt16& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plInt32& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plUInt32& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plInt64& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plUInt64& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const float& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const double& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plColor& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plVec2& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plVec3& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plVec4& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plVec2I32& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plVec3I32& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plVec4I32& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plVec2U32& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plVec3U32& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plVec4U32& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plQuat& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plTime& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plUuid& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plAngle& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plColorGammaUB& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plHashedString& value)
{
  InitInplace(value);
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(const plTempHashedString& value)
{
  InitInplace(value);
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::CustomTypeCast, int>>
PLASMA_ALWAYS_INLINE plVariant::plVariant(const T& value)
{
  const constexpr bool forceSharing = TypeDeduction<T>::forceSharing;
  const constexpr bool inlineSized = sizeof(T) <= InlinedStruct::DataSize;
  const constexpr bool isPOD = plIsPodType<T>::value;
  InitTypedObject(value, plTraitInt < (!forceSharing && inlineSized && isPOD) ? 1 : 0 > ());
}

template <typename T>
PLASMA_ALWAYS_INLINE plVariant::plVariant(const T* value)
{
  constexpr bool bla = !std::is_same<T, void>::value;
  PLASMA_CHECK_AT_COMPILETIME(bla);
  InitTypedPointer(const_cast<T*>(value), plGetStaticRTTI<T>());
}

PLASMA_ALWAYS_INLINE plVariant::plVariant(void* value, const plRTTI* pType)
{
  InitTypedPointer(value, pType);
}

PLASMA_ALWAYS_INLINE plVariant::~plVariant()
{
  Release();
}

PLASMA_ALWAYS_INLINE void plVariant::operator=(const plVariant& other)
{
  if (this != &other)
  {
    Release();
    CopyFrom(other);
  }
}

PLASMA_ALWAYS_INLINE void plVariant::operator=(plVariant&& other) noexcept
{
  if (this != &other)
  {
    Release();
    MoveFrom(std::move(other));
  }
}

template <typename T>
PLASMA_ALWAYS_INLINE void plVariant::operator=(const T& value)
{
  *this = plVariant(value);
}

PLASMA_ALWAYS_INLINE bool plVariant::operator!=(const plVariant& other) const
{
  return !(*this == other);
}

template <typename T>
PLASMA_FORCE_INLINE bool plVariant::operator==(const T& other) const
{
  if (IsFloatingPoint())
  {
    if constexpr (TypeDeduction<T>::value > Type::Invalid && TypeDeduction<T>::value <= Type::Double)
    {
      return ConvertNumber<double>() == static_cast<double>(other);
    }

    return false;
  }
  else if (IsNumber())
  {
    if constexpr (TypeDeduction<T>::value > Type::Invalid && TypeDeduction<T>::value <= Type::Double)
    {
      return ConvertNumber<plInt64>() == static_cast<plInt64>(other);
    }

    return false;
  }

  if constexpr (std::is_same_v<T, plHashedString>)
  {
    if (m_uiType == Type::TempHashedString)
    {
      return Cast<plTempHashedString>() == other;
    }
  }
  else if constexpr (std::is_same_v<T, plTempHashedString>)
  {
    if (m_uiType == Type::HashedString)
    {
      return Cast<plHashedString>() == other;
    }
  }

  using StorageType = typename TypeDeduction<T>::StorageType;
  PLASMA_ASSERT_DEV(IsA<StorageType>(), "Stored type '{0}' does not match comparison type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<StorageType>() == other;
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plVariant::operator!=(const T& other) const
{
  return !(*this == other);
}

PLASMA_ALWAYS_INLINE bool plVariant::IsValid() const
{
  return m_uiType != Type::Invalid;
}

PLASMA_ALWAYS_INLINE bool plVariant::IsNumber() const
{
  return IsNumberStatic(m_uiType);
}

PLASMA_ALWAYS_INLINE bool plVariant::IsFloatingPoint() const
{
  return IsFloatingPointStatic(m_uiType);
}

PLASMA_ALWAYS_INLINE bool plVariant::IsString() const
{
  return IsStringStatic(m_uiType);
}

PLASMA_ALWAYS_INLINE bool plVariant::IsHashedString() const
{
  return IsHashedStringStatic(m_uiType);
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::DirectCast, int>>
PLASMA_ALWAYS_INLINE bool plVariant::IsA() const
{
  return m_uiType == TypeDeduction<T>::value;
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::PointerCast, int>>
PLASMA_ALWAYS_INLINE bool plVariant::IsA() const
{
  if (m_uiType == TypeDeduction<T>::value)
  {
    const plTypedPointer& ptr = *reinterpret_cast<const plTypedPointer*>(&m_Data);
    // Always allow cast to void*.
    if constexpr (std::is_same<T, void*>::value || std::is_same<T, const void*>::value)
    {
      return true;
    }
    else if (ptr.m_pType)
    {
      using NonPointerT = typename plTypeTraits<T>::NonConstReferencePointerType;
      const plRTTI* pType = plGetStaticRTTI<NonPointerT>();
      return IsDerivedFrom(ptr.m_pType, pType);
    }
    else if (!ptr.m_pObject)
    {
      // nullptr can be converted to anything
      return true;
    }
  }
  return false;
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::TypedObject, int>>
PLASMA_ALWAYS_INLINE bool plVariant::IsA() const
{
  return m_uiType == TypeDeduction<T>::value;
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::CustomTypeCast, int>>
PLASMA_ALWAYS_INLINE bool plVariant::IsA() const
{
  using NonRefT = typename plTypeTraits<T>::NonConstReferenceType;
  if (m_uiType == TypeDeduction<T>::value)
  {
    if (const plRTTI* pType = GetReflectedType())
    {
      return IsDerivedFrom(pType, plGetStaticRTTI<NonRefT>());
    }
  }
  return false;
}

PLASMA_ALWAYS_INLINE plVariant::Type::Enum plVariant::GetType() const
{
  return static_cast<Type::Enum>(m_uiType);
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::DirectCast, int>>
PLASMA_ALWAYS_INLINE const T& plVariant::Get() const
{
  PLASMA_ASSERT_DEV(IsA<T>(), "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::PointerCast, int>>
PLASMA_ALWAYS_INLINE T plVariant::Get() const
{
  PLASMA_ASSERT_DEV(IsA<T>(), "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::TypedObject, int>>
PLASMA_ALWAYS_INLINE const T plVariant::Get() const
{
  PLASMA_ASSERT_DEV(IsA<T>(), "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::CustomTypeCast, int>>
PLASMA_ALWAYS_INLINE const T& plVariant::Get() const
{
  PLASMA_ASSERT_DEV(m_uiType == TypeDeduction<T>::value, "Stored type '{0}' does not match requested type '{1}'", m_uiType, TypeDeduction<T>::value);
  return Cast<T>();
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::DirectCast, int>>
PLASMA_ALWAYS_INLINE T& plVariant::GetWritable()
{
  GetWriteAccess();
  return const_cast<T&>(Get<T>());
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::PointerCast, int>>
PLASMA_ALWAYS_INLINE T plVariant::GetWritable()
{
  GetWriteAccess();
  return const_cast<T>(Get<T>());
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::CustomTypeCast, int>>
PLASMA_ALWAYS_INLINE T& plVariant::GetWritable()
{
  GetWriteAccess();
  return const_cast<T&>(Get<T>());
}

PLASMA_ALWAYS_INLINE const void* plVariant::GetData() const
{
  if (m_uiType == Type::TypedPointer)
  {
    return Cast<plTypedPointer>().m_pObject;
  }
  return m_bIsShared ? m_Data.shared->m_Ptr : &m_Data;
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plVariant::CanConvertTo() const
{
  return CanConvertTo(static_cast<Type::Enum>(TypeDeduction<T>::value));
}

template <typename T>
T plVariant::ConvertTo(plResult* out_pConversionStatus /* = nullptr*/) const
{
  if (!CanConvertTo<T>())
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = PLASMA_FAILURE;

    return T();
  }

  if (m_uiType == TypeDeduction<T>::value)
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = PLASMA_SUCCESS;

    return Cast<T>();
  }

  T result = {};
  bool bSuccessful = true;
  plVariantHelper::To(*this, result, bSuccessful);

  if (out_pConversionStatus != nullptr)
    *out_pConversionStatus = bSuccessful ? PLASMA_SUCCESS : PLASMA_FAILURE;

  return result;
}


/// private methods

template <typename T>
PLASMA_FORCE_INLINE void plVariant::InitInplace(const T& value)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value != Type::Invalid, "value of this type cannot be stored in a Variant");
  PLASMA_CHECK_AT_COMPILETIME_MSG(plGetTypeClass<T>::value <= plTypeIsMemRelocatable::value, "in place data needs to be POD or mem relocatable");
  PLASMA_CHECK_AT_COMPILETIME_MSG(sizeof(T) <= sizeof(m_Data), "value of this type is too big to bestored inline in a Variant");
  plMemoryUtils::CopyConstruct(reinterpret_cast<T*>(&m_Data), value, 1);

  m_uiType = TypeDeduction<T>::value;
  m_bIsShared = false;
}

template <typename T>
PLASMA_FORCE_INLINE void plVariant::InitTypedObject(const T& value, plTraitInt<0>)
{
  using StorageType = typename TypeDeduction<T>::StorageType;

  PLASMA_CHECK_AT_COMPILETIME_MSG((sizeof(StorageType) > sizeof(InlinedStruct::DataSize)) || TypeDeduction<T>::forceSharing, "Value should be inplace instead.");
  PLASMA_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value == Type::TypedObject, "value of this type cannot be stored in a Variant");
  const plRTTI* pType = plGetStaticRTTI<T>();
  m_Data.shared = PLASMA_DEFAULT_NEW(TypedSharedData<StorageType>, value, pType);
  m_uiType = Type::TypedObject;
  m_bIsShared = true;
}

template <typename T>
PLASMA_FORCE_INLINE void plVariant::InitTypedObject(const T& value, plTraitInt<1>)
{
  using StorageType = typename TypeDeduction<T>::StorageType;
  PLASMA_CHECK_AT_COMPILETIME_MSG((sizeof(StorageType) <= InlinedStruct::DataSize) && !TypeDeduction<T>::forceSharing, "Value can't be stored inplace.");
  PLASMA_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value == Type::TypedObject, "value of this type cannot be stored in a Variant");
  PLASMA_CHECK_AT_COMPILETIME_MSG(plIsPodType<T>::value, "in place data needs to be POD");
  plMemoryUtils::CopyConstruct(reinterpret_cast<T*>(&m_Data), value, 1);
  m_Data.inlined.m_pType = plGetStaticRTTI<T>();
  m_uiType = Type::TypedObject;
  m_bIsShared = false;
}

inline void plVariant::Release()
{
  if (m_bIsShared)
  {
    if (m_Data.shared->m_uiRef.Decrement() == 0)
    {
      PLASMA_DEFAULT_DELETE(m_Data.shared);
    }
  }
}

inline void plVariant::CopyFrom(const plVariant& other)
{
  m_uiType = other.m_uiType;
  m_bIsShared = other.m_bIsShared;

  if (m_bIsShared)
  {
    m_Data.shared = other.m_Data.shared;
    m_Data.shared->m_uiRef.Increment();
  }
  else if (other.IsValid())
  {
    m_Data = other.m_Data;
  }
}

PLASMA_ALWAYS_INLINE void plVariant::MoveFrom(plVariant&& other)
{
  m_uiType = other.m_uiType;
  m_bIsShared = other.m_bIsShared;
  m_Data = other.m_Data;

  other.m_uiType = Type::Invalid;
  other.m_bIsShared = false;
  other.m_Data.shared = nullptr;
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::DirectCast, int>>
const T& plVariant::Cast() const
{
  const bool validType = plConversionTest<T, typename TypeDeduction<T>::StorageType>::sameType;
  PLASMA_CHECK_AT_COMPILETIME_MSG(validType, "Invalid Cast, can only cast to storage type");

  return m_bIsShared ? *static_cast<const T*>(m_Data.shared->m_Ptr) : *reinterpret_cast<const T*>(&m_Data);
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::PointerCast, int>>
T plVariant::Cast() const
{
  const plTypedPointer& ptr = *reinterpret_cast<const plTypedPointer*>(&m_Data);

  const plRTTI* pType = GetReflectedType();
  using NonRefPtrT = typename plTypeTraits<T>::NonConstReferencePointerType;
  if constexpr (!std::is_same<T, void*>::value && !std::is_same<T, const void*>::value)
  {
    PLASMA_ASSERT_DEV(pType == nullptr || IsDerivedFrom(pType, plGetStaticRTTI<NonRefPtrT>()), "Object of type '{0}' does not derive from '{}'", GetTypeName(pType), GetTypeName(plGetStaticRTTI<NonRefPtrT>()));
  }
  return static_cast<T>(ptr.m_pObject);
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::TypedObject, int>>
const T plVariant::Cast() const
{
  plTypedObject obj;
  obj.m_pObject = GetData();
  obj.m_pType = GetReflectedType();
  return obj;
}

template <typename T, typename std::enable_if_t<plVariantTypeDeduction<T>::classification == plVariantClass::CustomTypeCast, int>>
const T& plVariant::Cast() const
{
  const plRTTI* pType = GetReflectedType();
  using NonRefT = typename plTypeTraits<T>::NonConstReferenceType;
  PLASMA_ASSERT_DEV(IsDerivedFrom(pType, plGetStaticRTTI<NonRefT>()), "Object of type '{0}' does not derive from '{}'", GetTypeName(pType), GetTypeName(plGetStaticRTTI<NonRefT>()));

  return m_bIsShared ? *static_cast<const T*>(m_Data.shared->m_Ptr) : *reinterpret_cast<const T*>(&m_Data);
}

PLASMA_ALWAYS_INLINE bool plVariant::IsNumberStatic(plUInt32 type)
{
  return type > Type::FirstStandardType && type <= Type::Double;
}

PLASMA_ALWAYS_INLINE bool plVariant::IsFloatingPointStatic(plUInt32 type)
{
  return type == Type::Float || type == Type::Double;
}

PLASMA_ALWAYS_INLINE bool plVariant::IsStringStatic(plUInt32 type)
{
  return type == Type::String || type == Type::StringView;
}

PLASMA_ALWAYS_INLINE bool plVariant::IsHashedStringStatic(plUInt32 type)
{
  return type == Type::HashedString || type == Type::TempHashedString;
}

PLASMA_ALWAYS_INLINE bool plVariant::IsVector2Static(plUInt32 type)
{
  return type == Type::Vector2 || type == Type::Vector2I || type == Type::Vector2U;
}

PLASMA_ALWAYS_INLINE bool plVariant::IsVector3Static(plUInt32 type)
{
  return type == Type::Vector3 || type == Type::Vector3I || type == Type::Vector3U;
}

PLASMA_ALWAYS_INLINE bool plVariant::IsVector4Static(plUInt32 type)
{
  return type == Type::Vector4 || type == Type::Vector4I || type == Type::Vector4U;
}

template <typename T>
T plVariant::ConvertNumber() const
{
  switch (m_uiType)
  {
    case Type::Bool:
      return static_cast<T>(Cast<bool>());
    case Type::Int8:
      return static_cast<T>(Cast<plInt8>());
    case Type::UInt8:
      return static_cast<T>(Cast<plUInt8>());
    case Type::Int16:
      return static_cast<T>(Cast<plInt16>());
    case Type::UInt16:
      return static_cast<T>(Cast<plUInt16>());
    case Type::Int32:
      return static_cast<T>(Cast<plInt32>());
    case Type::UInt32:
      return static_cast<T>(Cast<plUInt32>());
    case Type::Int64:
      return static_cast<T>(Cast<plInt64>());
    case Type::UInt64:
      return static_cast<T>(Cast<plUInt64>());
    case Type::Float:
      return static_cast<T>(Cast<float>());
    case Type::Double:
      return static_cast<T>(Cast<double>());
  }

  PLASMA_REPORT_FAILURE("Variant is not a number");
  return T(0);
}

template <>
struct plHashHelper<plVariant>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(const plVariant& value)
  {
    plUInt64 uiHash = value.ComputeHash(0);
    return (plUInt32)uiHash;
  }

  PLASMA_ALWAYS_INLINE static bool Equal(const plVariant& a, const plVariant& b)
  {
    return a.GetType() == b.GetType() && a == b;
  }
};
