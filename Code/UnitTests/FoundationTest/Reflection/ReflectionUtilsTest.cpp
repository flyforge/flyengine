#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

template <typename T>
static void SetComponentTest(plVec2Template<T> vVector, T value)
{
  plVariant var = vVector;
  plReflectionUtils::SetComponent(var, 0, value);
  PLASMA_TEST_BOOL(var.Get<plVec2Template<T>>().x == value);
  plReflectionUtils::SetComponent(var, 1, value);
  PLASMA_TEST_BOOL(var.Get<plVec2Template<T>>().y == value);
}

template <typename T>
static void SetComponentTest(plVec3Template<T> vVector, T value)
{
  plVariant var = vVector;
  plReflectionUtils::SetComponent(var, 0, value);
  PLASMA_TEST_BOOL(var.Get<plVec3Template<T>>().x == value);
  plReflectionUtils::SetComponent(var, 1, value);
  PLASMA_TEST_BOOL(var.Get<plVec3Template<T>>().y == value);
  plReflectionUtils::SetComponent(var, 2, value);
  PLASMA_TEST_BOOL(var.Get<plVec3Template<T>>().z == value);
}

template <typename T>
static void SetComponentTest(plVec4Template<T> vVector, T value)
{
  plVariant var = vVector;
  plReflectionUtils::SetComponent(var, 0, value);
  PLASMA_TEST_BOOL(var.Get<plVec4Template<T>>().x == value);
  plReflectionUtils::SetComponent(var, 1, value);
  PLASMA_TEST_BOOL(var.Get<plVec4Template<T>>().y == value);
  plReflectionUtils::SetComponent(var, 2, value);
  PLASMA_TEST_BOOL(var.Get<plVec4Template<T>>().z == value);
  plReflectionUtils::SetComponent(var, 3, value);
  PLASMA_TEST_BOOL(var.Get<plVec4Template<T>>().w == value);
}

template <class T>
static void ClampValueTest(T tooSmall, T tooBig, T min, T max)
{
  plClampValueAttribute minClamp(min, {});
  plClampValueAttribute maxClamp({}, max);
  plClampValueAttribute bothClamp(min, max);

  plVariant value = tooSmall;
  PLASMA_TEST_BOOL(plReflectionUtils::ClampValue(value, &minClamp).Succeeded());
  PLASMA_TEST_BOOL(value == min);

  value = tooSmall;
  PLASMA_TEST_BOOL(plReflectionUtils::ClampValue(value, &bothClamp).Succeeded());
  PLASMA_TEST_BOOL(value == min);

  value = tooBig;
  PLASMA_TEST_BOOL(plReflectionUtils::ClampValue(value, &maxClamp).Succeeded());
  PLASMA_TEST_BOOL(value == max);

  value = tooBig;
  PLASMA_TEST_BOOL(plReflectionUtils::ClampValue(value, &bothClamp).Succeeded());
  PLASMA_TEST_BOOL(value == max);
}


PLASMA_CREATE_SIMPLE_TEST(Reflection, Utils)
{
  plDefaultMemoryStreamStorage StreamStorage;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "WriteObjectToDDL")
  {
    plMemoryStreamWriter FileOut(&StreamStorage);

    plTestClass2 c2;
    c2.SetText("Hallo");
    c2.m_MyVector.Set(14, 16, 18);
    c2.m_Struct.m_fFloat1 = 128;
    c2.m_Struct.m_UInt8 = 234;
    c2.m_Struct.m_Angle = plAngle::Degree(360);
    c2.m_Struct.m_vVec3I = plVec3I32(9, 8, 7);
    c2.m_Struct.m_DataBuffer.Clear();
    c2.m_Color = plColor(0.1f, 0.2f, 0.3f);
    c2.m_Time = plTime::Seconds(91.0f);
    c2.m_enumClass = plExampleEnum::Value3;
    c2.m_bitflagsClass = plExampleBitflags::Value1 | plExampleBitflags::Value2 | plExampleBitflags::Value3;
    c2.m_array.PushBack(5.0f);
    c2.m_array.PushBack(10.0f);
    c2.m_Variant = plVec3(1.0f, 2.0f, 3.0f);

    plReflectionSerializer::WriteObjectToDDL(FileOut, c2.GetDynamicRTTI(), &c2, false, plOpenDdlWriter::TypeStringMode::Compliant);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReadObjectPropertiesFromDDL")
  {
    plMemoryStreamReader FileIn(&StreamStorage);

    plTestClass2 c2;

    plReflectionSerializer::ReadObjectPropertiesFromDDL(FileIn, *c2.GetDynamicRTTI(), &c2);

    PLASMA_TEST_STRING(c2.GetText(), "Hallo");
    PLASMA_TEST_VEC3(c2.m_MyVector, plVec3(3, 4, 5), 0.0f);
    PLASMA_TEST_FLOAT(c2.m_Time.GetSeconds(), 91.0f, 0.0f);
    PLASMA_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    PLASMA_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    PLASMA_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    PLASMA_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    PLASMA_TEST_INT(c2.m_Struct.m_UInt8, 234);
    PLASMA_TEST_BOOL(c2.m_Struct.m_Angle == plAngle::Degree(360));
    PLASMA_TEST_BOOL(c2.m_Struct.m_vVec3I == plVec3I32(9, 8, 7));
    PLASMA_TEST_BOOL(c2.m_Struct.m_DataBuffer == plDataBuffer());
    PLASMA_TEST_BOOL(c2.m_enumClass == plExampleEnum::Value3);
    PLASMA_TEST_BOOL(c2.m_bitflagsClass == (plExampleBitflags::Value1 | plExampleBitflags::Value2 | plExampleBitflags::Value3));
    PLASMA_TEST_INT(c2.m_array.GetCount(), 2);
    if (c2.m_array.GetCount() == 2)
    {
      PLASMA_TEST_FLOAT(c2.m_array[0], 5.0f, 0.0f);
      PLASMA_TEST_FLOAT(c2.m_array[1], 10.0f, 0.0f);
    }
    PLASMA_TEST_VEC3(c2.m_Variant.Get<plVec3>(), plVec3(1.0f, 2.0f, 3.0f), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReadObjectPropertiesFromDDL (different type)")
  {
    // here we restore the same properties into a different type of object which has properties that are named the same
    // but may have slightly different types (but which are compatible)

    plMemoryStreamReader FileIn(&StreamStorage);

    plTestClass2b c2;

    plReflectionSerializer::ReadObjectPropertiesFromDDL(FileIn, *c2.GetDynamicRTTI(), &c2);

    PLASMA_TEST_STRING(c2.GetText(), "Tut"); // not restored, different property name
    PLASMA_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    PLASMA_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    PLASMA_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    PLASMA_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    PLASMA_TEST_INT(c2.m_Struct.m_UInt8, 234);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReadObjectFromDDL")
  {
    plMemoryStreamReader FileIn(&StreamStorage);

    const plRTTI* pRtti;
    void* pObject = plReflectionSerializer::ReadObjectFromDDL(FileIn, pRtti);

    plTestClass2& c2 = *((plTestClass2*)pObject);

    PLASMA_TEST_STRING(c2.GetText(), "Hallo");
    PLASMA_TEST_VEC3(c2.m_MyVector, plVec3(3, 4, 5), 0.0f);
    PLASMA_TEST_FLOAT(c2.m_Time.GetSeconds(), 91.0f, 0.0f);
    PLASMA_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    PLASMA_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    PLASMA_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    PLASMA_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    PLASMA_TEST_INT(c2.m_Struct.m_UInt8, 234);
    PLASMA_TEST_BOOL(c2.m_Struct.m_Angle == plAngle::Degree(360));
    PLASMA_TEST_BOOL(c2.m_Struct.m_vVec3I == plVec3I32(9, 8, 7));
    PLASMA_TEST_BOOL(c2.m_Struct.m_DataBuffer == plDataBuffer());
    PLASMA_TEST_BOOL(c2.m_enumClass == plExampleEnum::Value3);
    PLASMA_TEST_BOOL(c2.m_bitflagsClass == (plExampleBitflags::Value1 | plExampleBitflags::Value2 | plExampleBitflags::Value3));
    PLASMA_TEST_INT(c2.m_array.GetCount(), 2);
    if (c2.m_array.GetCount() == 2)
    {
      PLASMA_TEST_FLOAT(c2.m_array[0], 5.0f, 0.0f);
      PLASMA_TEST_FLOAT(c2.m_array[1], 10.0f, 0.0f);
    }
    PLASMA_TEST_VEC3(c2.m_Variant.Get<plVec3>(), plVec3(1.0f, 2.0f, 3.0f), 0.0f);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }

  plFileSystem::ClearAllDataDirectories();

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetComponent")
  {
    SetComponentTest(plVec2(0.0f, 0.1f), -0.5f);
    SetComponentTest(plVec3(0.0f, 0.1f, 0.2f), -0.5f);
    SetComponentTest(plVec4(0.0f, 0.1f, 0.2f, 0.3f), -0.5f);
    SetComponentTest(plVec2I32(0, 1), -4);
    SetComponentTest(plVec3I32(0, 1, 2), -4);
    SetComponentTest(plVec4I32(0, 1, 2, 3), -4);
    SetComponentTest(plVec2U32(0, 1), 4u);
    SetComponentTest(plVec3U32(0, 1, 2), 4u);
    SetComponentTest(plVec4U32(0, 1, 2, 3), 4u);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ClampValue")
  {
    ClampValueTest<float>(-1, 1000, 2, 4);
    ClampValueTest<double>(-1, 1000, 2, 4);
    ClampValueTest<plInt32>(-1, 1000, 2, 4);
    ClampValueTest<plUInt64>(1, 1000, 2, 4);
    ClampValueTest<plTime>(plTime::Milliseconds(1), plTime::Milliseconds(1000), plTime::Milliseconds(2), plTime::Milliseconds(4));
    ClampValueTest<plAngle>(plAngle::Degree(1), plAngle::Degree(1000), plAngle::Degree(2), plAngle::Degree(4));
    ClampValueTest<plVec3>(plVec3(1), plVec3(1000), plVec3(2), plVec3(4));
    ClampValueTest<plVec4I32>(plVec4I32(1), plVec4I32(1000), plVec4I32(2), plVec4I32(4));
    ClampValueTest<plVec4U32>(plVec4U32(1), plVec4U32(1000), plVec4U32(2), plVec4U32(4));

    plVarianceTypeFloat vf = {1.0f, 2.0f};
    plVariant variance = vf;
    PLASMA_TEST_BOOL(plReflectionUtils::ClampValue(variance, nullptr).Succeeded());

    plVarianceTypeFloat clamp = {2.0f, 3.0f};
    plClampValueAttribute minClamp(clamp, {});
    PLASMA_TEST_BOOL(plReflectionUtils::ClampValue(variance, &minClamp).Failed());
  }
}
