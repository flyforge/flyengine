#include <FoundationTest/FoundationTestPCH.h>

#include <FoundationTest/Reflection/ReflectionTestClasses.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plExampleEnum, 1)
  PLASMA_ENUM_CONSTANTS(plExampleEnum::Value1, plExampleEnum::Value2)
  PLASMA_ENUM_CONSTANT(plExampleEnum::Value3),
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_BITFLAGS(plExampleBitflags, 1)
  PLASMA_BITFLAGS_CONSTANTS(plExampleBitflags::Value1, plExampleBitflags::Value2)
  PLASMA_BITFLAGS_CONSTANT(plExampleBitflags::Value3),
PLASMA_END_STATIC_REFLECTED_BITFLAGS;


PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAbstractTestClass, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plAbstractTestStruct, plNoBase, 1, plRTTINoAllocator);
PLASMA_END_STATIC_REFLECTED_TYPE;


PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plTestStruct, plNoBase, 7, plRTTIDefaultAllocator<plTestStruct>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Float", m_fFloat1)->AddAttributes(new plDefaultValueAttribute(1.1f)),
    PLASMA_MEMBER_PROPERTY_READ_ONLY("Vector", m_vProperty3)->AddAttributes(new plDefaultValueAttribute(plVec3(3.0f,4.0f,5.0f))),
    PLASMA_ACCESSOR_PROPERTY("Int", GetInt, SetInt)->AddAttributes(new plDefaultValueAttribute(2)),
    PLASMA_MEMBER_PROPERTY("UInt8", m_UInt8)->AddAttributes(new plDefaultValueAttribute(6)),
    PLASMA_MEMBER_PROPERTY("Variant", m_variant)->AddAttributes(new plDefaultValueAttribute("Test")),
    PLASMA_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new plDefaultValueAttribute(plAngle::Degree(0.5))),
    PLASMA_MEMBER_PROPERTY("DataBuffer", m_DataBuffer)->AddAttributes(new plDefaultValueAttribute(plTestStruct::GetDefaultDataBuffer())),
    PLASMA_MEMBER_PROPERTY("vVec3I", m_vVec3I)->AddAttributes(new plDefaultValueAttribute(plVec3I32(1,2,3))),
    PLASMA_MEMBER_PROPERTY("VarianceAngle", m_VarianceAngle)->AddAttributes(new plDefaultValueAttribute(plVarianceTypeAngle{0.5f, plAngle::Degree(90.0f)})),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plTestStruct3, plNoBase, 71, plRTTIDefaultAllocator<plTestStruct3>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Float", m_fFloat1)->AddAttributes(new plDefaultValueAttribute(33.3f)),
    PLASMA_ACCESSOR_PROPERTY("Int", GetInt, SetInt),
    PLASMA_MEMBER_PROPERTY("UInt8", m_UInt8),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_CONSTRUCTOR_PROPERTY(),
    PLASMA_CONSTRUCTOR_PROPERTY(double, plInt16),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plTypedObjectStruct, plNoBase, 1, plRTTIDefaultAllocator<plTypedObjectStruct>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Float", m_fFloat1)->AddAttributes(new plDefaultValueAttribute(33.3f)),
    PLASMA_MEMBER_PROPERTY("Int", m_iInt32),
    PLASMA_MEMBER_PROPERTY("UInt8", m_UInt8),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTestClass1, 11, plRTTIDefaultAllocator<plTestClass1>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("SubStruct", m_Struct),
    // PLASMA_MEMBER_PROPERTY("MyVector", m_MyVector), Intentionally not reflected
    PLASMA_MEMBER_PROPERTY("Color", m_Color),
    PLASMA_ACCESSOR_PROPERTY_READ_ONLY("SubVector", GetVector)->AddAttributes(new plDefaultValueAttribute(plVec3(3, 4, 5)))
  }
    PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plInt32 plTestClass2Allocator::m_iAllocs = 0;
plInt32 plTestClass2Allocator::m_iDeallocs = 0;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTestClass2, 22, plTestClass2Allocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Text", GetText, SetText)->AddAttributes(new plDefaultValueAttribute("Legen")),
    PLASMA_MEMBER_PROPERTY("Time", m_Time),
    PLASMA_ENUM_MEMBER_PROPERTY("Enum", plExampleEnum, m_enumClass),
    PLASMA_BITFLAGS_MEMBER_PROPERTY("Bitflags", plExampleBitflags, m_bitflagsClass),
    PLASMA_ARRAY_MEMBER_PROPERTY("Array", m_array),
    PLASMA_MEMBER_PROPERTY("Variant", m_Variant),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTestClass2b, 24, plRTTIDefaultAllocator<plTestClass2b>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Text2b", GetText, SetText),
    PLASMA_MEMBER_PROPERTY("SubStruct", m_Struct),
    PLASMA_MEMBER_PROPERTY("Color", m_Color),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTestArrays, 1, plRTTIDefaultAllocator<plTestArrays>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("Hybrid", m_Hybrid),
    PLASMA_ARRAY_MEMBER_PROPERTY("HybridChar", m_HybridChar),
    PLASMA_ARRAY_MEMBER_PROPERTY("Dynamic", m_Dynamic),
    PLASMA_ARRAY_MEMBER_PROPERTY("Deque", m_Deque),
    PLASMA_ARRAY_MEMBER_PROPERTY("Custom", m_CustomVariant),

    PLASMA_ARRAY_MEMBER_PROPERTY_READ_ONLY("HybridRO", m_Hybrid),
    PLASMA_ARRAY_MEMBER_PROPERTY_READ_ONLY("HybridCharRO", m_HybridChar),
    PLASMA_ARRAY_MEMBER_PROPERTY_READ_ONLY("DynamicRO", m_Dynamic),
    PLASMA_ARRAY_MEMBER_PROPERTY_READ_ONLY("DequeRO", m_Deque),
    PLASMA_ARRAY_MEMBER_PROPERTY_READ_ONLY("CustomRO", m_CustomVariant),

    PLASMA_ARRAY_ACCESSOR_PROPERTY("AcHybrid", GetCount, GetValue, SetValue, Insert, Remove),
    PLASMA_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcHybridRO", GetCount, GetValue),
    PLASMA_ARRAY_ACCESSOR_PROPERTY("AcHybridChar", GetCountChar, GetValueChar, SetValueChar, InsertChar, RemoveChar),
    PLASMA_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcHybridCharRO", GetCountChar, GetValueChar),
    PLASMA_ARRAY_ACCESSOR_PROPERTY("AcDynamic", GetCountDyn, GetValueDyn, SetValueDyn, InsertDyn, RemoveDyn),
    PLASMA_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcDynamicRO", GetCountDyn, GetValueDyn),
    PLASMA_ARRAY_ACCESSOR_PROPERTY("AcDeque", GetCountDeq, GetValueDeq, SetValueDeq, InsertDeq, RemoveDeq),
    PLASMA_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcDequeRO", GetCountDeq, GetValueDeq),
    PLASMA_ARRAY_ACCESSOR_PROPERTY("AcCustom", GetCountCustom, GetValueCustom, SetValueCustom, InsertCustom, RemoveCustom),
    PLASMA_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcCustomRO", GetCountCustom, GetValueCustom),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plUInt32 plTestArrays::GetCount() const
{
  return m_Hybrid.GetCount();
}
double plTestArrays::GetValue(plUInt32 uiIndex) const
{
  return m_Hybrid[uiIndex];
}
void plTestArrays::SetValue(plUInt32 uiIndex, double value)
{
  m_Hybrid[uiIndex] = value;
}
void plTestArrays::Insert(plUInt32 uiIndex, double value)
{
  m_Hybrid.Insert(value, uiIndex);
}
void plTestArrays::Remove(plUInt32 uiIndex)
{
  m_Hybrid.RemoveAtAndCopy(uiIndex);
}

plUInt32 plTestArrays::GetCountChar() const
{
  return m_HybridChar.GetCount();
}
const char* plTestArrays::GetValueChar(plUInt32 uiIndex) const
{
  return m_HybridChar[uiIndex];
}
void plTestArrays::SetValueChar(plUInt32 uiIndex, const char* value)
{
  m_HybridChar[uiIndex] = value;
}
void plTestArrays::InsertChar(plUInt32 uiIndex, const char* value)
{
  m_HybridChar.Insert(value, uiIndex);
}
void plTestArrays::RemoveChar(plUInt32 uiIndex)
{
  m_HybridChar.RemoveAtAndCopy(uiIndex);
}

plUInt32 plTestArrays::GetCountDyn() const
{
  return m_Dynamic.GetCount();
}
const plTestStruct3& plTestArrays::GetValueDyn(plUInt32 uiIndex) const
{
  return m_Dynamic[uiIndex];
}
void plTestArrays::SetValueDyn(plUInt32 uiIndex, const plTestStruct3& value)
{
  m_Dynamic[uiIndex] = value;
}
void plTestArrays::InsertDyn(plUInt32 uiIndex, const plTestStruct3& value)
{
  m_Dynamic.Insert(value, uiIndex);
}
void plTestArrays::RemoveDyn(plUInt32 uiIndex)
{
  m_Dynamic.RemoveAtAndCopy(uiIndex);
}

plUInt32 plTestArrays::GetCountDeq() const
{
  return m_Deque.GetCount();
}
const plTestArrays& plTestArrays::GetValueDeq(plUInt32 uiIndex) const
{
  return m_Deque[uiIndex];
}
void plTestArrays::SetValueDeq(plUInt32 uiIndex, const plTestArrays& value)
{
  m_Deque[uiIndex] = value;
}
void plTestArrays::InsertDeq(plUInt32 uiIndex, const plTestArrays& value)
{
  m_Deque.Insert(value, uiIndex);
}
void plTestArrays::RemoveDeq(plUInt32 uiIndex)
{
  m_Deque.RemoveAtAndCopy(uiIndex);
}

plUInt32 plTestArrays::GetCountCustom() const
{
  return m_CustomVariant.GetCount();
}
plVarianceTypeAngle plTestArrays::GetValueCustom(plUInt32 uiIndex) const
{
  return m_CustomVariant[uiIndex];
}
void plTestArrays::SetValueCustom(plUInt32 uiIndex, plVarianceTypeAngle value)
{
  m_CustomVariant[uiIndex] = value;
}
void plTestArrays::InsertCustom(plUInt32 uiIndex, plVarianceTypeAngle value)
{
  m_CustomVariant.Insert(value, uiIndex);
}
void plTestArrays::RemoveCustom(plUInt32 uiIndex)
{
  m_CustomVariant.RemoveAtAndCopy(uiIndex);
}

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTestSets, 1, plRTTIDefaultAllocator<plTestSets>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_SET_MEMBER_PROPERTY("Set", m_SetMember),
    PLASMA_SET_MEMBER_PROPERTY_READ_ONLY("SetRO", m_SetMember),
    PLASMA_SET_ACCESSOR_PROPERTY("AcSet", GetSet, Insert, Remove),
    PLASMA_SET_ACCESSOR_PROPERTY_READ_ONLY("AcSetRO", GetSet),
    PLASMA_SET_MEMBER_PROPERTY("HashSet", m_HashSetMember),
    PLASMA_SET_MEMBER_PROPERTY_READ_ONLY("HashSetRO", m_HashSetMember),
    PLASMA_SET_ACCESSOR_PROPERTY("HashAcSet", GetHashSet, HashInsert, HashRemove),
    PLASMA_SET_ACCESSOR_PROPERTY_READ_ONLY("HashAcSetRO", GetHashSet),
    PLASMA_SET_ACCESSOR_PROPERTY("AcPseudoSet", GetPseudoSet, PseudoInsert, PseudoRemove),
    PLASMA_SET_ACCESSOR_PROPERTY_READ_ONLY("AcPseudoSetRO", GetPseudoSet),
    PLASMA_SET_ACCESSOR_PROPERTY("AcPseudoSet2", GetPseudoSet2, PseudoInsert2, PseudoRemove2),
    PLASMA_SET_ACCESSOR_PROPERTY_READ_ONLY("AcPseudoSet2RO", GetPseudoSet2),
    PLASMA_SET_ACCESSOR_PROPERTY("AcPseudoSet2b", GetPseudoSet2, PseudoInsert2b, PseudoRemove2b),
    PLASMA_SET_MEMBER_PROPERTY("CustomHashSet", m_CustomVariant),
    PLASMA_SET_MEMBER_PROPERTY_READ_ONLY("CustomHashSetRO", m_CustomVariant),
    PLASMA_SET_ACCESSOR_PROPERTY("CustomHashAcSet", GetCustomHashSet, CustomHashInsert, CustomHashRemove),
    PLASMA_SET_ACCESSOR_PROPERTY_READ_ONLY("CustomHashAcSetRO", GetCustomHashSet),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const plSet<double>& plTestSets::GetSet() const
{
  return m_SetAccessor;
}

void plTestSets::Insert(double value)
{
  m_SetAccessor.Insert(value);
}

void plTestSets::Remove(double value)
{
  m_SetAccessor.Remove(value);
}


const plHashSet<plInt64>& plTestSets::GetHashSet() const
{
  return m_HashSetAccessor;
}

void plTestSets::HashInsert(plInt64 value)
{
  m_HashSetAccessor.Insert(value);
}

void plTestSets::HashRemove(plInt64 value)
{
  m_HashSetAccessor.Remove(value);
}

const plDeque<int>& plTestSets::GetPseudoSet() const
{
  return m_Deque;
}

void plTestSets::PseudoInsert(int value)
{
  if (!m_Deque.Contains(value))
    m_Deque.PushBack(value);
}

void plTestSets::PseudoRemove(int value)
{
  m_Deque.RemoveAndCopy(value);
}


plArrayPtr<const plString> plTestSets::GetPseudoSet2() const
{
  return m_Array;
}

void plTestSets::PseudoInsert2(const plString& value)
{
  if (!m_Array.Contains(value))
    m_Array.PushBack(value);
}

void plTestSets::PseudoRemove2(const plString& value)
{
  m_Array.RemoveAndCopy(value);
}

void plTestSets::PseudoInsert2b(const char* value)
{
  if (!m_Array.Contains(value))
    m_Array.PushBack(value);
}

void plTestSets::PseudoRemove2b(const char* value)
{
  m_Array.RemoveAndCopy(value);
}

const plHashSet<plVarianceTypeAngle>& plTestSets::GetCustomHashSet() const
{
  return m_CustomVariant;
}

void plTestSets::CustomHashInsert(plVarianceTypeAngle value)
{
  m_CustomVariant.Insert(value);
}

void plTestSets::CustomHashRemove(plVarianceTypeAngle value)
{
  m_CustomVariant.Remove(value);
}

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTestMaps, 1, plRTTIDefaultAllocator<plTestMaps>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MAP_MEMBER_PROPERTY("Map", m_MapMember),
    PLASMA_MAP_MEMBER_PROPERTY_READ_ONLY("MapRO", m_MapMember),
    PLASMA_MAP_WRITE_ACCESSOR_PROPERTY("AcMap", GetContainer, Insert, Remove),
    PLASMA_MAP_MEMBER_PROPERTY("HashTable", m_HashTableMember),
    PLASMA_MAP_MEMBER_PROPERTY_READ_ONLY("HashTableRO", m_HashTableMember),
    PLASMA_MAP_WRITE_ACCESSOR_PROPERTY("AcHashTable", GetContainer2, Insert2, Remove2),
    PLASMA_MAP_ACCESSOR_PROPERTY("Accessor", GetKeys3, GetValue3, Insert3, Remove3),
    PLASMA_MAP_ACCESSOR_PROPERTY_READ_ONLY("AccessorRO", GetKeys3, GetValue3),
    PLASMA_MAP_MEMBER_PROPERTY("CustomVariant", m_CustomVariant),
    PLASMA_MAP_MEMBER_PROPERTY_READ_ONLY("CustomVariantRO", m_CustomVariant),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool plTestMaps::operator==(const plTestMaps& rhs) const
{
  for (plUInt32 i = 0; i < m_Accessor3.GetCount(); i++)
  {
    bool bRes = false;
    for (plUInt32 j = 0; j < rhs.m_Accessor3.GetCount(); j++)
    {
      if (m_Accessor3[i].m_Key == rhs.m_Accessor3[j].m_Key)
      {
        if (m_Accessor3[i].m_Value == rhs.m_Accessor3[j].m_Value)
          bRes = true;
      }
    }
    if (!bRes)
      return false;
  }
  return m_MapMember == rhs.m_MapMember && m_MapAccessor == rhs.m_MapAccessor && m_HashTableMember == rhs.m_HashTableMember && m_HashTableAccessor == rhs.m_HashTableAccessor && m_CustomVariant == rhs.m_CustomVariant;
}

const plMap<plString, plInt64>& plTestMaps::GetContainer() const
{
  return m_MapAccessor;
}

void plTestMaps::Insert(const char* szKey, plInt64 value)
{
  m_MapAccessor.Insert(szKey, value);
}

void plTestMaps::Remove(const char* szKey)
{
  m_MapAccessor.Remove(szKey);
}

const plHashTable<plString, plString>& plTestMaps::GetContainer2() const
{
  return m_HashTableAccessor;
}

void plTestMaps::Insert2(const char* szKey, const plString& value)
{
  m_HashTableAccessor.Insert(szKey, value);
}


void plTestMaps::Remove2(const char* szKey)
{
  m_HashTableAccessor.Remove(szKey);
}

const plRangeView<const char*, plUInt32> plTestMaps::GetKeys3() const
{
  return plRangeView<const char*, plUInt32>([this]() -> plUInt32 { return 0; }, [this]() -> plUInt32 { return m_Accessor3.GetCount(); }, [this](plUInt32& ref_uiIt) { ++ref_uiIt; }, [this](const plUInt32& uiIt) -> const char* { return m_Accessor3[uiIt].m_Key; });
}

void plTestMaps::Insert3(const char* szKey, const plVariant& value)
{
  for (auto&& t : m_Accessor3)
  {
    if (t.m_Key == szKey)
    {
      t.m_Value = value;
      return;
    }
  }
  auto&& t = m_Accessor3.ExpandAndGetRef();
  t.m_Key = szKey;
  t.m_Value = value;
}

void plTestMaps::Remove3(const char* szKey)
{
  for (plUInt32 i = 0; i < m_Accessor3.GetCount(); i++)
  {
    const Tuple& t = m_Accessor3[i];
    if (t.m_Key == szKey)
    {
      m_Accessor3.RemoveAtAndSwap(i);
      break;
    }
  }
}

bool plTestMaps::GetValue3(const char* szKey, plVariant& out_value) const
{
  for (const auto& t : m_Accessor3)
  {
    if (t.m_Key == szKey)
    {
      out_value = t.m_Value;
      return true;
    }
  }
  return false;
}

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTestPtr, 1, plRTTIDefaultAllocator<plTestPtr>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("ConstCharPtr", GetString, SetString),
    PLASMA_ACCESSOR_PROPERTY("ArraysPtr", GetArrays, SetArrays)->AddFlags(plPropertyFlags::PointerOwner),
    PLASMA_MEMBER_PROPERTY("ArraysPtrDirect", m_pArraysDirect)->AddFlags(plPropertyFlags::PointerOwner),
    PLASMA_ARRAY_MEMBER_PROPERTY("PtrArray", m_ArrayPtr)->AddFlags(plPropertyFlags::PointerOwner),
    PLASMA_SET_MEMBER_PROPERTY("PtrSet", m_SetPtr)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plTestEnumStruct, plNoBase, 1, plRTTIDefaultAllocator<plTestEnumStruct>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("m_enum", plExampleEnum, m_enum),
    PLASMA_ENUM_MEMBER_PROPERTY("m_enumClass", plExampleEnum, m_enumClass),
    PLASMA_ENUM_ACCESSOR_PROPERTY("m_enum2", plExampleEnum, GetEnum, SetEnum),
    PLASMA_ENUM_ACCESSOR_PROPERTY("m_enumClass2", plExampleEnum,  GetEnumClass, SetEnumClass),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plTestBitflagsStruct, plNoBase, 1, plRTTIDefaultAllocator<plTestBitflagsStruct>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_BITFLAGS_MEMBER_PROPERTY("m_bitflagsClass", plExampleBitflags, m_bitflagsClass),
    PLASMA_BITFLAGS_ACCESSOR_PROPERTY("m_bitflagsClass2", plExampleBitflags, GetBitflagsClass, SetBitflagsClass),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on
