#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Types/VariantTypeRegistry.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_64BIT)
PLASMA_CHECK_AT_COMPILETIME(sizeof(plVariant) == 24);
#else
PLASMA_CHECK_AT_COMPILETIME(sizeof(plVariant) == 20);
#endif

/// constructors

plVariant::plVariant(const plMat3& value)
{
  InitShared(value);
}

plVariant::plVariant(const plMat4& value)
{
  InitShared(value);
}

plVariant::plVariant(const plTransform& value)
{
  InitShared(value);
}

plVariant::plVariant(const char* value)
{
  InitShared(value);
}

plVariant::plVariant(const plString& value)
{
  InitShared(value);
}

plVariant::plVariant(const plStringView& value, bool bCopyString)
{
  if (bCopyString)
    InitShared(plString(value));
  else
    InitInplace(value);
}

plVariant::plVariant(const plUntrackedString& value)
{
  InitShared(value);
}

plVariant::plVariant(const plDataBuffer& value)
{
  InitShared(value);
}

plVariant::plVariant(const plVariantArray& value)
{
  using StorageType = typename TypeDeduction<plVariantArray>::StorageType;
  m_Data.shared = PLASMA_DEFAULT_NEW(TypedSharedData<StorageType>, value, nullptr);
  m_uiType = TypeDeduction<plVariantArray>::value;
  m_bIsShared = true;
}

plVariant::plVariant(const plVariantDictionary& value)
{
  using StorageType = typename TypeDeduction<plVariantDictionary>::StorageType;
  m_Data.shared = PLASMA_DEFAULT_NEW(TypedSharedData<StorageType>, value, nullptr);
  m_uiType = TypeDeduction<plVariantDictionary>::value;
  m_bIsShared = true;
}

plVariant::plVariant(const plTypedPointer& value)
{
  InitInplace(value);
}

plVariant::plVariant(const plTypedObject& value)
{
  void* ptr = plReflectionSerializer::Clone(value.m_pObject, value.m_pType);
  m_Data.shared = PLASMA_DEFAULT_NEW(RTTISharedData, ptr, value.m_pType);
  m_uiType = Type::TypedObject;
  m_bIsShared = true;
}

void plVariant::CopyTypedObject(const void* value, const plRTTI* pType)
{
  Release();
  void* ptr = plReflectionSerializer::Clone(value, pType);
  m_Data.shared = PLASMA_DEFAULT_NEW(RTTISharedData, ptr, pType);
  m_uiType = Type::TypedObject;
  m_bIsShared = true;
}

void plVariant::MoveTypedObject(void* value, const plRTTI* pType)
{
  Release();
  m_Data.shared = PLASMA_DEFAULT_NEW(RTTISharedData, value, pType);
  m_uiType = Type::TypedObject;
  m_bIsShared = true;
}

template <typename T>
PLASMA_ALWAYS_INLINE void plVariant::InitShared(const T& value)
{
  using StorageType = typename TypeDeduction<T>::StorageType;

  PLASMA_CHECK_AT_COMPILETIME_MSG((sizeof(StorageType) > sizeof(Data)) || TypeDeduction<T>::forceSharing, "value of this type should be stored inplace");
  PLASMA_CHECK_AT_COMPILETIME_MSG(TypeDeduction<T>::value != Type::Invalid, "value of this type cannot be stored in a Variant");
  const plRTTI* pType = plGetStaticRTTI<T>();

  m_Data.shared = PLASMA_DEFAULT_NEW(TypedSharedData<StorageType>, value, pType);
  m_uiType = TypeDeduction<T>::value;
  m_bIsShared = true;
}

/// functors

struct ComputeHashFunc
{
  template <typename T>
  PLASMA_FORCE_INLINE plUInt64 operator()(const plVariant& v, const void* pData, plUInt64 uiSeed)
  {
    PLASMA_CHECK_AT_COMPILETIME_MSG(sizeof(typename plVariant::TypeDeduction<T>::StorageType) <= sizeof(float) * 4 &&
                                  !plVariant::TypeDeduction<T>::forceSharing,
      "This type requires special handling! Add a specialization below.");
    return plHashingUtils::xxHash64(pData, sizeof(T), uiSeed);
  }
};

template <>
PLASMA_ALWAYS_INLINE plUInt64 ComputeHashFunc::operator()<plString>(const plVariant& v, const void* pData, plUInt64 uiSeed)
{
  auto pString = static_cast<const plString*>(pData);

  return plHashingUtils::xxHash64String(*pString, uiSeed);
}

template <>
PLASMA_ALWAYS_INLINE plUInt64 ComputeHashFunc::operator()<plMat3>(const plVariant& v, const void* pData, plUInt64 uiSeed)
{
  return plHashingUtils::xxHash64(pData, sizeof(plMat3), uiSeed);
}

template <>
PLASMA_ALWAYS_INLINE plUInt64 ComputeHashFunc::operator()<plMat4>(const plVariant& v, const void* pData, plUInt64 uiSeed)
{
  return plHashingUtils::xxHash64(pData, sizeof(plMat4), uiSeed);
}

template <>
PLASMA_ALWAYS_INLINE plUInt64 ComputeHashFunc::operator()<plTransform>(const plVariant& v, const void* pData, plUInt64 uiSeed)
{
  return plHashingUtils::xxHash64(pData, sizeof(plTransform), uiSeed);
}

template <>
PLASMA_ALWAYS_INLINE plUInt64 ComputeHashFunc::operator()<plDataBuffer>(const plVariant& v, const void* pData, plUInt64 uiSeed)
{
  auto pDataBuffer = static_cast<const plDataBuffer*>(pData);

  return plHashingUtils::xxHash64(pDataBuffer->GetData(), pDataBuffer->GetCount(), uiSeed);
}

template <>
PLASMA_FORCE_INLINE plUInt64 ComputeHashFunc::operator()<plVariantArray>(const plVariant& v, const void* pData, plUInt64 uiSeed)
{
  auto pVariantArray = static_cast<const plVariantArray*>(pData);

  plUInt64 uiHash = uiSeed;
  for (const plVariant& var : *pVariantArray)
  {
    uiHash = var.ComputeHash(uiHash);
  }

  return uiHash;
}

template <>
plUInt64 ComputeHashFunc::operator()<plVariantDictionary>(const plVariant& v, const void* pData, plUInt64 uiSeed)
{
  auto pVariantDictionary = static_cast<const plVariantDictionary*>(pData);

  plHybridArray<plUInt64, 128> hashes;
  hashes.Reserve(pVariantDictionary->GetCount() * 2);

  for (auto& it : *pVariantDictionary)
  {
    hashes.PushBack(plHashingUtils::xxHash64String(it.Key(), uiSeed));
    hashes.PushBack(it.Value().ComputeHash(uiSeed));
  }

  hashes.Sort();

  return plHashingUtils::xxHash64(hashes.GetData(), hashes.GetCount() * sizeof(plUInt64), uiSeed);
}

template <>
PLASMA_FORCE_INLINE plUInt64 ComputeHashFunc::operator()<plTypedPointer>(const plVariant& v, const void* pData, plUInt64 uiSeed)
{
  PLASMA_IGNORE_UNUSED(pData);

  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

template <>
PLASMA_FORCE_INLINE plUInt64 ComputeHashFunc::operator()<plTypedObject>(const plVariant& v, const void* pData, plUInt64 uiSeed)
{
  auto pType = v.GetReflectedType();

  const plVariantTypeInfo* pTypeInfo = plVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pType);
  PLASMA_ASSERT_DEV(pTypeInfo, "The type '{0}' was declared but not defined, add PLASMA_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable comparing of this variant type.", pType->GetTypeName());
  PLASMA_MSVC_ANALYSIS_ASSUME(pTypeInfo != nullptr);
  plUInt32 uiHash32 = pTypeInfo->Hash(pData);

  return plHashingUtils::xxHash64(&uiHash32, sizeof(plUInt32), uiSeed);
}

struct CompareFunc
{
  template <typename T>
  PLASMA_ALWAYS_INLINE void operator()()
  {
    m_bResult = m_pThis->Cast<T>() == m_pOther->Cast<T>();
  }

  const plVariant* m_pThis;
  const plVariant* m_pOther;
  bool m_bResult;
};

template <>
PLASMA_FORCE_INLINE void CompareFunc::operator()<plTypedObject>()
{
  m_bResult = false;
  plTypedObject A = m_pThis->Get<plTypedObject>();
  plTypedObject B = m_pOther->Get<plTypedObject>();
  if (A.m_pType == B.m_pType)
  {
    const plVariantTypeInfo* pTypeInfo = plVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(A.m_pType);
    PLASMA_ASSERT_DEV(pTypeInfo, "The type '{0}' was declared but not defined, add PLASMA_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable comparing of this variant type.", A.m_pType->GetTypeName());
    PLASMA_MSVC_ANALYSIS_ASSUME(pTypeInfo != nullptr);
    m_bResult = pTypeInfo->Equal(A.m_pObject, B.m_pObject);
  }
}

struct IndexFunc
{
  template <typename T>
  PLASMA_FORCE_INLINE plVariant Impl(plTraitInt<1>)
  {
    const plRTTI* pRtti = m_pThis->GetReflectedType();
    const plAbstractMemberProperty* pProp = plReflectionUtils::GetMemberProperty(pRtti, m_uiIndex);
    if (!pProp)
      return plVariant();

    if (m_pThis->GetType() == plVariantType::TypedPointer)
    {
      const plTypedPointer& ptr = m_pThis->Get<plTypedPointer>();
      if (ptr.m_pObject)
        return plReflectionUtils::GetMemberPropertyValue(pProp, ptr.m_pObject);
      else
        return plVariant();
    }
    return plReflectionUtils::GetMemberPropertyValue(pProp, m_pThis->GetData());
  }

  template <typename T>
  PLASMA_ALWAYS_INLINE plVariant Impl(plTraitInt<0>)
  {
    return plVariant();
  }

  template <typename T>
  PLASMA_FORCE_INLINE void operator()()
  {
    m_Result = Impl<T>(plTraitInt<plVariant::TypeDeduction<T>::hasReflectedMembers>());
  }

  const plVariant* m_pThis;
  plVariant m_Result;
  plUInt32 m_uiIndex;
};

struct KeyFunc
{
  template <typename T>
  PLASMA_FORCE_INLINE plVariant Impl(plTraitInt<1>)
  {
    const plRTTI* pRtti = m_pThis->GetReflectedType();
    const plAbstractMemberProperty* pProp = plReflectionUtils::GetMemberProperty(pRtti, m_szKey);
    if (!pProp)
      return plVariant();
    if (m_pThis->GetType() == plVariantType::TypedPointer)
    {
      const plTypedPointer& ptr = m_pThis->Get<plTypedPointer>();
      if (ptr.m_pObject)
        return plReflectionUtils::GetMemberPropertyValue(pProp, ptr.m_pObject);
      else
        return plVariant();
    }
    return plReflectionUtils::GetMemberPropertyValue(pProp, m_pThis->GetData());
  }

  template <typename T>
  PLASMA_ALWAYS_INLINE plVariant Impl(plTraitInt<0>)
  {
    return plVariant();
  }

  template <typename T>
  PLASMA_ALWAYS_INLINE void operator()()
  {
    m_Result = Impl<T>(plTraitInt<plVariant::TypeDeduction<T>::hasReflectedMembers>());
  }

  const plVariant* m_pThis;
  plVariant m_Result;
  const char* m_szKey;
};

struct ConvertFunc
{
  template <typename T>
  PLASMA_ALWAYS_INLINE void operator()()
  {
    T result = {};
    plVariantHelper::To(*m_pThis, result, m_bSuccessful);

    if constexpr (std::is_same_v<T, plStringView>)
    {
      m_Result = plVariant(result, false);
    }
    else
    {
      m_Result = result;
    }
  }

  const plVariant* m_pThis;
  plVariant m_Result;
  bool m_bSuccessful;
};

/// public methods

bool plVariant::operator==(const plVariant& other) const
{
  if (m_uiType == Type::Invalid && other.m_uiType == Type::Invalid)
  {
    return true;
  }
  else if ((IsFloatingPoint() && other.IsNumber()) || (other.IsFloatingPoint() && IsNumber()))
  {
    // if either of them is a floating point number, compare them as doubles

    return ConvertNumber<double>() == other.ConvertNumber<double>();
  }
  else if (IsNumber() && other.IsNumber())
  {
    return ConvertNumber<plInt64>() == other.ConvertNumber<plInt64>();
  }
  else if (IsString() && other.IsString())
  {
    const plStringView a = IsA<plStringView>() ? Get<plStringView>() : plStringView(Get<plString>().GetData());
    const plStringView b = other.IsA<plStringView>() ? other.Get<plStringView>() : plStringView(other.Get<plString>().GetData());
    return a.IsEqual(b);
  }
  else if (IsHashedString() && other.IsHashedString())
  {
    const plTempHashedString a = IsA<plTempHashedString>() ? Get<plTempHashedString>() : plTempHashedString(Get<plHashedString>());
    const plTempHashedString b = other.IsA<plTempHashedString>() ? other.Get<plTempHashedString>() : plTempHashedString(other.Get<plHashedString>());
    return a == b;
  }
  else if (m_uiType == other.m_uiType)
  {
    CompareFunc compareFunc;
    compareFunc.m_pThis = this;
    compareFunc.m_pOther = &other;

    DispatchTo(compareFunc, GetType());

    return compareFunc.m_bResult;
  }

  return false;
}

plTypedPointer plVariant::GetWriteAccess()
{
  plTypedPointer obj;
  obj.m_pType = GetReflectedType();
  if (m_bIsShared)
  {
    if (m_Data.shared->m_uiRef > 1)
    {
      // We need to make sure we hold the only reference to the shared data to be able to edit it.
      SharedData* pData = m_Data.shared->Clone();
      Release();
      m_Data.shared = pData;
    }
    obj.m_pObject = m_Data.shared->m_Ptr;
  }
  else
  {
    obj.m_pObject = m_uiType == Type::TypedPointer ? Cast<plTypedPointer>().m_pObject : &m_Data;
  }
  return obj;
}

const plVariant plVariant::operator[](plUInt32 uiIndex) const
{
  if (m_uiType == Type::VariantArray)
  {
    const plVariantArray& a = Cast<plVariantArray>();
    if (uiIndex < a.GetCount())
      return a[uiIndex];
  }
  else if (IsValid())
  {
    IndexFunc func;
    func.m_pThis = this;
    func.m_uiIndex = uiIndex;

    DispatchTo(func, GetType());

    return func.m_Result;
  }

  return plVariant();
}

const plVariant plVariant::operator[](StringWrapper key) const
{
  if (m_uiType == Type::VariantDictionary)
  {
    plVariant result;
    Cast<plVariantDictionary>().TryGetValue(key.m_str, result);
    return result;
  }
  else if (IsValid())
  {
    KeyFunc func;
    func.m_pThis = this;
    func.m_szKey = key.m_str;

    DispatchTo(func, GetType());

    return func.m_Result;
  }

  return plVariant();
}

bool plVariant::CanConvertTo(Type::Enum type) const
{
  if (m_uiType == type)
    return true;

  if (type == Type::Invalid)
    return false;

  const bool bTargetIsString = (type == Type::String) || (type == Type::HashedString) || (type == Type::TempHashedString);

  if (bTargetIsString && m_uiType == Type::Invalid)
    return true;

  if (bTargetIsString && (m_uiType > Type::FirstStandardType && m_uiType < Type::LastStandardType && m_uiType != Type::DataBuffer))
    return true;
  if (bTargetIsString && (m_uiType == Type::VariantArray || m_uiType == Type::VariantDictionary))
    return true;
  if (type == Type::StringView && (m_uiType == Type::String || m_uiType == Type::HashedString))
    return true;
  if (type == Type::TempHashedString && m_uiType == Type::HashedString)
    return true;

  if (!IsValid())
    return false;

  if (IsNumberStatic(type) && (IsNumber() || m_uiType == Type::String || m_uiType == Type::HashedString))
    return true;

  if (IsVector2Static(type) && (IsVector2Static(m_uiType)))
    return true;

  if (IsVector3Static(type) && (IsVector3Static(m_uiType)))
    return true;

  if (IsVector4Static(type) && (IsVector4Static(m_uiType)))
    return true;

  if (type == Type::Color && m_uiType == Type::ColorGamma)
    return true;
  if (type == Type::ColorGamma && m_uiType == Type::Color)
    return true;

  return false;
}

plVariant plVariant::ConvertTo(Type::Enum type, plResult* out_pConversionStatus /* = nullptr*/) const
{
  if (!CanConvertTo(type))
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = PLASMA_FAILURE;

    return plVariant(); // creates an invalid variant
  }

  if (m_uiType == type)
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = PLASMA_SUCCESS;

    return *this;
  }

  ConvertFunc convertFunc;
  convertFunc.m_pThis = this;
  convertFunc.m_bSuccessful = true;

  DispatchTo(convertFunc, type);

  if (out_pConversionStatus != nullptr)
    *out_pConversionStatus = convertFunc.m_bSuccessful ? PLASMA_SUCCESS : PLASMA_FAILURE;

  return convertFunc.m_Result;
}

plUInt64 plVariant::ComputeHash(plUInt64 uiSeed) const
{
  if (!IsValid())
    return uiSeed;

  ComputeHashFunc obj;
  return DispatchTo<ComputeHashFunc>(obj, GetType(), *this, GetData(), uiSeed + GetType());
}


inline plVariant::RTTISharedData::RTTISharedData(void* pData, const plRTTI* pType)
  : SharedData(pData, pType)
{
  PLASMA_ASSERT_DEBUG(pType != nullptr && pType->GetAllocator()->CanAllocate(), "");
}

inline plVariant::RTTISharedData::~RTTISharedData()
{
  m_pType->GetAllocator()->Deallocate(m_Ptr);
}


plVariant::plVariant::SharedData* plVariant::RTTISharedData::Clone() const
{
  void* ptr = plReflectionSerializer::Clone(m_Ptr, m_pType);
  return PLASMA_DEFAULT_NEW(RTTISharedData, ptr, m_pType);
}

struct GetTypeFromVariantFunc
{
  template <typename T>
  PLASMA_ALWAYS_INLINE void operator()()
  {
    m_pType = plGetStaticRTTI<T>();
  }

  const plVariant* m_pVariant;
  const plRTTI* m_pType;
};

template <>
PLASMA_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<plVariantArray>()
{
  m_pType = nullptr;
}
template <>
PLASMA_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<plVariantDictionary>()
{
  m_pType = nullptr;
}
template <>
PLASMA_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<plTypedPointer>()
{
  m_pType = m_pVariant->Cast<plTypedPointer>().m_pType;
}
template <>
PLASMA_ALWAYS_INLINE void GetTypeFromVariantFunc::operator()<plTypedObject>()
{
  m_pType = m_pVariant->m_bIsShared ? m_pVariant->m_Data.shared->m_pType : m_pVariant->m_Data.inlined.m_pType;
}

const plRTTI* plVariant::GetReflectedType() const
{
  if (m_uiType != Type::Invalid)
  {
    GetTypeFromVariantFunc func;
    func.m_pVariant = this;
    func.m_pType = nullptr;
    plVariant::DispatchTo(func, GetType());
    return func.m_pType;
  }
  return nullptr;
}

void plVariant::InitTypedPointer(void* value, const plRTTI* pType)
{
  plTypedPointer ptr;
  ptr.m_pObject = value;
  ptr.m_pType = pType;

  plMemoryUtils::CopyConstruct(reinterpret_cast<plTypedPointer*>(&m_Data), ptr, 1);

  m_uiType = TypeDeduction<plTypedPointer>::value;
  m_bIsShared = false;
}

bool plVariant::IsDerivedFrom(const plRTTI* pType1, const plRTTI* pType2)
{
  return pType1->IsDerivedFrom(pType2);
}

plStringView plVariant::GetTypeName(const plRTTI* pType)
{
  return pType->GetTypeName();
}

//////////////////////////////////////////////////////////////////////////

struct AddFunc
{
  template <typename T>
  PLASMA_ALWAYS_INLINE void operator()(const plVariant& a, const plVariant& b, plVariant& out_res)
  {
    if constexpr (std::is_same_v<T, plInt8> || std::is_same_v<T, plUInt8> ||
                  std::is_same_v<T, plInt16> || std::is_same_v<T, plUInt16> ||
                  std::is_same_v<T, plInt32> || std::is_same_v<T, plUInt32> ||
                  std::is_same_v<T, plInt64> || std::is_same_v<T, plUInt64> ||
                  std::is_same_v<T, float> || std::is_same_v<T, double> ||
                  std::is_same_v<T, plColor> ||
                  std::is_same_v<T, plVec2> || std::is_same_v<T, plVec3> || std::is_same_v<T, plVec4> ||
                  std::is_same_v<T, plVec2I32> || std::is_same_v<T, plVec3I32> || std::is_same_v<T, plVec4I32> ||
                  std::is_same_v<T, plVec2U32> || std::is_same_v<T, plVec3U32> || std::is_same_v<T, plVec4U32> ||
                  std::is_same_v<T, plTime> ||
                  std::is_same_v<T, plAngle>)
    {
      out_res = a.Get<T>() + b.Get<T>();
    }
    else if constexpr (std::is_same_v<T, plString> || std::is_same_v<T, plStringView>)
    {
      plStringBuilder s;
      s.Set(a.Get<T>(), b.Get<T>());
      out_res = plString(s.GetView());
    }
    else if constexpr (std::is_same_v<T, plHashedString>)
    {
      plStringBuilder s;
      s.Set(a.Get<T>(), b.Get<T>());

      plHashedString hashedS;
      hashedS.Assign(s);
      out_res = hashedS;
    }
  }
};

plVariant operator+(const plVariant& a, const plVariant& b)
{
  if (a.IsNumber() && b.IsNumber())
  {
    auto biggerType = plMath::Max(a.GetType(), b.GetType());

    AddFunc func;
    plVariant result;
    plVariant::DispatchTo(func, biggerType, a.ConvertTo(biggerType), b.ConvertTo(biggerType), result);
    return result;
  }
  else if (a.GetType() == b.GetType())
  {
    AddFunc func;
    plVariant result;
    plVariant::DispatchTo(func, a.GetType(), a, b, result);
    return result;
  }

  return plVariant();
}

//////////////////////////////////////////////////////////////////////////

struct SubFunc
{
  template <typename T>
  PLASMA_ALWAYS_INLINE void operator()(const plVariant& a, const plVariant& b, plVariant& out_res)
  {
    if constexpr (std::is_same_v<T, plInt8> || std::is_same_v<T, plUInt8> ||
                  std::is_same_v<T, plInt16> || std::is_same_v<T, plUInt16> ||
                  std::is_same_v<T, plInt32> || std::is_same_v<T, plUInt32> ||
                  std::is_same_v<T, plInt64> || std::is_same_v<T, plUInt64> ||
                  std::is_same_v<T, float> || std::is_same_v<T, double> ||
                  std::is_same_v<T, plColor> ||
                  std::is_same_v<T, plVec2> || std::is_same_v<T, plVec3> || std::is_same_v<T, plVec4> ||
                  std::is_same_v<T, plVec2I32> || std::is_same_v<T, plVec3I32> || std::is_same_v<T, plVec4I32> ||
                  std::is_same_v<T, plVec2U32> || std::is_same_v<T, plVec3U32> || std::is_same_v<T, plVec4U32> ||
                  std::is_same_v<T, plTime> ||
                  std::is_same_v<T, plAngle>)
    {
      out_res = a.Get<T>() - b.Get<T>();
    }
  }
};

plVariant operator-(const plVariant& a, const plVariant& b)
{
  if (a.IsNumber() && b.IsNumber())
  {
    auto biggerType = plMath::Max(a.GetType(), b.GetType());

    SubFunc func;
    plVariant result;
    plVariant::DispatchTo(func, biggerType, a.ConvertTo(biggerType), b.ConvertTo(biggerType), result);
    return result;
  }
  else if (a.GetType() == b.GetType())
  {
    SubFunc func;
    plVariant result;
    plVariant::DispatchTo(func, a.GetType(), a, b, result);
    return result;
  }

  return plVariant();
}

//////////////////////////////////////////////////////////////////////////

struct MulFunc
{
  template <typename T>
  PLASMA_ALWAYS_INLINE void operator()(const plVariant& a, const plVariant& b, plVariant& out_res)
  {
    if constexpr (std::is_same_v<T, plInt8> || std::is_same_v<T, plUInt8> ||
                  std::is_same_v<T, plInt16> || std::is_same_v<T, plUInt16> ||
                  std::is_same_v<T, plInt32> || std::is_same_v<T, plUInt32> ||
                  std::is_same_v<T, plInt64> || std::is_same_v<T, plUInt64> ||
                  std::is_same_v<T, float> || std::is_same_v<T, double> ||
                  std::is_same_v<T, plColor> ||
                  std::is_same_v<T, plTime>)
    {
      out_res = a.Get<T>() * b.Get<T>();
    }
    else if constexpr (std::is_same_v<T, plVec2> || std::is_same_v<T, plVec3> || std::is_same_v<T, plVec4> ||
                       std::is_same_v<T, plVec2I32> || std::is_same_v<T, plVec3I32> || std::is_same_v<T, plVec4I32> ||
                       std::is_same_v<T, plVec2U32> || std::is_same_v<T, plVec3U32> || std::is_same_v<T, plVec4U32>)
    {
      out_res = a.Get<T>().CompMul(b.Get<T>());
    }
    else if constexpr (std::is_same_v<T, plAngle>)
    {
      out_res = plAngle(a.Get<T>() * b.Get<T>().GetRadian());
    }
  }
};

plVariant operator*(const plVariant& a, const plVariant& b)
{
  if (a.IsNumber() && b.IsNumber())
  {
    auto biggerType = plMath::Max(a.GetType(), b.GetType());

    MulFunc func;
    plVariant result;
    plVariant::DispatchTo(func, biggerType, a.ConvertTo(biggerType), b.ConvertTo(biggerType), result);
    return result;
  }
  else if (a.GetType() == b.GetType())
  {
    MulFunc func;
    plVariant result;
    plVariant::DispatchTo(func, a.GetType(), a, b, result);
    return result;
  }

  return plVariant();
}

//////////////////////////////////////////////////////////////////////////

struct DivFunc
{
  template <typename T>
  PLASMA_ALWAYS_INLINE void operator()(const plVariant& a, const plVariant& b, plVariant& out_res)
  {
    if constexpr (std::is_same_v<T, plInt8> || std::is_same_v<T, plUInt8> ||
                  std::is_same_v<T, plInt16> || std::is_same_v<T, plUInt16> ||
                  std::is_same_v<T, plInt32> || std::is_same_v<T, plUInt32> ||
                  std::is_same_v<T, plInt64> || std::is_same_v<T, plUInt64> ||
                  std::is_same_v<T, float> || std::is_same_v<T, double> ||
                  std::is_same_v<T, plTime>)
    {
      out_res = a.Get<T>() / b.Get<T>();
    }
    else if constexpr (std::is_same_v<T, plVec2> || std::is_same_v<T, plVec3> || std::is_same_v<T, plVec4> ||
                       std::is_same_v<T, plVec2I32> || std::is_same_v<T, plVec3I32> || std::is_same_v<T, plVec4I32> ||
                       std::is_same_v<T, plVec2U32> || std::is_same_v<T, plVec3U32> || std::is_same_v<T, plVec4U32>)
    {
      out_res = a.Get<T>().CompDiv(b.Get<T>());
    }
    else if constexpr (std::is_same_v<T, plAngle>)
    {
      out_res = plAngle(a.Get<T>() / b.Get<T>().GetRadian());
    }
  }
};

plVariant operator/(const plVariant& a, const plVariant& b)
{
  if (a.IsNumber() && b.IsNumber())
  {
    auto biggerType = plMath::Max(a.GetType(), b.GetType());

    DivFunc func;
    plVariant result;
    plVariant::DispatchTo(func, biggerType, a.ConvertTo(biggerType), b.ConvertTo(biggerType), result);
    return result;
  }
  else if (a.GetType() == b.GetType())
  {
    DivFunc func;
    plVariant result;
    plVariant::DispatchTo(func, a.GetType(), a, b, result);
    return result;
  }

  return plVariant();
}

//////////////////////////////////////////////////////////////////////////

struct LerpFunc
{
  constexpr static bool CanInterpolate(plVariantType::Enum variantType)
  {
    return variantType >= plVariantType::Int8 && variantType <= plVariantType::Vector4;
  }

  template <typename T>
  PLASMA_ALWAYS_INLINE void operator()(const plVariant& a, const plVariant& b, double x, plVariant& out_res)
  {
    if constexpr (std::is_same_v<T, plQuat>)
    {
      plQuat q = plQuat::MakeSlerp(a.Get<plQuat>(), b.Get<plQuat>(), static_cast<float>(x));
      out_res = q;
    }
    else if constexpr (CanInterpolate(static_cast<plVariantType::Enum>(plVariantTypeDeduction<T>::value)))
    {
      out_res = plMath::Lerp(a.Get<T>(), b.Get<T>(), static_cast<float>(x));
    }
    else
    {
      out_res = (x < 0.5) ? a : b;
    }
  }
};

namespace plMath
{
  plVariant Lerp(const plVariant& a, const plVariant& b, double fFactor)
  {
    LerpFunc func;
    plVariant result;
    plVariant::DispatchTo(func, a.GetType(), a, b, fFactor, result);
    return result;
  }
} // namespace plMath

PLASMA_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_Variant);
