#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Serialization);

class TestContext : public plRttiConverterContext
{
public:
  virtual plInternal::NewInstance<void> CreateObject(const plUuid& guid, const plRTTI* pRtti) override
  {
    auto pObj = pRtti->GetAllocator()->Allocate<void>();
    RegisterObject(guid, pRtti, pObj);
    return pObj;
  }

  virtual void DeleteObject(const plUuid& guid) override
  {
    auto object = GetObjectByGUID(guid);
    object.m_pType->GetAllocator()->Deallocate(object.m_pObject);

    UnregisterObject(guid);
  }
};

template <typename T>
void TestSerialize(T* pObject)
{
  plAbstractObjectGraph graph;
  TestContext context;
  plRttiConverterWriter conv(&graph, &context, true, true);

  const plRTTI* pRtti = plGetStaticRTTI<T>();
  plUuid guid;
  guid.CreateNewUuid();

  context.RegisterObject(guid, pRtti, pObject);
  plAbstractObjectNode* pNode = conv.AddObjectToGraph(pRtti, pObject, "root");

  PLASMA_TEST_BOOL(pNode->GetGuid() == guid);
  PLASMA_TEST_STRING(pNode->GetType(), pRtti->GetTypeName());
  PLASMA_TEST_INT(pNode->GetProperties().GetCount(), pNode->GetProperties().GetCount());

  {
    plContiguousMemoryStreamStorage storage;
    plMemoryStreamWriter writer(&storage);
    plMemoryStreamReader reader(&storage);

    plAbstractGraphDdlSerializer::Write(writer, &graph);

    plStringBuilder sData, sData2;
    sData.SetSubString_ElementCount((const char*)storage.GetData(), storage.GetStorageSize32());


    plRttiConverterReader convRead(&graph, &context);
    auto* pRootNode = graph.GetNodeByName("root");
    PLASMA_TEST_BOOL(pRootNode != nullptr);

    T target;
    convRead.ApplyPropertiesToObject(pRootNode, pRtti, &target);
    PLASMA_TEST_BOOL(target == *pObject);

    // Overwrite again to test for leaks as existing values have to be removed first by plRttiConverterReader.
    convRead.ApplyPropertiesToObject(pRootNode, pRtti, &target);
    PLASMA_TEST_BOOL(target == *pObject);

    {
      T clone;
      plReflectionSerializer::Clone(pObject, &clone, pRtti);
      PLASMA_TEST_BOOL(clone == *pObject);
      PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(&clone, pObject, pRtti));
    }

    {
      T* pClone = plReflectionSerializer::Clone(pObject);
      PLASMA_TEST_BOOL(*pClone == *pObject);
      PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(pClone, pObject));
      // Overwrite again to test for leaks as existing values have to be removed first by clone.
      plReflectionSerializer::Clone(pObject, pClone, pRtti);
      PLASMA_TEST_BOOL(*pClone == *pObject);
      PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(pClone, pObject, pRtti));
      pRtti->GetAllocator()->Deallocate(pClone);
    }

    plAbstractObjectGraph graph2;
    plAbstractGraphDdlSerializer::Read(reader, &graph2).IgnoreResult();

    plContiguousMemoryStreamStorage storage2;
    plMemoryStreamWriter writer2(&storage2);

    plAbstractGraphDdlSerializer::Write(writer2, &graph2);
    sData2.SetSubString_ElementCount((const char*)storage2.GetData(), storage2.GetStorageSize32());

    PLASMA_TEST_BOOL(sData == sData2);
  }

  {
    plContiguousMemoryStreamStorage storage;
    plMemoryStreamWriter writer(&storage);
    plMemoryStreamReader reader(&storage);

    plAbstractGraphBinarySerializer::Write(writer, &graph);

    plRttiConverterReader convRead(&graph, &context);
    auto* pRootNode = graph.GetNodeByName("root");
    PLASMA_TEST_BOOL(pRootNode != nullptr);

    T target;
    convRead.ApplyPropertiesToObject(pRootNode, pRtti, &target);
    PLASMA_TEST_BOOL(target == *pObject);

    plAbstractObjectGraph graph2;
    plAbstractGraphBinarySerializer::Read(reader, &graph2);

    plContiguousMemoryStreamStorage storage2;
    plMemoryStreamWriter writer2(&storage2);

    plAbstractGraphBinarySerializer::Write(writer2, &graph2);

    PLASMA_TEST_INT(storage.GetStorageSize32(), storage2.GetStorageSize32());

    if (storage.GetStorageSize32() == storage2.GetStorageSize32())
    {
      PLASMA_TEST_BOOL(plMemoryUtils::RawByteCompare(storage.GetData(), storage2.GetData(), storage.GetStorageSize32()) == 0);
    }
  }
}

PLASMA_CREATE_SIMPLE_TEST(Serialization, RttiConverter)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PODs")
  {
    plTestStruct t1;
    t1.m_fFloat1 = 5.0f;
    t1.m_UInt8 = 222;
    t1.m_variant = "A";
    t1.m_Angle = plAngle::Degree(5);
    t1.m_DataBuffer.PushBack(1);
    t1.m_DataBuffer.PushBack(5);
    t1.m_vVec3I = plVec3I32(0, 1, 333);
    TestSerialize(&t1);

    {
      plTestStruct clone;
      plReflectionSerializer::Clone(&t1, &clone, plGetStaticRTTI<plTestStruct>());
      PLASMA_TEST_BOOL(t1 == clone);
      PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestStruct>()));
      clone.m_variant = "Test";
      PLASMA_TEST_BOOL(!plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestStruct>()));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "EmbededStruct")
  {
    plTestClass1 t1;
    t1.m_Color = plColor::Yellow;
    t1.m_Struct.m_fFloat1 = 5.0f;
    t1.m_Struct.m_UInt8 = 222;
    t1.m_Struct.m_variant = "A";
    t1.m_Struct.m_Angle = plAngle::Degree(5);
    t1.m_Struct.m_DataBuffer.PushBack(1);
    t1.m_Struct.m_DataBuffer.PushBack(5);
    t1.m_Struct.m_vVec3I = plVec3I32(0, 1, 333);
    TestSerialize(&t1);

    {
      plTestClass1 clone;
      plReflectionSerializer::Clone(&t1, &clone, plGetStaticRTTI<plTestClass1>());
      PLASMA_TEST_BOOL(t1 == clone);
      PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestClass1>()));
      clone.m_Struct.m_DataBuffer[1] = 6;
      PLASMA_TEST_BOOL(!plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestClass1>()));
      clone.m_Struct.m_DataBuffer[1] = 5;
      clone.m_Struct.m_variant = plVec3(1, 2, 3);
      PLASMA_TEST_BOOL(!plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestClass1>()));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Enum")
  {
    plTestEnumStruct t1;
    t1.m_enum = plExampleEnum::Value2;
    t1.m_enumClass = plExampleEnum::Value3;
    t1.SetEnum(plExampleEnum::Value2);
    t1.SetEnumClass(plExampleEnum::Value3);
    TestSerialize(&t1);

    {
      plTestEnumStruct clone;
      plReflectionSerializer::Clone(&t1, &clone, plGetStaticRTTI<plTestEnumStruct>());
      PLASMA_TEST_BOOL(t1 == clone);
      PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestEnumStruct>()));
      clone.m_enum = plExampleEnum::Value3;
      PLASMA_TEST_BOOL(!plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestEnumStruct>()));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Bitflags")
  {
    plTestBitflagsStruct t1;
    t1.m_bitflagsClass.SetValue(0);
    t1.SetBitflagsClass(plExampleBitflags::Value1 | plExampleBitflags::Value2);
    TestSerialize(&t1);

    {
      plTestBitflagsStruct clone;
      plReflectionSerializer::Clone(&t1, &clone, plGetStaticRTTI<plTestBitflagsStruct>());
      PLASMA_TEST_BOOL(t1 == clone);
      PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestBitflagsStruct>()));
      clone.m_bitflagsClass = plExampleBitflags::Value1;
      PLASMA_TEST_BOOL(!plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestBitflagsStruct>()));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Derived Class")
  {
    plTestClass2 t1;
    t1.m_Color = plColor::Yellow;
    t1.m_Struct.m_fFloat1 = 5.0f;
    t1.m_Struct.m_UInt8 = 222;
    t1.m_Struct.m_variant = "A";
    t1.m_Struct.m_Angle = plAngle::Degree(5);
    t1.m_Struct.m_DataBuffer.PushBack(1);
    t1.m_Struct.m_DataBuffer.PushBack(5);
    t1.m_Struct.m_vVec3I = plVec3I32(0, 1, 333);
    t1.m_Time = plTime::Seconds(22.2f);
    t1.m_enumClass = plExampleEnum::Value3;
    t1.m_bitflagsClass = plExampleBitflags::Value1 | plExampleBitflags::Value2;
    t1.m_array.PushBack(40.0f);
    t1.m_array.PushBack(-1.5f);
    t1.m_Variant = plVec4(1, 2, 3, 4);
    t1.SetText("LALALALA");
    TestSerialize(&t1);

    {
      plTestClass2 clone;
      plReflectionSerializer::Clone(&t1, &clone, plGetStaticRTTI<plTestClass2>());
      PLASMA_TEST_BOOL(t1 == clone);
      PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestClass2>()));
      clone.m_Struct.m_DataBuffer[1] = 6;
      PLASMA_TEST_BOOL(!plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestClass2>()));
      clone.m_Struct.m_DataBuffer[1] = 5;
      t1.m_array.PushBack(-1.33f);
      PLASMA_TEST_BOOL(!plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestClass2>()));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Arrays")
  {
    plTestArrays t1;
    t1.m_Hybrid.PushBack(4.5f);
    t1.m_Hybrid.PushBack(2.3f);
    t1.m_HybridChar.PushBack("Test");

    plTestStruct3 ts;
    ts.m_fFloat1 = 5.0f;
    ts.m_UInt8 = 22;
    t1.m_Dynamic.PushBack(ts);
    t1.m_Dynamic.PushBack(ts);
    t1.m_Deque.PushBack(plTestArrays());
    TestSerialize(&t1);

    {
      plTestArrays clone;
      plReflectionSerializer::Clone(&t1, &clone, plGetStaticRTTI<plTestArrays>());
      PLASMA_TEST_BOOL(t1 == clone);
      PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestArrays>()));
      clone.m_Dynamic.PushBack(plTestStruct3());
      PLASMA_TEST_BOOL(!plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestArrays>()));
      clone.m_Dynamic.PopBack();
      clone.m_Hybrid.PushBack(444.0f);
      PLASMA_TEST_BOOL(!plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestArrays>()));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Sets")
  {
    plTestSets t1;
    t1.m_SetMember.Insert(0);
    t1.m_SetMember.Insert(5);
    t1.m_SetMember.Insert(-33);
    t1.m_SetAccessor.Insert(-0.0f);
    t1.m_SetAccessor.Insert(5.4f);
    t1.m_SetAccessor.Insert(-33.0f);
    t1.m_Deque.PushBack(3);
    t1.m_Deque.PushBack(33);
    t1.m_Array.PushBack("Test");
    t1.m_Array.PushBack("Bla");
    TestSerialize(&t1);

    {
      plTestSets clone;
      plReflectionSerializer::Clone(&t1, &clone, plGetStaticRTTI<plTestSets>());
      PLASMA_TEST_BOOL(t1 == clone);
      PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestSets>()));
      clone.m_SetMember.Insert(12);
      PLASMA_TEST_BOOL(!plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestSets>()));
      clone.m_SetMember.Remove(12);
      clone.m_Array.PushBack("Bla2");
      PLASMA_TEST_BOOL(!plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestSets>()));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Pointer")
  {
    plTestPtr t1;
    t1.m_sString = "Ttttest";
    t1.m_pArrays = PLASMA_DEFAULT_NEW(plTestArrays);
    t1.m_pArraysDirect = PLASMA_DEFAULT_NEW(plTestArrays);
    t1.m_ArrayPtr.PushBack(PLASMA_DEFAULT_NEW(plTestArrays));
    t1.m_SetPtr.Insert(PLASMA_DEFAULT_NEW(plTestSets));
    TestSerialize(&t1);

    {
      plTestPtr clone;
      plReflectionSerializer::Clone(&t1, &clone, plGetStaticRTTI<plTestPtr>());
      PLASMA_TEST_BOOL(t1 == clone);
      PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestPtr>()));
      clone.m_SetPtr.GetIterator().Key()->m_Deque.PushBack(42);
      PLASMA_TEST_BOOL(!plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestPtr>()));
      clone.m_SetPtr.GetIterator().Key()->m_Deque.PopBack();
      clone.m_ArrayPtr[0]->m_Hybrid.PushBack(123.0f);
      PLASMA_TEST_BOOL(!plReflectionUtils::IsEqual(&t1, &clone, plGetStaticRTTI<plTestPtr>()));
    }
  }
}
