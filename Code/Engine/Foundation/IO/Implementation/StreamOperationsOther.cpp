#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// plAllocatorBase::Stats

void operator<<(plStreamWriter& inout_stream, const plAllocatorBase::Stats& rhs)
{
  inout_stream << rhs.m_uiNumAllocations;
  inout_stream << rhs.m_uiNumDeallocations;
  inout_stream << rhs.m_uiAllocationSize;
}

void operator>>(plStreamReader& inout_stream, plAllocatorBase::Stats& rhs)
{
  inout_stream >> rhs.m_uiNumAllocations;
  inout_stream >> rhs.m_uiNumDeallocations;
  inout_stream >> rhs.m_uiAllocationSize;
}

// plTime

void operator<<(plStreamWriter& inout_stream, plTime value)
{
  inout_stream << value.GetSeconds();
}

void operator>>(plStreamReader& inout_stream, plTime& ref_value)
{
  double d = 0;
  inout_stream.ReadQWordValue(&d).IgnoreResult();

  ref_value = plTime::Seconds(d);
}

// plUuid

void operator<<(plStreamWriter& inout_stream, const plUuid& value)
{
  inout_stream << value.m_uiHigh;
  inout_stream << value.m_uiLow;
}

void operator>>(plStreamReader& inout_stream, plUuid& ref_value)
{
  inout_stream >> ref_value.m_uiHigh;
  inout_stream >> ref_value.m_uiLow;
}

// plHashedString

void operator<<(plStreamWriter& inout_stream, const plHashedString& sValue)
{
  inout_stream.WriteString(sValue.GetView()).AssertSuccess();
}

void operator>>(plStreamReader& inout_stream, plHashedString& ref_sValue)
{
  plStringBuilder sTemp;
  inout_stream >> sTemp;
  ref_sValue.Assign(sTemp);
}

// plTempHashedString

void operator<<(plStreamWriter& inout_stream, const plTempHashedString& sValue)
{
  inout_stream << (plUInt64)sValue.GetHash();
}

void operator>>(plStreamReader& inout_stream, plTempHashedString& ref_sValue)
{
  plUInt64 hash;
  inout_stream >> hash;
  ref_sValue = plTempHashedString(hash);
}

// plVariant

struct WriteValueFunc
{
  template <typename T>
  PLASMA_ALWAYS_INLINE void operator()()
  {
    (*m_pStream) << m_pValue->Get<T>();
  }

  plStreamWriter* m_pStream;
  const plVariant* m_pValue;
};

template <>
PLASMA_FORCE_INLINE void WriteValueFunc::operator()<plVariantArray>()
{
  const plVariantArray& values = m_pValue->Get<plVariantArray>();
  const plUInt32 iCount = values.GetCount();
  (*m_pStream) << iCount;
  for (plUInt32 i = 0; i < iCount; i++)
  {
    (*m_pStream) << values[i];
  }
}

template <>
PLASMA_FORCE_INLINE void WriteValueFunc::operator()<plVariantDictionary>()
{
  const plVariantDictionary& values = m_pValue->Get<plVariantDictionary>();
  const plUInt32 iCount = values.GetCount();
  (*m_pStream) << iCount;
  for (auto it = values.GetIterator(); it.IsValid(); ++it)
  {
    (*m_pStream) << it.Key();
    (*m_pStream) << it.Value();
  }
}

template <>
inline void WriteValueFunc::operator()<plTypedPointer>()
{
  PLASMA_REPORT_FAILURE("Type 'plReflectedClass*' not supported in serialization.");
}

template <>
inline void WriteValueFunc::operator()<plTypedObject>()
{
  plTypedObject obj = m_pValue->Get<plTypedObject>();
  if (const plVariantTypeInfo* pTypeInfo = plVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(obj.m_pType))
  {
    (*m_pStream) << obj.m_pType->GetTypeName();
    pTypeInfo->Serialize(*m_pStream, obj.m_pObject);
  }
  else
  {
    PLASMA_REPORT_FAILURE("The type '{0}' was declared but not defined, add PLASMA_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", obj.m_pType->GetTypeName());
  }
}

template <>
PLASMA_FORCE_INLINE void WriteValueFunc::operator()<plStringView>()
{
  plStringBuilder s = m_pValue->Get<plStringView>();
  (*m_pStream) << s;
}

template <>
PLASMA_FORCE_INLINE void WriteValueFunc::operator()<plDataBuffer>()
{
  const plDataBuffer& data = m_pValue->Get<plDataBuffer>();
  const plUInt32 iCount = data.GetCount();
  (*m_pStream) << iCount;
  m_pStream->WriteBytes(data.GetData(), data.GetCount()).AssertSuccess();
}

struct ReadValueFunc
{
  template <typename T>
  PLASMA_FORCE_INLINE void operator()()
  {
    T value;
    (*m_pStream) >> value;
    *m_pValue = value;
  }

  plStreamReader* m_pStream;
  plVariant* m_pValue;
};

template <>
PLASMA_FORCE_INLINE void ReadValueFunc::operator()<plVariantArray>()
{
  plVariantArray values;
  plUInt32 iCount;
  (*m_pStream) >> iCount;
  values.SetCount(iCount);
  for (plUInt32 i = 0; i < iCount; i++)
  {
    (*m_pStream) >> values[i];
  }
  *m_pValue = values;
}

template <>
PLASMA_FORCE_INLINE void ReadValueFunc::operator()<plVariantDictionary>()
{
  plVariantDictionary values;
  plUInt32 iCount;
  (*m_pStream) >> iCount;
  for (plUInt32 i = 0; i < iCount; i++)
  {
    plString key;
    plVariant value;
    (*m_pStream) >> key;
    (*m_pStream) >> value;
    values.Insert(key, value);
  }
  *m_pValue = values;
}

template <>
inline void ReadValueFunc::operator()<plTypedPointer>()
{
  PLASMA_REPORT_FAILURE("Type 'plTypedPointer' not supported in serialization.");
}

template <>
inline void ReadValueFunc::operator()<plTypedObject>()
{
  plStringBuilder sType;
  (*m_pStream) >> sType;
  const plRTTI* pType = plRTTI::FindTypeByName(sType);
  PLASMA_ASSERT_DEV(pType, "The type '{0}' could not be found.", sType);
  const plVariantTypeInfo* pTypeInfo = plVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pType);
  PLASMA_ASSERT_DEV(pTypeInfo, "The type '{0}' was declared but not defined, add PLASMA_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", sType);
  PLASMA_MSVC_ANALYSIS_ASSUME(pType != nullptr);
  PLASMA_MSVC_ANALYSIS_ASSUME(pTypeInfo != nullptr);
  void* pObject = pType->GetAllocator()->Allocate<void>();
  pTypeInfo->Deserialize(*m_pStream, pObject);
  m_pValue->MoveTypedObject(pObject, pType);
}

template <>
inline void ReadValueFunc::operator()<plStringView>()
{
  PLASMA_REPORT_FAILURE("Type 'plStringView' not supported in serialization.");
}

template <>
PLASMA_FORCE_INLINE void ReadValueFunc::operator()<plDataBuffer>()
{
  plDataBuffer data;
  plUInt32 iCount;
  (*m_pStream) >> iCount;
  data.SetCountUninitialized(iCount);

  m_pStream->ReadBytes(data.GetData(), iCount);
  *m_pValue = data;
}

void operator<<(plStreamWriter& inout_stream, const plVariant& value)
{
  plUInt8 variantVersion = (plUInt8)plGetStaticRTTI<plVariant>()->GetTypeVersion();
  inout_stream << variantVersion;
  plVariant::Type::Enum type = value.GetType();
  plUInt8 typeStorage = type;
  if (typeStorage == plVariantType::StringView)
    typeStorage = plVariantType::String;
  inout_stream << typeStorage;

  if (type != plVariant::Type::Invalid)
  {
    WriteValueFunc func;
    func.m_pStream = &inout_stream;
    func.m_pValue = &value;

    plVariant::DispatchTo(func, type);
  }
}

void operator>>(plStreamReader& inout_stream, plVariant& ref_value)
{
  plUInt8 variantVersion;
  inout_stream >> variantVersion;
  PLASMA_ASSERT_DEBUG(plGetStaticRTTI<plVariant>()->GetTypeVersion() == variantVersion, "Older variant serialization not supported!");

  plUInt8 typeStorage;
  inout_stream >> typeStorage;
  plVariant::Type::Enum type = (plVariant::Type::Enum)typeStorage;

  if (type != plVariant::Type::Invalid)
  {
    ReadValueFunc func;
    func.m_pStream = &inout_stream;
    func.m_pValue = &ref_value;

    plVariant::DispatchTo(func, type);
  }
  else
  {
    ref_value = plVariant();
  }
}

// plTimestamp

void operator<<(plStreamWriter& inout_stream, plTimestamp value)
{
  inout_stream << value.GetInt64(plSIUnitOfTime::Microsecond);
}

void operator>>(plStreamReader& inout_stream, plTimestamp& ref_value)
{
  plInt64 value;
  inout_stream >> value;

  ref_value.SetInt64(value, plSIUnitOfTime::Microsecond);
}

// plVarianceTypeFloat

void operator<<(plStreamWriter& inout_stream, const plVarianceTypeFloat& value)
{
  inout_stream << value.m_fVariance;
  inout_stream << value.m_Value;
}
void operator>>(plStreamReader& inout_stream, plVarianceTypeFloat& ref_value)
{
  inout_stream >> ref_value.m_fVariance;
  inout_stream >> ref_value.m_Value;
}

// plVarianceTypeTime

void operator<<(plStreamWriter& inout_stream, const plVarianceTypeTime& value)
{
  inout_stream << value.m_fVariance;
  inout_stream << value.m_Value;
}
void operator>>(plStreamReader& inout_stream, plVarianceTypeTime& ref_value)
{
  inout_stream >> ref_value.m_fVariance;
  inout_stream >> ref_value.m_Value;
}

// plVarianceTypeAngle

void operator<<(plStreamWriter& inout_stream, const plVarianceTypeAngle& value)
{
  inout_stream << value.m_fVariance;
  inout_stream << value.m_Value;
}
void operator>>(plStreamReader& inout_stream, plVarianceTypeAngle& ref_value)
{
  inout_stream >> ref_value.m_fVariance;
  inout_stream >> ref_value.m_Value;
}
PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamOperationsOther);
