#pragma once

#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>

struct plIntegerStruct
{
public:
  plIntegerStruct()
  {
    m_iInt8 = 1;
    m_uiUInt8 = 1;
    m_iInt16 = 1;
    m_iUInt16 = 1;
    m_iInt32 = 1;
    m_uiUInt32 = 1;
    m_iInt64 = 1;
    m_iUInt64 = 1;
  }

  void SetInt8(plInt8 i) { m_iInt8 = i; }
  plInt8 GetInt8() const { return m_iInt8; }
  void SetUInt8(plUInt8 i) { m_uiUInt8 = i; }
  plUInt8 GetUInt8() const { return m_uiUInt8; }
  void SetInt32(plInt32 i) { m_iInt32 = i; }
  plInt32 GetInt32() const { return m_iInt32; }
  void SetUInt32(plUInt32 i) { m_uiUInt32 = i; }
  plUInt32 GetUInt32() const { return m_uiUInt32; }

  plInt16 m_iInt16;
  plUInt16 m_iUInt16;
  plInt64 m_iInt64;
  plUInt64 m_iUInt64;

private:
  plInt8 m_iInt8;
  plUInt8 m_uiUInt8;
  plInt32 m_iInt32;
  plUInt32 m_uiUInt32;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plIntegerStruct);


struct plFloatStruct
{
public:
  plFloatStruct()
  {
    m_fFloat = 1.0f;
    m_fDouble = 1.0;
    m_Time = plTime::Seconds(1.0);
    m_Angle = plAngle::Degree(45.0f);
  }

  void SetFloat(float f) { m_fFloat = f; }
  float GetFloat() const { return m_fFloat; }
  void SetDouble(double d) { m_fDouble = d; }
  double GetDouble() const { return m_fDouble; }
  void SetTime(plTime t) { m_Time = t; }
  plTime GetTime() const { return m_Time; }
  plAngle GetAngle() const { return m_Angle; }
  void SetAngle(plAngle t) { m_Angle = t; }

private:
  float m_fFloat;
  double m_fDouble;
  plTime m_Time;
  plAngle m_Angle;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plFloatStruct);


class plPODClass : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPODClass, plReflectedClass);

public:
  plPODClass()
  {
    m_bBool = true;
    m_Color = plColor(1.0f, 0.0f, 0.0f, 0.0f);
    m_Color2 = plColorGammaUB(255, 10, 1);
    m_sString = "Test";
    m_Buffer.PushBack(0xFF);
    m_Buffer.PushBack(0x0);
    m_Buffer.PushBack(0xCD);
    m_VarianceAngle = {0.1f, plAngle::Degree(90.0f)};
  }

  plIntegerStruct m_IntegerStruct;
  plFloatStruct m_FloatStruct;

  void SetBool(bool b) { m_bBool = b; }
  bool GetBool() const { return m_bBool; }
  void SetColor(plColor c) { m_Color = c; }
  plColor GetColor() const { return m_Color; }
  const char* GetString() const { return m_sString.GetData(); }
  void SetString(const char* szSz) { m_sString = szSz; }

  const plDataBuffer& GetBuffer() const { return m_Buffer; }
  void SetBuffer(const plDataBuffer& data) { m_Buffer = data; }

  plVarianceTypeAngle GetCustom() const { return m_VarianceAngle; }
  void SetCustom(plVarianceTypeAngle value) { m_VarianceAngle = value; }

private:
  bool m_bBool;
  plColor m_Color;
  plColorGammaUB m_Color2;
  plString m_sString;
  plDataBuffer m_Buffer;
  plVarianceTypeAngle m_VarianceAngle;
};


class plMathClass : public plPODClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMathClass, plPODClass);

public:
  plMathClass()
  {
    m_vVec2 = plVec2(1.0f, 1.0f);
    m_vVec3 = plVec3(1.0f, 1.0f, 1.0f);
    m_vVec4 = plVec4(1.0f, 1.0f, 1.0f, 1.0f);
    m_Vec2I = plVec2I32(1, 1);
    m_Vec3I = plVec3I32(1, 1, 1);
    m_Vec4I = plVec4I32(1, 1, 1, 1);
    m_qQuat = plQuat(1.0f, 1.0f, 1.0f, 1.0f);
    m_mMat3.SetZero();
    m_mMat4.SetZero();
  }

  void SetVec2(plVec2 v) { m_vVec2 = v; }
  plVec2 GetVec2() const { return m_vVec2; }
  void SetVec3(plVec3 v) { m_vVec3 = v; }
  plVec3 GetVec3() const { return m_vVec3; }
  void SetVec4(plVec4 v) { m_vVec4 = v; }
  plVec4 GetVec4() const { return m_vVec4; }
  void SetQuat(plQuat q) { m_qQuat = q; }
  plQuat GetQuat() const { return m_qQuat; }
  void SetMat3(plMat3 m) { m_mMat3 = m; }
  plMat3 GetMat3() const { return m_mMat3; }
  void SetMat4(plMat4 m) { m_mMat4 = m; }
  plMat4 GetMat4() const { return m_mMat4; }

  plVec2I32 m_Vec2I;
  plVec3I32 m_Vec3I;
  plVec4I32 m_Vec4I;

private:
  plVec2 m_vVec2;
  plVec3 m_vVec3;
  plVec4 m_vVec4;
  plQuat m_qQuat;
  plMat3 m_mMat3;
  plMat4 m_mMat4;
};


struct plExampleEnum
{
  using StorageType = plInt8;
  enum Enum
  {
    Value1 = 0,      // normal value
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
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plExampleBitflags);


class plEnumerationsClass : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEnumerationsClass, plReflectedClass);

public:
  plEnumerationsClass()
  {
    m_EnumClass = plExampleEnum::Value2;
    m_BitflagsClass = plExampleBitflags::Value2;
  }

  void SetEnum(plExampleEnum::Enum e) { m_EnumClass = e; }
  plExampleEnum::Enum GetEnum() const { return m_EnumClass; }
  void SetBitflags(plBitflags<plExampleBitflags> e) { m_BitflagsClass = e; }
  plBitflags<plExampleBitflags> GetBitflags() const { return m_BitflagsClass; }

private:
  plEnum<plExampleEnum> m_EnumClass;
  plBitflags<plExampleBitflags> m_BitflagsClass;
};


struct InnerStruct
{
  PLASMA_DECLARE_POD_TYPE();

public:
  float m_fP1;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, InnerStruct);


class OuterClass : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(OuterClass, plReflectedClass);

public:
  InnerStruct m_Inner1;
  float m_fP1;
};

class ExtendedOuterClass : public OuterClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(ExtendedOuterClass, OuterClass);

public:
  plString m_more;
};

class plObjectTest : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plObjectTest, plReflectedClass);

public:
  plObjectTest() = default;
  ~plObjectTest()
  {
    for (OuterClass* pTest : m_ClassPtrArray)
    {
      plGetStaticRTTI<OuterClass>()->GetAllocator()->Deallocate(pTest);
    }
    for (plObjectTest* pTest : m_SubObjectSet)
    {
      plGetStaticRTTI<plObjectTest>()->GetAllocator()->Deallocate(pTest);
    }
    for (auto it = m_ClassPtrMap.GetIterator(); it.IsValid(); ++it)
    {
      plGetStaticRTTI<OuterClass>()->GetAllocator()->Deallocate(it.Value());
    }
  }

  plArrayPtr<const plString> GetStandardTypeSet() const;
  void StandardTypeSetInsert(const plString& value);
  void StandardTypeSetRemove(const plString& value);

  OuterClass m_MemberClass;

  plDynamicArray<double> m_StandardTypeArray;
  plDynamicArray<OuterClass> m_ClassArray;
  plDeque<OuterClass*> m_ClassPtrArray;

  plDynamicArray<plString> m_StandardTypeSet;
  plSet<plObjectTest*> m_SubObjectSet;

  plMap<plString, double> m_StandardTypeMap;
  plHashTable<plString, OuterClass> m_ClassMap;
  plMap<plString, OuterClass*> m_ClassPtrMap;
};


class plMirrorTest : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMirrorTest, plReflectedClass);

public:
  plMirrorTest() = default;

  plMathClass m_math;
  plObjectTest m_object;
};
