#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/VarianceTypes.h>

struct plExampleEnum
{
  using StorageType = plInt8;
  enum Enum
  {
    Value1 = 1,      // normal value
    Value2 = -2,     // normal value
    Value3 = 4,      // normal value
    Default = Value1 // Default initialization value (required)
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plExampleEnum);


struct plExampleBitflags
{
  using StorageType = plUInt64;
  enum Enum : plUInt64
  {
    Value1 = PLASMA_BIT(0),  // normal value
    Value2 = PLASMA_BIT(31), // normal value
    Value3 = PLASMA_BIT(63), // normal value
    Default = Value1     // Default initialization value (required)
  };

  struct Bits
  {
    StorageType Value1 : 1;
    StorageType Padding : 30;
    StorageType Value2 : 1;
    StorageType Padding2 : 31;
    StorageType Value3 : 1;
  };
};

PLASMA_DECLARE_FLAGS_OPERATORS(plExampleBitflags);

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plExampleBitflags);


class plAbstractTestClass : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAbstractTestClass, plReflectedClass);

  virtual void AbstractFunction() = 0;
};


struct plAbstractTestStruct
{
  virtual void AbstractFunction() = 0;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plAbstractTestStruct);


struct plTestStruct
{
  PLASMA_ALLOW_PRIVATE_PROPERTIES(plTestStruct);

public:
  static plDataBuffer GetDefaultDataBuffer()
  {
    plDataBuffer data;
    data.PushBack(255);
    data.PushBack(0);
    data.PushBack(127);
    return data;
  }

  plTestStruct()
  {
    m_fFloat1 = 1.1f;
    m_iInt2 = 2;
    m_vProperty3.Set(3, 4, 5);
    m_UInt8 = 6;
    m_variant = "Test";
    m_Angle = plAngle::Degree(0.5);
    m_DataBuffer = GetDefaultDataBuffer();
    m_vVec3I = plVec3I32(1, 2, 3);
    m_VarianceAngle.m_fVariance = 0.5f;
    m_VarianceAngle.m_Value = plAngle::Degree(90.0f);
  }



  bool operator==(const plTestStruct& rhs) const
  {
    return m_fFloat1 == rhs.m_fFloat1 && m_UInt8 == rhs.m_UInt8 && m_variant == rhs.m_variant && m_iInt2 == rhs.m_iInt2 && m_vProperty3 == rhs.m_vProperty3 && m_Angle == rhs.m_Angle && m_DataBuffer == rhs.m_DataBuffer && m_vVec3I == rhs.m_vVec3I && m_VarianceAngle == rhs.m_VarianceAngle;
  }

  float m_fFloat1;
  plUInt8 m_UInt8;
  plVariant m_variant;
  plAngle m_Angle;
  plDataBuffer m_DataBuffer;
  plVec3I32 m_vVec3I;
  plVarianceTypeAngle m_VarianceAngle;

private:
  void SetInt(plInt32 i) { m_iInt2 = i; }
  plInt32 GetInt() const { return m_iInt2; }

  plInt32 m_iInt2;
  plVec3 m_vProperty3;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plTestStruct);


struct plTestStruct3
{
  PLASMA_ALLOW_PRIVATE_PROPERTIES(plTestStruct3);

public:
  plTestStruct3()
  {
    m_fFloat1 = 1.1f;
    m_UInt8 = 6;
    m_iInt32 = 2;
  }
  plTestStruct3(double a, plInt16 b)
  {
    m_fFloat1 = a;
    m_UInt8 = b;
    m_iInt32 = 32;
  }

  bool operator==(const plTestStruct3& rhs) const { return m_fFloat1 == rhs.m_fFloat1 && m_iInt32 == rhs.m_iInt32 && m_UInt8 == rhs.m_UInt8; }

  bool operator!=(const plTestStruct3& rhs) const { return !(*this == rhs); }

  double m_fFloat1;
  plInt16 m_UInt8;

  plUInt32 GetIntPublic() const { return m_iInt32; }

private:
  void SetInt(plUInt32 i) { m_iInt32 = i; }
  plUInt32 GetInt() const { return m_iInt32; }

  plInt32 m_iInt32;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plTestStruct3);

struct plTypedObjectStruct
{
  PLASMA_ALLOW_PRIVATE_PROPERTIES(plTypedObjectStruct);

public:
  plTypedObjectStruct()
  {
    m_fFloat1 = 1.1f;
    m_UInt8 = 6;
    m_iInt32 = 2;
  }
  plTypedObjectStruct(double a, plInt16 b)
  {
    m_fFloat1 = a;
    m_UInt8 = b;
    m_iInt32 = 32;
  }

  double m_fFloat1;
  plInt16 m_UInt8;
  plInt32 m_iInt32;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plTypedObjectStruct);
PLASMA_DECLARE_CUSTOM_VARIANT_TYPE(plTypedObjectStruct);

class plTestClass1 : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTestClass1, plReflectedClass);

public:
  plTestClass1()
  {
    m_MyVector.Set(3, 4, 5);

    m_Struct.m_fFloat1 = 33.3f;

    m_Color = plColor::CornflowerBlue; // The Original!
  }

  plTestClass1(const plColor& c, const plTestStruct& s)
  {
    m_Color = c;
    m_Struct = s;
    m_MyVector.Set(1, 2, 3);
  }

  bool operator==(const plTestClass1& rhs) const { return m_Struct == rhs.m_Struct && m_MyVector == rhs.m_MyVector && m_Color == rhs.m_Color; }

  plVec3 GetVector() const { return m_MyVector; }

  plTestStruct m_Struct;
  plVec3 m_MyVector;
  plColor m_Color;
};


class plTestClass2 : public plTestClass1
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTestClass2, plTestClass1);

public:
  plTestClass2() { m_sText = "Legen"; }

  bool operator==(const plTestClass2& rhs) const { return m_Time == rhs.m_Time && m_enumClass == rhs.m_enumClass && m_bitflagsClass == rhs.m_bitflagsClass && m_array == rhs.m_array && m_Variant == rhs.m_Variant && m_sText == rhs.m_sText; }

  const char* GetText() const { return m_sText.GetData(); }
  void SetText(const char* szSz) { m_sText = szSz; }

  plTime m_Time;
  plEnum<plExampleEnum> m_enumClass;
  plBitflags<plExampleBitflags> m_bitflagsClass;
  plHybridArray<float, 4> m_array;
  plVariant m_Variant;

private:
  plString m_sText;
};


struct plTestClass2Allocator : public plRTTIAllocator
{
  virtual plInternal::NewInstance<void> AllocateInternal(plAllocatorBase* pAllocator) override
  {
    ++m_iAllocs;

    return PLASMA_DEFAULT_NEW(plTestClass2);
  }

  virtual void Deallocate(void* pObject, plAllocatorBase* pAllocator) override
  {
    ++m_iDeallocs;

    plTestClass2* pPointer = (plTestClass2*)pObject;
    PLASMA_DEFAULT_DELETE(pPointer);
  }

  static plInt32 m_iAllocs;
  static plInt32 m_iDeallocs;
};


class plTestClass2b : plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTestClass2b, plReflectedClass);

public:
  plTestClass2b() { m_sText = "Tut"; }

  const char* GetText() const { return m_sText.GetData(); }
  void SetText(const char* szSz) { m_sText = szSz; }

  plTestStruct3 m_Struct;
  plColor m_Color;

private:
  plString m_sText;
};


class plTestArrays : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTestArrays, plReflectedClass);

public:
  plTestArrays() = default;

  bool operator==(const plTestArrays& rhs) const
  {
    return m_Hybrid == rhs.m_Hybrid && m_Dynamic == rhs.m_Dynamic && m_Deque == rhs.m_Deque && m_HybridChar == rhs.m_HybridChar && m_CustomVariant == rhs.m_CustomVariant;
  }

  bool operator!=(const plTestArrays& rhs) const { return !(*this == rhs); }

  plUInt32 GetCount() const;
  double GetValue(plUInt32 uiIndex) const;
  void SetValue(plUInt32 uiIndex, double value);
  void Insert(plUInt32 uiIndex, double value);
  void Remove(plUInt32 uiIndex);

  plUInt32 GetCountChar() const;
  const char* GetValueChar(plUInt32 uiIndex) const;
  void SetValueChar(plUInt32 uiIndex, const char* value);
  void InsertChar(plUInt32 uiIndex, const char* value);
  void RemoveChar(plUInt32 uiIndex);

  plUInt32 GetCountDyn() const;
  const plTestStruct3& GetValueDyn(plUInt32 uiIndex) const;
  void SetValueDyn(plUInt32 uiIndex, const plTestStruct3& value);
  void InsertDyn(plUInt32 uiIndex, const plTestStruct3& value);
  void RemoveDyn(plUInt32 uiIndex);

  plUInt32 GetCountDeq() const;
  const plTestArrays& GetValueDeq(plUInt32 uiIndex) const;
  void SetValueDeq(plUInt32 uiIndex, const plTestArrays& value);
  void InsertDeq(plUInt32 uiIndex, const plTestArrays& value);
  void RemoveDeq(plUInt32 uiIndex);

  plUInt32 GetCountCustom() const;
  plVarianceTypeAngle GetValueCustom(plUInt32 uiIndex) const;
  void SetValueCustom(plUInt32 uiIndex, plVarianceTypeAngle value);
  void InsertCustom(plUInt32 uiIndex, plVarianceTypeAngle value);
  void RemoveCustom(plUInt32 uiIndex);

  plHybridArray<double, 5> m_Hybrid;
  plHybridArray<plString, 2> m_HybridChar;
  plDynamicArray<plTestStruct3> m_Dynamic;
  plDeque<plTestArrays> m_Deque;
  plHybridArray<plVarianceTypeAngle, 1> m_CustomVariant;
};


class plTestSets : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTestSets, plReflectedClass);

public:
  plTestSets() = default;

  bool operator==(const plTestSets& rhs) const
  {
    return m_SetMember == rhs.m_SetMember && m_SetAccessor == rhs.m_SetAccessor && m_Deque == rhs.m_Deque && m_Array == rhs.m_Array && m_CustomVariant == rhs.m_CustomVariant;
  }

  bool operator!=(const plTestSets& rhs) const { return !(*this == rhs); }

  const plSet<double>& GetSet() const;
  void Insert(double value);
  void Remove(double value);

  const plHashSet<plInt64>& GetHashSet() const;
  void HashInsert(plInt64 value);
  void HashRemove(plInt64 value);

  const plDeque<int>& GetPseudoSet() const;
  void PseudoInsert(int value);
  void PseudoRemove(int value);

  plArrayPtr<const plString> GetPseudoSet2() const;
  void PseudoInsert2(const plString& value);
  void PseudoRemove2(const plString& value);

  void PseudoInsert2b(const char* value);
  void PseudoRemove2b(const char* value);

  const plHashSet<plVarianceTypeAngle>& GetCustomHashSet() const;
  void CustomHashInsert(plVarianceTypeAngle value);
  void CustomHashRemove(plVarianceTypeAngle value);

  plSet<plInt8> m_SetMember;
  plSet<double> m_SetAccessor;

  plHashSet<plInt32> m_HashSetMember;
  plHashSet<plInt64> m_HashSetAccessor;

  plDeque<int> m_Deque;
  plDynamicArray<plString> m_Array;
  plHashSet<plVarianceTypeAngle> m_CustomVariant;
};


class plTestMaps : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTestMaps, plReflectedClass);

public:
  plTestMaps() = default;

  bool operator==(const plTestMaps& rhs) const;

  const plMap<plString, plInt64>& GetContainer() const;
  void Insert(const char* szKey, plInt64 value);
  void Remove(const char* szKey);

  const plHashTable<plString, plString>& GetContainer2() const;
  void Insert2(const char* szKey, const plString& value);
  void Remove2(const char* szKey);

  const plRangeView<const char*, plUInt32> GetKeys3() const;
  void Insert3(const char* szKey, const plVariant& value);
  void Remove3(const char* szKey);
  bool GetValue3(const char* szKey, plVariant& out_value) const;

  plMap<plString, int> m_MapMember;
  plMap<plString, plInt64> m_MapAccessor;

  plHashTable<plString, double> m_HashTableMember;
  plHashTable<plString, plString> m_HashTableAccessor;

  plMap<plString, plVarianceTypeAngle> m_CustomVariant;

  struct Tuple
  {
    plString m_Key;
    plVariant m_Value;
  };
  plHybridArray<Tuple, 2> m_Accessor3;
};

class plTestPtr : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTestPtr, plReflectedClass);

public:
  plTestPtr()
  {
    m_pArrays = nullptr;
    m_pArraysDirect = nullptr;
  }

  ~plTestPtr()
  {
    PLASMA_DEFAULT_DELETE(m_pArrays);
    PLASMA_DEFAULT_DELETE(m_pArraysDirect);
    for (auto ptr : m_ArrayPtr)
    {
      PLASMA_DEFAULT_DELETE(ptr);
    }
    m_ArrayPtr.Clear();
    for (auto ptr : m_SetPtr)
    {
      PLASMA_DEFAULT_DELETE(ptr);
    }
    m_SetPtr.Clear();
  }

  bool operator==(const plTestPtr& rhs) const
  {
    if (m_sString != rhs.m_sString || (m_pArrays != rhs.m_pArrays && *m_pArrays != *rhs.m_pArrays))
      return false;

    if (m_ArrayPtr.GetCount() != rhs.m_ArrayPtr.GetCount())
      return false;

    for (plUInt32 i = 0; i < m_ArrayPtr.GetCount(); i++)
    {
      if (!(*m_ArrayPtr[i] == *rhs.m_ArrayPtr[i]))
        return false;
    }

    // only works for the test data if the test.
    if (m_SetPtr.IsEmpty() && rhs.m_SetPtr.IsEmpty())
      return true;

    if (m_SetPtr.GetCount() != 1 || rhs.m_SetPtr.GetCount() != 1)
      return true;

    return *m_SetPtr.GetIterator().Key() == *rhs.m_SetPtr.GetIterator().Key();
  }

  void SetString(const char* szValue) { m_sString = szValue; }
  const char* GetString() const { return m_sString; }

  void SetArrays(plTestArrays* pValue) { m_pArrays = pValue; }
  plTestArrays* GetArrays() const { return m_pArrays; }


  plString m_sString;
  plTestArrays* m_pArrays;
  plTestArrays* m_pArraysDirect;
  plDeque<plTestArrays*> m_ArrayPtr;
  plSet<plTestSets*> m_SetPtr;
};


struct plTestEnumStruct
{
  PLASMA_ALLOW_PRIVATE_PROPERTIES(plTestEnumStruct);

public:
  plTestEnumStruct()
  {
    m_enum = plExampleEnum::Value1;
    m_enumClass = plExampleEnum::Value1;
    m_Enum2 = plExampleEnum::Value1;
    m_EnumClass2 = plExampleEnum::Value1;
  }

  bool operator==(const plTestEnumStruct& rhs) const { return m_Enum2 == rhs.m_Enum2 && m_enum == rhs.m_enum && m_enumClass == rhs.m_enumClass && m_EnumClass2 == rhs.m_EnumClass2; }

  plExampleEnum::Enum m_enum;
  plEnum<plExampleEnum> m_enumClass;

  void SetEnum(plExampleEnum::Enum e) { m_Enum2 = e; }
  plExampleEnum::Enum GetEnum() const { return m_Enum2; }
  void SetEnumClass(plEnum<plExampleEnum> e) { m_EnumClass2 = e; }
  plEnum<plExampleEnum> GetEnumClass() const { return m_EnumClass2; }

private:
  plExampleEnum::Enum m_Enum2;
  plEnum<plExampleEnum> m_EnumClass2;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plTestEnumStruct);


struct plTestBitflagsStruct
{
  PLASMA_ALLOW_PRIVATE_PROPERTIES(plTestBitflagsStruct);

public:
  plTestBitflagsStruct()
  {
    m_bitflagsClass = plExampleBitflags::Value1;
    m_BitflagsClass2 = plExampleBitflags::Value1;
  }

  bool operator==(const plTestBitflagsStruct& rhs) const { return m_bitflagsClass == rhs.m_bitflagsClass && m_BitflagsClass2 == rhs.m_BitflagsClass2; }

  plBitflags<plExampleBitflags> m_bitflagsClass;

  void SetBitflagsClass(plBitflags<plExampleBitflags> e) { m_BitflagsClass2 = e; }
  plBitflags<plExampleBitflags> GetBitflagsClass() const { return m_BitflagsClass2; }

private:
  plBitflags<plExampleBitflags> m_BitflagsClass2;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plTestBitflagsStruct);
